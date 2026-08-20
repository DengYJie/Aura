#include "AuditService.h"

#include "AuditMetadataSanitizer.h"

#include <QJsonObject>
#include <QUuid>

AuditService::AuditService(std::shared_ptr<AuditRepository> repository)
    : m_repository(std::move(repository)) {}

QString AuditService::generateEventId() {
  return QStringLiteral("evt_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString AuditService::generateTraceId() {
  return QStringLiteral("trace_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

audit::AuditContext AuditService::makeContext(const QString& sourceModule,
                                              const QString& sourceUseCase,
                                              std::optional<int> uid) {
  audit::AuditContext context;
  context.uid = uid;
  context.sourceModule = sourceModule;
  context.sourceUseCase = sourceUseCase;
  context.clientType = QStringLiteral("desktop");
  return context;
}

void AuditService::logAsync(const audit::AuditContext& context,
                            audit::AuditEvent event,
                            std::function<void(bool)> callback) const {
  if (!m_repository) {
    if (callback) {
      callback(false);
    }
    return;
  }

  if (event.eventId.isEmpty()) {
    event.eventId = generateEventId();
  }
  if (!event.eventTime.isValid()) {
    event.eventTime = QDateTime::currentDateTime();
  }
  event.metadata = AuditMetadataSanitizer::sanitize(event.metadata);
  event.message = AuditMetadataSanitizer::sanitizeText(event.message);

  m_repository->appendAsync(context, event, std::move(callback));
}

void AuditService::queryRecentLogsAsync(const audit::AuditQuery& query,
                                        std::function<void(AuditLogPage)> callback) const {
  if (!m_repository) {
    if (callback) {
      callback({});
    }
    return;
  }
  m_repository->queryRecentLogsAsync(query, std::move(callback));
}

void AuditService::queryDashboardStatsAsync(const audit::DashboardQuery& query,
                                            std::function<void(DashboardStats)> callback) const {
  if (!m_repository) {
    if (callback) {
      callback({});
    }
    return;
  }
  m_repository->queryDashboardStatsAsync(query, std::move(callback));
}
