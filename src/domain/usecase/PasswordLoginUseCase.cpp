#include "PasswordLoginUseCase.h"
#include "core/CryptoUtils.h"

#include <QJsonObject>

#include <utility>

PasswordLoginUseCase::PasswordLoginUseCase(std::shared_ptr<UserRepository> userRepository,
                                           std::shared_ptr<AuditService> auditService)
    : m_userRepository(std::move(userRepository)),
      m_auditService(std::move(auditService)) {}

void PasswordLoginUseCase::loginAsync(const QString& account, const QString& password,
                                      std::function<void(bool, int, const QString&)> callback) {
    const QString traceId = AuditService::generateTraceId();
    m_userRepository->getUserByAccountAsync(account,
        [this, password, callback, account, traceId](std::optional<UserAuthDTO> userOpt) {
            auto context = AuditService::makeContext(QStringLiteral("auth"), QStringLiteral("PasswordLoginUseCase"));
            context.traceId = traceId;
            if (!userOpt.has_value()) {
                audit::AuditEvent event;
                event.eventType = QString::fromLatin1(audit::EventAuthPasswordFailed);
                event.eventLevel = QString::fromLatin1(audit::LevelWarn);
                event.eventResult = QString::fromLatin1(audit::ResultFailed);
                event.reasonCode = QString::fromLatin1(audit::ReasonBadAccount);
                event.message = QStringLiteral("密码登录失败：账户不存在");
                event.metadata = QJsonObject{{QStringLiteral("loginMode"), QStringLiteral("password")},
                                             {QStringLiteral("account"), account}};
                if (m_auditService) {
                    m_auditService->logAsync(context, event);
                }
                callback(false, -1, QString());
                return;
            }
            bool isAuth = CryptoUtils::verifyPassword(userOpt->pwdHash, password);
            context.uid = userOpt->user.uid();
            if (isAuth) {
                audit::AuditEvent event;
                event.eventType = QString::fromLatin1(audit::EventAuthPasswordSuccess);
                event.eventLevel = QString::fromLatin1(audit::LevelInfo);
                event.eventResult = QString::fromLatin1(audit::ResultSuccess);
                event.message = QStringLiteral("密码登录成功");
                event.metadata = QJsonObject{{QStringLiteral("loginMode"), QStringLiteral("password")}};
                if (m_auditService) {
                    m_auditService->logAsync(context, event);
                }
                callback(true, userOpt->user.uid(), userOpt->user.username());
            } else {
                audit::AuditEvent event;
                event.eventType = QString::fromLatin1(audit::EventAuthPasswordFailed);
                event.eventLevel = QString::fromLatin1(audit::LevelWarn);
                event.eventResult = QString::fromLatin1(audit::ResultFailed);
                event.reasonCode = QString::fromLatin1(audit::ReasonBadCredentials);
                event.message = QStringLiteral("密码登录失败：账号或密码错误");
                event.metadata = QJsonObject{{QStringLiteral("loginMode"), QStringLiteral("password")}};
                if (m_auditService) {
                    m_auditService->logAsync(context, event);
                }
                callback(false, -1, QString());
            }
        });
}
