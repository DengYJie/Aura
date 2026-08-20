#pragma once

#include <functional>
#include <QList>
#include <QVector>
#include <QString>

#include "../model/Conversation.h"
#include "../model/Message.h"

#include <QObject>
#include <QString>

namespace domain::repository {

class IChatRepository : public QObject {
    Q_OBJECT
public:
    explicit IChatRepository(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IChatRepository() = default;

signals:
    void messageReceived(const QString& conversationId, const model::Message& message);
    void messageDeliveryStatusChanged(const QString& conversationId, const QString& messageId, model::MessageDeliveryStatus status);

public:

    // 获取所有会话列表
    virtual QList<model::Conversation> getConversations() = 0;

    // 获取指定会话的历史消息
    virtual QVector<model::Message> getMessages(const QString& conversationId) = 0;

    // 存储/更新本地消息
    virtual void saveMessage(const QString& conversationId, const model::Message& message) = 0;

    // 发送消息并订阅 AI 流式响应
    using StreamChunkCallback = std::function<void(const QString& chunkText, bool isFinished)>;
    virtual void requestAiStreaming(const QString& conversationId,
                                    const QString& prompt,
                                    const QString& modelName,
                                    StreamChunkCallback callback) = 0;
    virtual void requestAiTask(const QString& conversationId,
                               const QString& taskType,
                               const QString& prompt,
                               const QString& modelName,
                               StreamChunkCallback callback) = 0;
};

}  // namespace domain::repository
