#include "SmsLoginUseCase.h"

#include <QJsonObject>

#include <utility>

SmsLoginUseCase::SmsLoginUseCase(std::shared_ptr<UserRepository> userRepository,
                                 std::shared_ptr<AuditService> auditService)
    : m_userRepository(std::move(userRepository)),
      m_auditService(std::move(auditService)) {}

void SmsLoginUseCase::smsLoginAsync(const QString& account, const QString& code,
                                    std::function<void(bool, int, const QString&, const QString&)> callback) {
    const QString traceId = AuditService::generateTraceId();
    auto context = AuditService::makeContext(QStringLiteral("auth"), QStringLiteral("SmsLoginUseCase"));
    context.traceId = traceId;

    if (code.trimmed().length() != 6) {
        audit::AuditEvent event;
        event.eventType = QString::fromLatin1(audit::EventAuthSmsFailed);
        event.eventLevel = QString::fromLatin1(audit::LevelWarn);
        event.eventResult = QString::fromLatin1(audit::ResultFailed);
        event.reasonCode = QString::fromLatin1(audit::ReasonInvalidCode);
        event.message = QStringLiteral("短信登录失败：验证码格式无效");
        event.metadata = QJsonObject{{QStringLiteral("loginMode"), QStringLiteral("sms")}};
        if (m_auditService) {
            m_auditService->logAsync(context, event);
        }
        callback(false, -1, QString(), QStringLiteral("请输入 6 位有效验证码"));
        return;
    }
    m_userRepository->getUserByAccountAsync(account.trimmed(),
        [this, callback, context](std::optional<UserAuthDTO> userOpt) mutable {
            if (!userOpt.has_value()) {
                audit::AuditEvent event;
                event.eventType = QString::fromLatin1(audit::EventAuthSmsFailed);
                event.eventLevel = QString::fromLatin1(audit::LevelWarn);
                event.eventResult = QString::fromLatin1(audit::ResultFailed);
                event.reasonCode = QString::fromLatin1(audit::ReasonBadAccount);
                event.message = QStringLiteral("短信登录失败：账户不存在");
                event.metadata = QJsonObject{{QStringLiteral("loginMode"), QStringLiteral("sms")}};
                if (m_auditService) {
                    m_auditService->logAsync(context, event);
                }
                callback(false, -1, QString(), QStringLiteral("账户不存在"));
            } else {
                context.uid = userOpt->user.uid();
                audit::AuditEvent event;
                event.eventType = QString::fromLatin1(audit::EventAuthSmsSuccess);
                event.eventLevel = QString::fromLatin1(audit::LevelInfo);
                event.eventResult = QString::fromLatin1(audit::ResultSuccess);
                event.message = QStringLiteral("短信登录成功");
                event.metadata = QJsonObject{{QStringLiteral("loginMode"), QStringLiteral("sms")}};
                if (m_auditService) {
                    m_auditService->logAsync(context, event);
                }
                callback(true, userOpt->user.uid(), userOpt->user.username(), QStringLiteral("登录成功"));
            }
        });
}
