#include "SecurityOverviewUseCase.h"

SecurityOverviewUseCase::SecurityOverviewUseCase(std::shared_ptr<AuditService> auditService)
    : m_auditService(std::move(auditService)) {}

void SecurityOverviewUseCase::loadStatsAsync(
    const audit::DashboardQuery& query,
    std::function<void(DashboardStats)> callback) const {
  if (!m_auditService) {
    if (callback) {
      callback({});
    }
    return;
  }
  m_auditService->queryDashboardStatsAsync(query, std::move(callback));
}
