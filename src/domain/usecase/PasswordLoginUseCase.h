#pragma once

#include "domain/usecase/AuditService.h"
#include "domain/repository/UserRepository.h"
#include <functional>
#include <memory>

/**
 * @brief 密码登录用例
 */
class PasswordLoginUseCase {
public:
    PasswordLoginUseCase(std::shared_ptr<UserRepository> userRepository,
                         std::shared_ptr<AuditService> auditService);

    void loginAsync(const QString& account, const QString& password,
                    std::function<void(bool success, int uid, const QString& username)> callback);

private:
    std::shared_ptr<UserRepository> m_userRepository;
    std::shared_ptr<AuditService> m_auditService;
};
