#pragma once

#include <QTcpServer>
#include <QHash>
#include <QObject>
#include <QString>

class ClientSession;

class ChatServer : public QTcpServer {
    Q_OBJECT
public:
    explicit ChatServer(const QString& dbPath = QString(), QObject *parent = nullptr);
    ~ChatServer() override;

    bool startServer(const QString& host = QStringLiteral("0.0.0.0"), quint16 port = 8080);

    // Routing
    bool registerClient(const QString& userId, ClientSession* session, QString* errorMessage = nullptr);
    void unregisterClient(const QString& userId);
    void routeMessage(const QString& fromUserId,
                      const QString& toUserId,
                      const QString& content,
                      const QString& messageId,
                      const QString& taskType = QString(),
                      const QString& reqId = QString());
    ClientSession* clientSession(const QString& userId) const;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QHash<QString, ClientSession*> m_clients; // userId -> session
};
