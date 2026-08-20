#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QColor>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::DatabaseManager(QObject* parent) : QObject(parent) {
}

DatabaseManager::~DatabaseManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::init(const QString& dbName) {
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        m_db = QSqlDatabase::database("qt_sql_default_connection");
        if (m_db.isOpen()) {
            m_db.close();
        }
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE");
    }

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    m_db.setDatabaseName(dataDir + "/" + dbName);

    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    return createTables();
}

bool DatabaseManager::createTables() {
    QSqlQuery query(m_db);
    
    // Create Messages table
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS Messages ("
        "id TEXT PRIMARY KEY, "
        "conversationId TEXT, "
        "senderType INTEGER, "
        "senderName TEXT, "
        "timestamp TEXT, "
        "rawText TEXT, "
        "renderMode INTEGER, "
        "sentimentTag INTEGER, "
        "deliveryStatus INTEGER, "
        "isStreaming INTEGER)"
    );

    if (!success) {
        qCritical() << "Failed to create Messages table:" << query.lastError().text();
        return false;
    }

    // Create Conversations table
    success = query.exec(
        "CREATE TABLE IF NOT EXISTS Conversations ("
        "id TEXT PRIMARY KEY, "
        "name TEXT, "
        "lastMessage TEXT, "
        "time TEXT, "
        "glyph TEXT, "
        "avatarBg TEXT, "
        "unreadCount INTEGER)"
    );

    if (!success) {
        qCritical() << "Failed to create Conversations table:" << query.lastError().text();
    }

    return success;
}

bool DatabaseManager::saveMessage(const QString& conversationId, const domain::model::Message& msg) {
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO Messages (id, conversationId, senderType, senderName, timestamp, rawText, renderMode, sentimentTag, deliveryStatus, isStreaming) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(msg.id);
    query.addBindValue(conversationId);
    query.addBindValue(static_cast<int>(msg.senderType));
    query.addBindValue(msg.senderName);
    query.addBindValue(msg.timestamp);
    query.addBindValue(msg.rawText);
    query.addBindValue(static_cast<int>(msg.renderMode));
    query.addBindValue(static_cast<int>(msg.sentimentTag));
    query.addBindValue(static_cast<int>(msg.deliveryStatus));
    query.addBindValue(msg.isStreaming ? 1 : 0);

    if (!query.exec()) {
        qWarning() << "Failed to save message:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::updateMessageDeliveryStatus(const QString& messageId, domain::model::MessageDeliveryStatus status) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE Messages SET deliveryStatus = ? WHERE id = ?");
    query.addBindValue(static_cast<int>(status));
    query.addBindValue(messageId);
    return query.exec();
}

QVector<domain::model::Message> DatabaseManager::getMessages(const QString& conversationId) {
    QVector<domain::model::Message> msgs;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, senderType, senderName, timestamp, rawText, renderMode, sentimentTag, deliveryStatus, isStreaming FROM Messages "
                  "WHERE conversationId = ? ORDER BY timestamp ASC");
    query.addBindValue(conversationId);

    if (query.exec()) {
        while (query.next()) {
            domain::model::Message msg;
            msg.id = query.value(0).toString();
            msg.senderType = static_cast<domain::model::MessageSenderType>(query.value(1).toInt());
            msg.senderName = query.value(2).toString();
            msg.timestamp = query.value(3).toString();
            msg.rawText = query.value(4).toString();
            msg.renderMode = static_cast<domain::model::MessageRenderMode>(query.value(5).toInt());
            msg.sentimentTag = static_cast<domain::model::MessageSentimentTag>(query.value(6).toInt());
            msg.deliveryStatus = static_cast<domain::model::MessageDeliveryStatus>(query.value(7).toInt());
            msg.isStreaming = query.value(8).toInt() == 1;
            msgs.append(msg);
        }
    }
    return msgs;
}

bool DatabaseManager::saveConversation(const domain::model::Conversation& conv) {
    QSqlQuery query(m_db);
    query.prepare("INSERT OR REPLACE INTO Conversations (id, name, lastMessage, time, glyph, avatarBg, unreadCount) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(conv.id);
    query.addBindValue(conv.name);
    query.addBindValue(conv.lastMessage);
    query.addBindValue(conv.time);
    query.addBindValue(conv.glyph);
    query.addBindValue(conv.avatarBg.name());
    query.addBindValue(conv.unreadCount);

    return query.exec();
}

QList<domain::model::Conversation> DatabaseManager::getConversations() {
    QList<domain::model::Conversation> convs;
    QSqlQuery query(m_db);
    // Real app might sort by a sortable timestamp, but we just select for now
    query.prepare("SELECT id, name, lastMessage, time, glyph, avatarBg, unreadCount FROM Conversations");
    
    if (query.exec()) {
        while (query.next()) {
            domain::model::Conversation conv;
            conv.id = query.value(0).toString();
            conv.name = query.value(1).toString();
            conv.lastMessage = query.value(2).toString();
            conv.time = query.value(3).toString();
            conv.glyph = query.value(4).toString();
            conv.avatarBg = QColor(query.value(5).toString());
            conv.unreadCount = query.value(6).toInt();
            convs.append(conv);
        }
    }
    return convs;
}
