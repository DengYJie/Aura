#include "SendMessageUseCase.h"

namespace domain::usecase {

SendMessageUseCase::SendMessageUseCase(std::shared_ptr<repository::IChatRepository> repository)
    : m_repository(std::move(repository))
{}

void SendMessageUseCase::saveMessage(const QString& conversationId, const model::Message& message) const
{
    if (m_repository) {
        m_repository->saveMessage(conversationId, message);
    }
}

void SendMessageUseCase::requestAiStreaming(const QString& conversationId,
                                            const QString& prompt,
                                            const QString& modelName,
                                            ChunkCallback callback) const
{
    if (m_repository) {
        m_repository->requestAiStreaming(conversationId, prompt, modelName, std::move(callback));
    }
}

void SendMessageUseCase::requestAiTask(const QString& conversationId,
                                       const QString& taskType,
                                       const QString& prompt,
                                       const QString& modelName,
                                       ChunkCallback callback) const
{
    if (m_repository) {
        m_repository->requestAiTask(conversationId, taskType, prompt, modelName, std::move(callback));
    }
}

}  // namespace domain::usecase
