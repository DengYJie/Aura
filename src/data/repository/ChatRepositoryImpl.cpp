#include "ChatRepositoryImpl.h"
#include "../remote/TcpChatClient.h"
#include "../local/DatabaseManager.h"
#include "../../utils/CryptoUtils.h"
#include <QDateTime>

namespace data::repository {

ChatRepositoryImpl::ChatRepositoryImpl(std::shared_ptr<local::LocalChatDataSource> localDataSource,
                                       std::shared_ptr<remote::RemoteAiDataSource> remoteDataSource,
                                       QObject* parent)
    : domain::repository::IChatRepository(parent)
    , m_localDataSource(std::move(localDataSource))
    , m_remoteDataSource(std::move(remoteDataSource))
{
    connect(&TcpChatClient::instance(), &TcpChatClient::incomingChat, this, [this](const QString& from, const QString& content, qint64 timestamp) {
        domain::model::Message msg;
        msg.id = QString("peer-%1").arg(timestamp);
        msg.senderType = domain::model::MessageSenderType::Peer;
        msg.senderName = from;
        
        // E2E Decryption
        msg.rawText = CryptoUtils::decrypt(content);
        
        msg.timestamp = QDateTime::fromMSecsSinceEpoch(timestamp).toString("hh:mm");
        if (msg.rawText.startsWith("data:image/")) {
            msg.renderMode = domain::model::MessageRenderMode::Image;
        } else {
            msg.renderMode = domain::model::MessageRenderMode::PlainText;
        }
        
        // Save to DB
        saveMessage(from, msg); // from is the conversationId for 1-1 chat
        emit messageReceived(from, msg);
    });

    connect(&TcpChatClient::instance(), &TcpChatClient::chatAck, this, [this](bool success, const QString& messageId, const QString& /*message*/, const QString& toUserId) {
        const auto status = success ? domain::model::MessageDeliveryStatus::Sent : domain::model::MessageDeliveryStatus::Failed;
        DatabaseManager::instance().updateMessageDeliveryStatus(messageId, status);
        emit messageDeliveryStatusChanged(toUserId, messageId, status);
    });
}

QList<domain::model::Conversation> ChatRepositoryImpl::getConversations()
{
    if (!m_localDataSource) {
        return {};
    }
    return m_localDataSource->loadConversations();
}

QVector<domain::model::Message> ChatRepositoryImpl::getMessages(const QString& conversationId)
{
    if (!m_localDataSource) {
        return {};
    }
    return m_localDataSource->loadMessages(conversationId);
}

void ChatRepositoryImpl::saveMessage(const QString& conversationId, const domain::model::Message& message)
{
    if (m_localDataSource) {
        m_localDataSource->saveMessage(conversationId, message);
    }
    // If it's a human-to-human chat and from me, route it via TCP
    if (message.senderType == domain::model::MessageSenderType::Self && !conversationId.startsWith("ai_")) {
        // E2E Encryption
        const QString encryptedContent = CryptoUtils::encrypt(message.rawText);
        const bool sentOk = TcpChatClient::instance().sendMessage(conversationId, encryptedContent, QString(), QString(), message.id);
        if (!sentOk) {
            DatabaseManager::instance().updateMessageDeliveryStatus(message.id, domain::model::MessageDeliveryStatus::Failed);
            emit messageDeliveryStatusChanged(conversationId, message.id, domain::model::MessageDeliveryStatus::Failed);
        }
    }
}

void ChatRepositoryImpl::requestAiStreaming(const QString& /*conversationId*/,
                                            const QString& prompt,
                                            const QString& modelName,
                                            StreamChunkCallback callback)
{
    if (m_remoteDataSource) {
        m_remoteDataSource->streamGenerate(prompt, modelName, std::move(callback), QStringLiteral("chat"));
    }
}

void ChatRepositoryImpl::requestAiTask(const QString& conversationId,
                                       const QString& taskType,
                                       const QString& prompt,
                                       const QString& modelName,
                                       StreamChunkCallback callback)
{
    if (!m_remoteDataSource) {
        return;
    }

    QString payload = prompt;
    if (taskType == QStringLiteral("sentiment") || taskType == QStringLiteral("suggest")) {
        payload = buildRecentHumanContext(conversationId, 6);
    }

    if (payload.trimmed().isEmpty()) {
        if (callback) {
            callback(QString(), true);
        }
        return;
    }

    m_remoteDataSource->streamGenerate(payload, modelName, std::move(callback), taskType);
}

QString ChatRepositoryImpl::buildRecentHumanContext(const QString& conversationId,
                                                    int maxMessages) const
{
    if (!m_localDataSource) {
        return {};
    }

    const QVector<domain::model::Message> messages = m_localDataSource->loadMessages(conversationId);
    if (messages.isEmpty() || messages.last().senderType != domain::model::MessageSenderType::Peer) {
        return {};
    }

    QStringList lines;
    int kept = 0;
    for (auto it = messages.crbegin(); it != messages.crend() && kept < maxMessages; ++it) {
        const auto& msg = *it;
        if (msg.senderType != domain::model::MessageSenderType::Self &&
            msg.senderType != domain::model::MessageSenderType::Peer) {
            continue;
        }
        const QString speaker = msg.senderType == domain::model::MessageSenderType::Self
            ? QStringLiteral("self")
            : QStringLiteral("other");
        lines.prepend(QStringLiteral("%1: %2").arg(speaker, msg.rawText));
        ++kept;
    }
    return lines.join('\n');
}

}  // namespace data::repository
