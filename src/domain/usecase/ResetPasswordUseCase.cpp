#include "ResetPasswordUseCase.h"
#include "core/CryptoUtils.h"

#include <QJsonObject>

#include <utility>

ResetPasswordUseCase::ResetPasswordUseCase(std::shared_ptr<UserRepository> userRepository,
                                           std::shared_ptr<AuditService> auditService)
    : m_userRepository(std::move(userRepository)),
      m_auditService(std::move(auditService)) {}

void ResetPasswordUseCase::resetPasswordAsync(const QString& account, const QString& newPassword,
                                              std::function<void(bool, QString)> callback) {
    const QString traceId = AuditService::generateTraceId();
    m_userRepository->getUserByAccountAsync(account,
        [=, this](std::optional<UserAuthDTO> userOpt) {
            auto context = AuditService::makeContext(QStringLiteral("auth"), QStringLiteral("ResetPasswordUseCase"));
            context.traceId = traceId;
            if (!userOpt.has_value()) {
                audit::AuditEvent event;
                event.eventType = QString::fromLatin1(audit::EventPasswordResetFailed);
                event.eventLevel = QString::fromLatin1(audit::LevelWarn);
                event.eventResult = QString::fromLatin1(audit::ResultFailed);
                event.reasonCode = QString::fromLatin1(audit::ReasonBadAccount);
                event.message = QStringLiteral("密码重置失败：未找到该账户");
                if (m_auditService) {
                    m_auditService->logAsync(context, event);
                }
                callback(false, QStringLiteral("未找到该账户"));
                return;
            }
            context.uid = userOpt->user.uid();
            QString pwd_hash = CryptoUtils::hashPassword(newPassword);
            m_userRepository->updatePasswordAsync(userOpt->user.uid(), pwd_hash,
                [this, callback, context](bool success) {
                    audit::AuditEvent event;
                    if (success) {
                        event.eventType = QString::fromLatin1(audit::EventPasswordResetSuccess);
                        event.eventLevel = QString::fromLatin1(audit::LevelInfo);
                        event.eventResult = QString::fromLatin1(audit::ResultSuccess);
                        event.message = QStringLiteral("密码重置成功");
                        if (m_auditService) {
                            m_auditService->logAsync(context, event);
                        }
                        callback(true, QStringLiteral("重置密码成功"));
                    } else {
                        event.eventType = QString::fromLatin1(audit::EventPasswordResetFailed);
                        event.eventLevel = QString::fromLatin1(audit::LevelWarn);
                        event.eventResult = QString::fromLatin1(audit::ResultError);
                        event.reasonCode = QString::fromLatin1(audit::ReasonSaveFailed);
                        event.message = QStringLiteral("密码重置失败：更新数据库失败");
                        if (m_auditService) {
                            m_auditService->logAsync(context, event);
                        }
                        callback(false, QStringLiteral("重置失败，请稍后重试"));
                    }
                });
        });
}
