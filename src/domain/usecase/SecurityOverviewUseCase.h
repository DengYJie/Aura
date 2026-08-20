#pragma once

#include "domain/usecase/AuditService.h"

#include <memory>

class SecurityOverviewUseCase {
 public:
  explicit SecurityOverviewUseCase(std::shared_ptr<AuditService> auditService);

  void loadStatsAsync(const audit::DashboardQuery& query,
                      std::function<void(DashboardStats)> callback) const;

 private:
  std::shared_ptr<AuditService> m_auditService;
};
