#include "LocalChatDataSource.h"
#include "DatabaseManager.h"

#include <FluentQt/Design.h>

namespace data::local {

using namespace domain::model;

LocalChatDataSource::LocalChatDataSource(const QString& currentUserId)
{
    initSeedData(currentUserId);
}

void LocalChatDataSource::initSeedData(const QString& userId)
{
    m_conversations = {
        {
            QStringLiteral("ai_chat"),
            QStringLiteral("DeepSeek 助手"),
            QStringLiteral("您好！我是您的 AI 智能助手，请问有什么可以帮您？"),
            QStringLiteral(""),
            Typography::Icons::Message,
            QColor("#0078D4"),
            0
        }
    };

    const QString peerId = (userId == "01") ? "02" : "01";
    const QString peerName = QString("User %1").arg(peerId);

    m_conversations.append({
        peerId,
        peerName,
        QStringLiteral(""),
        QStringLiteral(""),
        Typography::Icons::Contact,
        QColor("#107C41"),
        0
    });

    // 如果 SQLite 中的会话列表为空，将当前用户的初始会话持久化写入 SQLite
    auto existingConvs = DatabaseManager::instance().getConversations();
    if (existingConvs.isEmpty()) {
        for (const auto& conv : m_conversations) {
            DatabaseManager::instance().saveConversation(conv);
        }
    }
}

QList<Conversation> LocalChatDataSource::loadConversations() const
{
    auto convs = DatabaseManager::instance().getConversations();
    if (convs.isEmpty()) {
        return m_conversations;
    }
    return convs;
}

QVector<Message> LocalChatDataSource::loadMessages(const QString& conversationId) const
{
    return DatabaseManager::instance().getMessages(conversationId);
}

void LocalChatDataSource::saveMessage(const QString& conversationId, const Message& message)
{
    // Save to SQLite
    DatabaseManager::instance().saveMessage(conversationId, message);

    // Update conversation lastMessage & timestamp in DB and memory
    for (auto& conv : m_conversations) {
        if (conv.id == conversationId) {
            conv.lastMessage = message.renderMode == MessageRenderMode::Image
                ? QStringLiteral("[图片]")
                : message.rawText;
            conv.time = message.timestamp;
            DatabaseManager::instance().saveConversation(conv);
            break;
        }
    }
}

}  // namespace data::local
