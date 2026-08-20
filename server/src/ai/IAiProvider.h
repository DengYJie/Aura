#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>

class IAiProvider : public QObject {
    Q_OBJECT
public:
    explicit IAiProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAiProvider() = default;

    virtual void requestChatStreaming(const QJsonArray& messages) = 0;
    virtual QString providerName() const = 0;
    
signals:
    void streamChunkReceived(const QString& text);
    void streamFinished();
    void errorOccurred(const QString& errorMessage);
};
