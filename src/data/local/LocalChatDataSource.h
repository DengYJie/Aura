#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QVector>

#include "domain/model/Conversation.h"
#include "domain/model/Message.h"

namespace data::local {

class LocalChatDataSource {
public:
    explicit LocalChatDataSource(const QString& currentUserId);

    QList<domain::model::Conversation> loadConversations() const;
    QVector<domain::model::Message> loadMessages(const QString& conversationId) const;
    void saveMessage(const QString& conversationId, const domain::model::Message& message);

private:
    void initSeedData(const QString& userId);

    QList<domain::model::Conversation> m_conversations;
    QHash<QString, QVector<domain::model::Message>> m_messageStore;
};

}  // namespace data::local
