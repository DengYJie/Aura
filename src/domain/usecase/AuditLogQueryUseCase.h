#pragma once

#include "domain/entity/BehaviorEvidence.h"
#include "domain/repository/BehaviorEvidenceRepository.h"
#include "domain/usecase/AuditService.h"

#include <memory>
#include <optional>

class AuditLogQueryUseCase {
 public:
  AuditLogQueryUseCase(std::shared_ptr<AuditService> auditService,
                       std::shared_ptr<BehaviorEvidenceRepository> behaviorEvidenceRepository);

  void queryLogsAsync(const audit::AuditQuery& query,
                      std::function<void(AuditLogPage)> callback) const;

  void getBehaviorEvidenceAsync(
      const QString& eventId,
      std::function<void(std::optional<BehaviorEvidence>)> callback) const;

 private:
  std::shared_ptr<AuditService> m_auditService;
  std::shared_ptr<BehaviorEvidenceRepository> m_behaviorEvidenceRepository;
};
