#include "ChatServer.h"
#include "ClientSession.h"
#include "ServerDatabase.h"
#include "../ai/AiModelRouter.h"
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

ChatServer::ChatServer(const QString& dbPath, QObject *parent) : QTcpServer(parent) {
    QString path = dbPath;
    if (path.isEmpty()) {
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataPath);
        path = dataPath + "/aura_server.db";
    }
    ServerDatabase::instance().init(path);
}

ChatServer::~ChatServer() {
}

bool ChatServer::startServer(const QString& host, quint16 port) {
    QHostAddress addr(host);
    if (addr.isNull()) {
        addr = QHostAddress::Any;
    }
    if (listen(addr, port)) {
        qInfo().noquote() << QStringLiteral("AuraServer listening on %1:%2").arg(host).arg(port);
        return true;
    }
    qCritical().noquote() << QStringLiteral("Failed to bind %1:%2: %3").arg(host).arg(port).arg(errorString());
    return false;
}

void ChatServer::incomingConnection(qintptr socketDescriptor) {
    qDebug().noquote() << QStringLiteral("[TCP] New connection  fd=%1  online=%2").arg(socketDescriptor).arg(m_clients.size());
    new ClientSession(socketDescriptor, this, this);
}

bool ChatServer::registerClient(const QString& userId, ClientSession* session, QString* errorMessage) {
    if (userId.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Empty userId");
        }
        return false;
    }

    if (!session) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid session");
        }
        return false;
    }

    if (m_clients.contains(userId) && m_clients.value(userId) != session) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("User already logged in");
        }
        return false;
    }

    m_clients.insert(userId, session);
    qInfo().noquote() << QStringLiteral("[AUTH] + %1  online=%2").arg(userId).arg(m_clients.size());
    
    // Sync offline messages
    QList<ServerDatabase::OfflineMessage> offlineMsgs = ServerDatabase::instance().getAndClearOfflineMessages(userId);
    for (const auto& msg : offlineMsgs) {
        QJsonObject payload;
        payload["from"] = msg.fromUserId;
        payload["to"] = userId;
        payload["content"] = msg.content;
        payload["messageId"] = msg.messageId;
        if (!msg.taskType.isEmpty()) payload["taskType"] = msg.taskType;
        if (!msg.reqId.isEmpty()) payload["reqId"] = msg.reqId;
        payload["timestamp"] = msg.timestamp;
        session->sendFrame(Aura::Protocol::MessageType::IncomingChat, payload);
    }
    
    if (!offlineMsgs.isEmpty()) {
        qInfo().noquote() << QStringLiteral("[SYNC] Delivered %1 offline msg(s) to %2").arg(offlineMsgs.size()).arg(userId);
    }
    
    return true;
}

void ChatServer::unregisterClient(const QString& userId) {
    m_clients.remove(userId);
    qInfo().noquote() << QStringLiteral("[AUTH] - %1  online=%2").arg(userId).arg(m_clients.size());
}

ClientSession* ChatServer::clientSession(const QString& userId) const
{
    return m_clients.value(userId, nullptr);
}

void ChatServer::routeMessage(const QString& fromUserId,
                              const QString& toUserId,
                              const QString& content,
                              const QString& messageId,
                              const QString& taskType,
                              const QString& reqId) {

    ClientSession* senderSession = clientSession(fromUserId);
    if (!senderSession) {
        qWarning().noquote() << QStringLiteral("[ROUTE] Sender not found: %1").arg(fromUserId);
        return;
    }

    // Check if targeting AI
    if (toUserId.startsWith("ai_")) {
        const QString task = taskType.isEmpty() ? "chat" : taskType;
        qDebug().noquote() << QStringLiteral("[AI] %1 → %2  task=%3").arg(fromUserId, toUserId, task);
        QJsonObject ack = Aura::Protocol::makeAckPayload(true, QStringLiteral("AI request accepted"), messageId);
        ack["to"] = toUserId;
        senderSession->sendFrame(Aura::Protocol::MessageType::ChatAck, ack);
        AiModelRouter::instance().routeMessage(toUserId, content, senderSession, task, reqId);
        return;
    }

    // Human-to-Human routing
    if (m_clients.contains(toUserId)) {
        QJsonObject payload;
        payload["from"] = fromUserId;
        payload["to"] = toUserId;
        payload["content"] = content;
        if (!messageId.isEmpty()) {
            payload["messageId"] = messageId;
        }
        if (!reqId.isEmpty()) {
            payload["reqId"] = reqId;
        }
        payload["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        m_clients[toUserId]->sendFrame(Aura::Protocol::MessageType::IncomingChat, payload);

        qDebug().noquote() << QStringLiteral("[CHAT] %1 → %2  delivered").arg(fromUserId, toUserId);
        QJsonObject ack = Aura::Protocol::makeAckPayload(true, QStringLiteral("Delivered"), messageId);
        ack["to"] = toUserId;
        senderSession->sendFrame(Aura::Protocol::MessageType::ChatAck, ack);
    } else {
        ServerDatabase::instance().saveOfflineMessage(toUserId, fromUserId, content, messageId, taskType, reqId);
        qInfo().noquote() << QStringLiteral("[CHAT] %1 → %2  queued (offline)").arg(fromUserId, toUserId);
        QJsonObject ack = Aura::Protocol::makeAckPayload(true, QStringLiteral("Saved to offline storage"), messageId);
        ack["to"] = toUserId;
        ack["offline"] = true;
        senderSession->sendFrame(Aura::Protocol::MessageType::ChatAck, ack);
    }
}




