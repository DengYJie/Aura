#pragma once
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QList>

class ServerDatabase : public QObject {
    Q_OBJECT
public:
    static ServerDatabase& instance();

    bool init(const QString& dbPath);
    bool saveOfflineMessage(const QString& toUserId, const QString& fromUserId, const QString& content, const QString& messageId, const QString& taskType, const QString& reqId);
    
    struct OfflineMessage {
        QString messageId;
        QString fromUserId;
        QString content;
        QString taskType;
        QString reqId;
        qint64 timestamp;
    };
    
    QList<OfflineMessage> getAndClearOfflineMessages(const QString& userId);

private:
    explicit ServerDatabase(QObject* parent = nullptr);
    ~ServerDatabase() override = default;
};
