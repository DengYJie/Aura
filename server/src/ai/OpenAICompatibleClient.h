#pragma once
#include "IAiProvider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

class OpenAICompatibleClient : public IAiProvider {
    Q_OBJECT
public:
    explicit OpenAICompatibleClient(const QString& apiKey,
                                    const QString& baseUrl,
                                    const QString& modelName,
                                    double temperature = 0.7,
                                    int maxTokens = 2048,
                                    QObject* parent = nullptr);

    // 从接收单一字符串 prompt 改为接收结构化的 messages 数组
    void requestChatStreaming(const QJsonArray& messages);
    QString providerName() const override { return m_modelName; }

private slots:
    void onReadyRead();
    void onFinished();

private:
    void processSseLine(const QByteArray& line);

    QString m_apiKey;
    QString m_baseUrl;
    QString m_modelName;
    double  m_temperature;
    int     m_maxTokens;
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_reply;
    QByteArray m_streamBuffer;
};
