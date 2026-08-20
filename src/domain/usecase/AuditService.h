#pragma once

#include "domain/model/AuditTypes.h"
#include "domain/repository/AuditRepository.h"

#include <memory>

class AuditService {
 public:
  explicit AuditService(std::shared_ptr<AuditRepository> repository);

  static QString generateEventId();
  static QString generateTraceId();

  static audit::AuditContext makeContext(const QString& sourceModule,
                                         const QString& sourceUseCase,
                                         std::optional<int> uid = std::nullopt);

  void logAsync(const audit::AuditContext& context,
                audit::AuditEvent event,
                std::function<void(bool)> callback = {}) const;

  void queryRecentLogsAsync(const audit::AuditQuery& query,
                            std::function<void(AuditLogPage)> callback) const;

  void queryDashboardStatsAsync(const audit::DashboardQuery& query,
                                std::function<void(DashboardStats)> callback) const;

 private:
  std::shared_ptr<AuditRepository> m_repository;
};
