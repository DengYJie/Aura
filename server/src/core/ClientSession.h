#pragma once

#include <QTcpSocket>
#include <QObject>
#include <QByteArray>
#include "Protocol.h"

class ChatServer;

class ClientSession : public QObject {
    Q_OBJECT
public:
    explicit ClientSession(qintptr socketDescriptor, ChatServer* server, QObject *parent = nullptr);
    ~ClientSession() override;

    QString getUserId() const { return m_userId; }
    void sendFrame(Aura::Protocol::MessageType type, const QJsonObject& payload);
    void closeWithError(const QString& reason);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void handleFrame(Aura::Protocol::MessageType type, const QJsonObject& payload);
    void handleAuth(const QJsonObject& payload);
    void handleChat(const QJsonObject& payload);

    QTcpSocket* m_socket;
    ChatServer* m_server;
    QString m_userId;
    QByteArray m_buffer;
};
