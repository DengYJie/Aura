#include "BehaviorRiskService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QJsonObject featureToJson(const TrajectoryFeature& feat) {
  return QJsonObject{
      {QStringLiteral("totalDuration"), feat.totalDuration},
      {QStringLiteral("maxSpeedX"), feat.maxSpeedX},
      {QStringLiteral("avgSpeedX"), feat.avgSpeedX},
      {QStringLiteral("varDisplacementY"), feat.varDisplacementY},
      {QStringLiteral("reversalCount"), feat.reversalCount},
      {QStringLiteral("pauseCount"), feat.pauseCount},
      {QStringLiteral("maxAccelX"), feat.maxAccelX},
      {QStringLiteral("speedEntropy"), feat.speedEntropy},
      {QStringLiteral("straightnessRatio"), feat.straightnessRatio},
      {QStringLiteral("endSlowdownRatio"), feat.endSlowdownRatio}};
}

QJsonArray trajectoryToJson(const std::vector<TrackPoint>& trajectory) {
  QJsonArray points;
  for (const TrackPoint& point : trajectory) {
    points.push_back(QJsonObject{{QStringLiteral("x"), point.x},
                                 {QStringLiteral("y"), point.y},
                                 {QStringLiteral("t"), static_cast<qint64>(point.timestamp)}});
  }
  return points;
}

}  // namespace

BehaviorRiskService::BehaviorRiskService(std::shared_ptr<BehaviorRiskRepository> repository,
                                         std::shared_ptr<AuditService> auditService,
                                         std::shared_ptr<BehaviorEvidenceRepository> behaviorEvidenceRepository)
    : m_repository(std::move(repository)),
      m_auditService(std::move(auditService)),
      m_behaviorEvidenceRepository(std::move(behaviorEvidenceRepository)) {}

void BehaviorRiskService::verifyTrajectoryAsync(const std::vector<TrackPoint>& trajectory,
                                                BehaviorRiskRepository::Callback callback) {
  // 1. 业务逻辑：提取轨迹特征
  TrajectoryFeature feat = BehaviorTracker::extractFeatures(trajectory);
  const QString traceId = AuditService::generateTraceId();
  const QString eventId = AuditService::generateEventId();

  // 2. 将特征抛给数据层（Repository）进行打分
  if (m_repository) {
    m_repository->evaluateAsync(feat, [this, feat, trajectory, callback = std::move(callback), traceId, eventId](std::expected<float, std::string> result) mutable {
      auto context = AuditService::makeContext(QStringLiteral("risk"), QStringLiteral("BehaviorRiskService"));
      context.traceId = traceId;
      audit::AuditEvent event;
      event.eventId = eventId;
      event.metadata = QJsonObject{{QStringLiteral("scenario"), QStringLiteral("slider_captcha")},
                                   {QStringLiteral("featureSummary"), featureToJson(feat)}};
      if (result.has_value()) {
        const float score = result.value();
        const bool isHumanLike = score >= 0.5f;
        event.eventType = isHumanLike ? QString::fromLatin1(audit::EventCaptchaPassed)
                                      : QString::fromLatin1(audit::EventCaptchaRiskBlocked);
        event.eventLevel = isHumanLike ? QString::fromLatin1(audit::LevelInfo)
                                       : QString::fromLatin1(audit::LevelWarn);
        event.eventResult = isHumanLike ? QString::fromLatin1(audit::ResultSuccess)
                                        : QString::fromLatin1(audit::ResultBlocked);
        event.riskScore = score;
        event.reasonCode = isHumanLike ? QString() : QString::fromLatin1(audit::ReasonLowLiveness);
        event.message = isHumanLike ? QStringLiteral("行为风控通过")
                                    : QStringLiteral("行为风控拦截：轨迹判定异常");

        if (m_behaviorEvidenceRepository) {
          BehaviorEvidence evidence;
          evidence.eventId = eventId;
          evidence.scenario = QStringLiteral("slider_captcha");
          evidence.capturedAt = QDateTime::currentDateTime();
          evidence.featureJson = QString::fromUtf8(QJsonDocument(featureToJson(feat)).toJson(QJsonDocument::Compact));
          evidence.trajectoryJson = QString::fromUtf8(QJsonDocument(trajectoryToJson(trajectory)).toJson(QJsonDocument::Compact));
          evidence.riskScore = score;
          evidence.modelVersion = QStringLiteral("behavior_mlp.onnx@v1");
          evidence.decision = isHumanLike ? QStringLiteral("pass") : QStringLiteral("block");
          m_behaviorEvidenceRepository->appendAsync(evidence, {});
        }
      } else {
        event.eventType = QString::fromLatin1(audit::EventCaptchaFailed);
        event.eventLevel = QString::fromLatin1(audit::LevelWarn);
        event.eventResult = QString::fromLatin1(audit::ResultError);
        event.reasonCode = QString::fromLatin1(audit::ReasonRiskModelFailed);
        event.message = QStringLiteral("行为风控失败：风险模型不可用");
        event.metadata.insert(QStringLiteral("error"), QString::fromStdString(result.error()));
      }

      if (m_auditService) {
        m_auditService->logAsync(context, event);
      }
      callback(std::move(result));
    });
  } else {
    auto context = AuditService::makeContext(QStringLiteral("risk"), QStringLiteral("BehaviorRiskService"));
    context.traceId = traceId;
    audit::AuditEvent event;
    event.eventId = eventId;
    event.eventType = QString::fromLatin1(audit::EventCaptchaFailed);
    event.eventLevel = QString::fromLatin1(audit::LevelWarn);
    event.eventResult = QString::fromLatin1(audit::ResultError);
    event.reasonCode = QString::fromLatin1(audit::ReasonRiskModelFailed);
    event.message = QStringLiteral("行为风控失败：风险仓储未初始化");
    if (m_auditService) {
      m_auditService->logAsync(context, event);
    }
    callback(std::unexpected("BehaviorRiskService: Repository not initialized"));
  }
}
