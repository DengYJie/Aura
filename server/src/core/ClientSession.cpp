#include "ClientSession.h"
#include "ChatServer.h"
#include <QDebug>
#include <QUuid>

ClientSession::ClientSession(qintptr socketDescriptor, ChatServer* server, QObject *parent)
    : QObject(parent), m_server(server)
{
    m_socket = new QTcpSocket(this);
    if (m_socket->setSocketDescriptor(socketDescriptor)) {
        connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
        connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
    } else {
        deleteLater();
    }
}

ClientSession::~ClientSession() {
    if (!m_userId.isEmpty()) {
        m_server->unregisterClient(m_userId);
    }
}

void ClientSession::sendFrame(Aura::Protocol::MessageType type, const QJsonObject& payload) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray frame = Aura::Protocol::createFrame(type, payload);
        m_socket->write(frame);
        m_socket->flush();
    }
}

void ClientSession::closeWithError(const QString& reason)
{
    QJsonObject payload;
    payload["content"] = reason;
    sendFrame(Aura::Protocol::MessageType::Error, payload);
    m_socket->disconnectFromHost();
}

void ClientSession::onReadyRead() {
    m_buffer.append(m_socket->readAll());
    
    // Process all frames separated by newline
    int newlineIndex;
    while ((newlineIndex = m_buffer.indexOf('\n')) != -1) {
        QByteArray frameData = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex + 1);

        Aura::Protocol::MessageType type;
        QJsonObject payload;
        if (Aura::Protocol::parseFrame(frameData, type, payload)) {
            handleFrame(type, payload);
        } else {
            qWarning() << "Failed to parse frame:" << frameData;
        }
    }
}

void ClientSession::onDisconnected() {
    const QString who = m_userId.isEmpty() ? QStringLiteral("(anonymous)") : m_userId;
    qInfo().noquote() << QStringLiteral("[TCP] Disconnected: %1").arg(who);
    deleteLater();
}

void ClientSession::handleFrame(Aura::Protocol::MessageType type, const QJsonObject& payload) {
    switch (type) {
        case Aura::Protocol::MessageType::Auth:
            handleAuth(payload);
            break;
        case Aura::Protocol::MessageType::Chat:
            handleChat(payload);
            break;
        default:
            qWarning().noquote() << QStringLiteral("[FRAME] Unknown type %1 from %2").arg(static_cast<int>(type)).arg(m_userId);
            break;
    }
}

void ClientSession::handleAuth(const QJsonObject& payload) {
    QString userId = payload["userId"].toString();
    if (userId.isEmpty()) {
        qWarning().noquote() << QStringLiteral("[AUTH] Rejected: empty userId from fd=%1").arg(m_socket->socketDescriptor());
        sendFrame(Aura::Protocol::MessageType::AuthAck,
                  Aura::Protocol::makeAckPayload(false, QStringLiteral("Empty userId")));
        return;
    }

    QString errorMessage;
    if (!m_server->registerClient(userId, this, &errorMessage)) {
        qWarning().noquote() << QStringLiteral("[AUTH] Rejected: user=%1 reason=%2").arg(userId, errorMessage);
        sendFrame(Aura::Protocol::MessageType::AuthAck,
                  Aura::Protocol::makeAckPayload(false, errorMessage));
        return;
    }

    m_userId = userId;
    sendFrame(Aura::Protocol::MessageType::AuthAck,
              Aura::Protocol::makeAckPayload(true, QStringLiteral("Authenticated")));
}

void ClientSession::handleChat(const QJsonObject& payload) {
    if (m_userId.isEmpty()) {
        qWarning().noquote() << QStringLiteral("[CHAT] Unauthenticated client tried to send chat (fd=%1)").arg(m_socket->socketDescriptor());
        sendFrame(Aura::Protocol::MessageType::Error,
                  Aura::Protocol::makeAckPayload(false, QStringLiteral("Authenticate first")));
        return;
    }

    QString toUserId = payload["to"].toString();
    QString content = payload["content"].toString();
    QString messageId = payload["messageId"].toString();
    QString taskType = payload["taskType"].toString();
    QString reqId = payload["reqId"].toString();
    bool isEncrypted = payload["isEncrypted"].toBool();

    if (toUserId.isEmpty() || content.isEmpty()) {
        qWarning().noquote() << QStringLiteral("[CHAT] Missing 'to' or 'content' from user=%1").arg(m_userId);
        sendFrame(Aura::Protocol::MessageType::ChatAck,
                  Aura::Protocol::makeAckPayload(false, QStringLiteral("Missing target or content"), messageId));
        return;
    }

    if (messageId.isEmpty()) {
        messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    if (isEncrypted && toUserId.startsWith("ai_")) {
        qWarning().noquote() << QStringLiteral("[CHAT] %1 tried to send encrypted msg to AI — blocked").arg(m_userId);
        sendFrame(Aura::Protocol::MessageType::Error,
                  Aura::Protocol::makeAckPayload(false, QStringLiteral("Cannot send encrypted messages to AI"), messageId));
        return;
    }

    m_server->routeMessage(m_userId, toUserId, content, messageId, taskType, reqId);
}

