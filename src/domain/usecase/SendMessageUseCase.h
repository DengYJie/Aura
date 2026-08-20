#pragma once

#include <functional>
#include <memory>
#include <QString>

#include "../model/Message.h"
#include "../repository/IChatRepository.h"

namespace domain::usecase {

class SendMessageUseCase {
public:
    explicit SendMessageUseCase(std::shared_ptr<repository::IChatRepository> repository);

    // 存储用户消息
    void saveMessage(const QString& conversationId, const model::Message& message) const;

    // 发起 AI 流式推理请求
    using ChunkCallback = std::function<void(const QString& chunkText, bool isFinished)>;
    void requestAiStreaming(const QString& conversationId,
                            const QString& prompt,
                            const QString& modelName,
                            ChunkCallback callback) const;
    void requestAiTask(const QString& conversationId,
                       const QString& taskType,
                       const QString& prompt,
                       const QString& modelName,
                       ChunkCallback callback) const;

private:
    std::shared_ptr<repository::IChatRepository> m_repository;
};

}  // namespace domain::usecase
