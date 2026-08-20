#pragma once

#include <QString>
#include <QSqlDatabase>
#include <QObject>
#include <QList>
#include <QVector>
#include "../../domain/model/Message.h"
#include "../../domain/model/Conversation.h"

class DatabaseManager : public QObject {
    Q_OBJECT
public:
    static DatabaseManager& instance();

    bool init(const QString& dbName = "aura_chat.db");

    // Operations
    bool saveMessage(const QString& conversationId, const domain::model::Message& msg);
    bool updateMessageDeliveryStatus(const QString& messageId, domain::model::MessageDeliveryStatus status);
    QVector<domain::model::Message> getMessages(const QString& conversationId);

    bool saveConversation(const domain::model::Conversation& conv);
    QList<domain::model::Conversation> getConversations();

private:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager() override;

    bool createTables();

    QSqlDatabase m_db;
};
