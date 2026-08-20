#include "TcpChatClient.h"
#include <QDebug>

TcpChatClient& TcpChatClient::instance() {
    static TcpChatClient inst;
    return inst;
}

TcpChatClient::TcpChatClient(QObject* parent) : QObject(parent) {
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected,    this, &TcpChatClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpChatClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead,    this, &TcpChatClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        // 连接失败也触发重连（e.g. ConnectionRefusedError）
        emit errorOccurred(m_socket->errorString(), QString());
        scheduleReconnect();
    });

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &TcpChatClient::attemptReconnect);
}

TcpChatClient::~TcpChatClient() {
    m_reconnectTimer->stop();
    if (m_socket->isOpen()) {
        m_socket->close();
    }
}

void TcpChatClient::connectToServer(const QString& host, quint16 port) {
    m_host = host;
    m_port = port;
    m_reconnectDelay = 1; // 首次连接重置退避
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        m_socket->connectToHost(host, port);
    }
}

void TcpChatClient::authenticate(const QString& userId) {
    m_userId = userId; // 保存以便重连后自动重认证
    QJsonObject payload;
    payload["userId"] = userId;
    QByteArray frame = Aura::Protocol::createFrame(Aura::Protocol::MessageType::Auth, payload);
    m_socket->write(frame);
    m_socket->flush();
}

bool TcpChatClient::sendMessage(const QString& toUserId,
                                const QString& content,
                                const QString& taskType,
                                const QString& reqId,
                                const QString& messageId) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        qWarning() << "[TCP] sendMessage: not connected, dropping frame";
        return false;
    }
    QJsonObject payload;
    payload["to"] = toUserId;
    payload["content"] = content;
    if (!taskType.isEmpty())  payload["taskType"]  = taskType;
    if (!reqId.isEmpty())     payload["reqId"]     = reqId;
    if (!messageId.isEmpty()) payload["messageId"] = messageId;
    QByteArray frame = Aura::Protocol::createFrame(Aura::Protocol::MessageType::Chat, payload);
    const qint64 bytesWritten = m_socket->write(frame);
    m_socket->flush();
    return bytesWritten > 0;
}

// ── 内部槽 ──────────────────────────────────────────────────────────

void TcpChatClient::onConnected() {
    qDebug() << "[TCP] Connected to" << m_host << m_port;
    m_reconnectDelay = 1;
    m_reconnectTimer->stop();

    // 必须在 emit connected() 之前判断：
    // 首次连接时 m_userId 为空，AppContainer 的单次回调会 authenticate() 并赋值；
    // 重连时 m_userId 已有值，AppContainer 的回调已断开，需要我们自己重认证。
    const bool isReconnect = !m_userId.isEmpty();
    emit connected();

    if (isReconnect) {
        qDebug() << "[TCP] Auto re-authenticating as" << m_userId;
        authenticate(m_userId);
    }
}

void TcpChatClient::onDisconnected() {
    qDebug() << "[TCP] Disconnected";
    emit disconnected();
    scheduleReconnect();
}

void TcpChatClient::scheduleReconnect() {
    if (m_host.isEmpty() || m_port == 0) {
        return; // 从未调用过 connectToServer，不重连
    }
    if (m_reconnectTimer->isActive()) {
        return; // 已经在等待中
    }
    qDebug() << "[TCP] Reconnecting in" << m_reconnectDelay << "s...";
    emit reconnecting(m_reconnectDelay);
    m_reconnectTimer->start(m_reconnectDelay * 1000);

    // 指数退避：1 → 2 → 4 → 8 → 16 → 32 → 32 → ...
    m_reconnectDelay = qMin(m_reconnectDelay * 2, kMaxReconnectDelay);
}

void TcpChatClient::attemptReconnect() {
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->abort(); // 强制清理旧连接
    }
    qDebug() << "[TCP] Attempting reconnect to" << m_host << m_port;
    m_socket->connectToHost(m_host, m_port);
}

// ── 数据接收 ────────────────────────────────────────────────────────

void TcpChatClient::onReadyRead() {
    m_buffer.append(m_socket->readAll());

    int newlineIndex;
    while ((newlineIndex = m_buffer.indexOf('\n')) != -1) {
        QByteArray frameData = m_buffer.left(newlineIndex);
        m_buffer.remove(0, newlineIndex + 1);

        Aura::Protocol::MessageType type;
        QJsonObject payload;
        if (Aura::Protocol::parseFrame(frameData, type, payload)) {
            handleFrame(type, payload);
        } else {
            qWarning() << "[TCP] Failed to parse frame:" << frameData.left(120);
        }
    }
}

void TcpChatClient::handleFrame(Aura::Protocol::MessageType type, const QJsonObject& payload) {
    switch (type) {
        case Aura::Protocol::MessageType::AuthAck:
            emit authStatus(payload["success"].toBool());
            break;
        case Aura::Protocol::MessageType::ChatAck:
            emit chatAck(payload["success"].toBool(),
                         payload["messageId"].toString(),
                         payload["message"].toString(),
                         payload["to"].toString());
            break;
        case Aura::Protocol::MessageType::IncomingChat:
            emit incomingChat(payload["from"].toString(),
                              payload["content"].toString(),
                              payload["timestamp"].toVariant().toLongLong());
            break;
        case Aura::Protocol::MessageType::StreamChunk:
            emit streamChunk(payload["from"].toString(),
                             payload["chunk"].toString(),
                             payload["reqId"].toString());
            break;
        case Aura::Protocol::MessageType::StreamEnd:
            emit streamEnd(payload["from"].toString(), payload["reqId"].toString());
            break;
        case Aura::Protocol::MessageType::Error:
            qWarning() << "[TCP] Server error:" << payload["content"].toString();
            emit errorOccurred(payload["content"].toString(), payload["reqId"].toString());
            break;
        default:
            break;
    }
}
