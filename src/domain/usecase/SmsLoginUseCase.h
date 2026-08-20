#pragma once

#include "domain/usecase/AuditService.h"
#include "domain/repository/UserRepository.h"
#include <functional>
#include <memory>

/**
 * @brief 验证码登录用例
 */
class SmsLoginUseCase {
public:
    SmsLoginUseCase(std::shared_ptr<UserRepository> userRepository,
                    std::shared_ptr<AuditService> auditService);

    void smsLoginAsync(const QString& account, const QString& code,
                       std::function<void(bool success, int uid, const QString& username, const QString& message)> callback);

private:
    std::shared_ptr<UserRepository> m_userRepository;
    std::shared_ptr<AuditService> m_auditService;
};
