#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include "../../../shared/Protocol.h"
#include "../../domain/model/Message.h"

class TcpChatClient : public QObject {
    Q_OBJECT
public:
    static TcpChatClient& instance();

    void connectToServer(const QString& host, quint16 port);
    void authenticate(const QString& userId);
    bool sendMessage(const QString& toUserId,
                     const QString& content,
                     const QString& taskType = QString(),
                     const QString& reqId = QString(),
                     const QString& messageId = QString());

    bool isConnected() const {
        return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
    }

signals:
    void connected();
    void disconnected();
    void authStatus(bool success);
    void chatAck(bool success, const QString& messageId, const QString& message, const QString& toUserId);
    void incomingChat(const QString& fromUserId, const QString& content, qint64 timestamp);
    void streamChunk(const QString& fromUserId, const QString& chunk, const QString& reqId);
    void streamEnd(const QString& fromUserId, const QString& reqId);
    void errorOccurred(const QString& errorMsg, const QString& reqId);
    // 重连状态通知（供 UI 显示"正在重连中 (3s)..."）
    void reconnecting(int delaySeconds);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void attemptReconnect();

private:
    explicit TcpChatClient(QObject* parent = nullptr);
    ~TcpChatClient() override;

    void handleFrame(Aura::Protocol::MessageType type, const QJsonObject& payload);
    void scheduleReconnect();

    QTcpSocket*  m_socket;
    QByteArray   m_buffer;

    // 重连状态
    QString      m_host;
    quint16      m_port           = 0;
    QString      m_userId;                  // 认证成功后保存，用于重连后自动重认证
    QTimer*      m_reconnectTimer = nullptr;
    int          m_reconnectDelay = 1;      // 当前退避间隔（秒），初始 1s
    static constexpr int kMaxReconnectDelay = 32; // 最大 32s
};
