#include "ServerDatabase.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

ServerDatabase& ServerDatabase::instance() {
    static ServerDatabase inst;
    return inst;
}

ServerDatabase::ServerDatabase(QObject* parent) : QObject(parent) {}

bool ServerDatabase::init(const QString& dbPath) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "server_db");
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qCritical().noquote() << QStringLiteral("[DB] Failed to open: %1 — %2").arg(dbPath, db.lastError().text());
        return false;
    }

    QSqlQuery query(db);
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS offline_messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "messageId TEXT, "
        "toUserId TEXT, "
        "fromUserId TEXT, "
        "content TEXT, "
        "taskType TEXT, "
        "reqId TEXT, "
        "timestamp INTEGER"
        ")"
    );

    if (!success) {
        qCritical().noquote() << QStringLiteral("[DB] Failed to create table: %1").arg(query.lastError().text());
        return false;
    }
    qInfo().noquote() << QStringLiteral("[DB] Opened: %1").arg(dbPath);
    return true;
}

bool ServerDatabase::saveOfflineMessage(const QString& toUserId, const QString& fromUserId, const QString& content, const QString& messageId, const QString& taskType, const QString& reqId) {
    QSqlDatabase db = QSqlDatabase::database("server_db");
    if (!db.isOpen()) return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO offline_messages (messageId, toUserId, fromUserId, content, taskType, reqId, timestamp) "
                  "VALUES (:messageId, :toUserId, :fromUserId, :content, :taskType, :reqId, :timestamp)");
    query.bindValue(":messageId", messageId);
    query.bindValue(":toUserId", toUserId);
    query.bindValue(":fromUserId", fromUserId);
    query.bindValue(":content", content);
    query.bindValue(":taskType", taskType);
    query.bindValue(":reqId", reqId);
    query.bindValue(":timestamp", QDateTime::currentMSecsSinceEpoch());

    if (!query.exec()) {
        qWarning() << "Failed to save offline message:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<ServerDatabase::OfflineMessage> ServerDatabase::getAndClearOfflineMessages(const QString& userId) {
    QList<OfflineMessage> results;
    QSqlDatabase db = QSqlDatabase::database("server_db");
    if (!db.isOpen()) return results;

    db.transaction();
    QSqlQuery query(db);
    query.prepare("SELECT messageId, fromUserId, content, taskType, reqId, timestamp FROM offline_messages WHERE toUserId = :toUserId ORDER BY timestamp ASC");
    query.bindValue(":toUserId", userId);
    
    if (query.exec()) {
        while (query.next()) {
            OfflineMessage msg;
            msg.messageId = query.value(0).toString();
            msg.fromUserId = query.value(1).toString();
            msg.content = query.value(2).toString();
            msg.taskType = query.value(3).toString();
            msg.reqId = query.value(4).toString();
            msg.timestamp = query.value(5).toLongLong();
            results.append(msg);
        }
    }

    if (!results.isEmpty()) {
        QSqlQuery deleteQuery(db);
        deleteQuery.prepare("DELETE FROM offline_messages WHERE toUserId = :toUserId");
        deleteQuery.bindValue(":toUserId", userId);
        deleteQuery.exec();
    }
    
    db.commit();
    return results;
}

