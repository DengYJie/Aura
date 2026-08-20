#pragma once

#include "domain/usecase/AuditService.h"
#include "domain/repository/UserRepository.h"
#include <functional>
#include <memory>

/**
 * @brief 密码重置用例
 */
class ResetPasswordUseCase {
public:
    ResetPasswordUseCase(std::shared_ptr<UserRepository> userRepository,
                         std::shared_ptr<AuditService> auditService);

    void resetPasswordAsync(const QString& account, const QString& newPassword,
                            std::function<void(bool, QString)> callback);

private:
    std::shared_ptr<UserRepository> m_userRepository;
    std::shared_ptr<AuditService> m_auditService;
};
