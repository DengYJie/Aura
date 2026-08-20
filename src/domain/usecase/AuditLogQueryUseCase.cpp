#include "AuditLogQueryUseCase.h"

AuditLogQueryUseCase::AuditLogQueryUseCase(
    std::shared_ptr<AuditService> auditService,
    std::shared_ptr<BehaviorEvidenceRepository> behaviorEvidenceRepository)
    : m_auditService(std::move(auditService)),
      m_behaviorEvidenceRepository(std::move(behaviorEvidenceRepository)) {}

void AuditLogQueryUseCase::queryLogsAsync(
    const audit::AuditQuery& query,
    std::function<void(AuditLogPage)> callback) const {
  if (!m_auditService) {
    if (callback) {
      callback({});
    }
    return;
  }
  m_auditService->queryRecentLogsAsync(query, std::move(callback));
}

void AuditLogQueryUseCase::getBehaviorEvidenceAsync(
    const QString& eventId,
    std::function<void(std::optional<BehaviorEvidence>)> callback) const {
  if (!m_behaviorEvidenceRepository) {
    if (callback) {
      callback(std::nullopt);
    }
    return;
  }
  m_behaviorEvidenceRepository->getByEventIdAsync(eventId, std::move(callback));
}
