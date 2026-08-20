#include "OpenAICompatibleClient.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

OpenAICompatibleClient::OpenAICompatibleClient(const QString& apiKey,
                                               const QString& baseUrl,
                                               const QString& modelName,
                                               double temperature,
                                               int maxTokens,
                                               QObject* parent)
    : IAiProvider(parent),
      m_apiKey(apiKey),
      m_baseUrl(baseUrl),
      m_modelName(modelName),
      m_temperature(temperature),
      m_maxTokens(maxTokens),
      m_reply(nullptr)
{
    m_manager = new QNetworkAccessManager(this);
}

void OpenAICompatibleClient::requestChatStreaming(const QJsonArray& messages) {
    if (m_apiKey.isEmpty() && m_baseUrl.startsWith("https://")) {
        // 真实原因只记录在服务端日志，不向客户端暴露配置细节
        qCritical().noquote() << QStringLiteral("[AI] %1: API key not configured").arg(m_modelName);
        emit errorOccurred(QStringLiteral("AI service unavailable."));
        return;
    }

    QUrl url(m_baseUrl + "/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    // SSE 流式传输建议使用 HTTP/1.1，避免部分代理或网关在 HTTP/2 下出现 stream reset / protocol error
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    if (!m_apiKey.isEmpty() && m_apiKey != "ollama") {
        request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());
    }

    QJsonObject payload;
    payload["model"] = m_modelName;
    payload["messages"] = messages;
    payload["stream"] = true;
    if (m_temperature >= 0.0) {
        payload["temperature"] = m_temperature;
    }
    if (m_maxTokens > 0) {
        payload["max_tokens"] = m_maxTokens;
    }

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    m_streamBuffer.clear();
    m_reply = m_manager->post(request, data);
    connect(m_reply, &QNetworkReply::readyRead, this, &OpenAICompatibleClient::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &OpenAICompatibleClient::onFinished);
}

void OpenAICompatibleClient::onReadyRead() {
    if (!m_reply) return;

    m_streamBuffer.append(m_reply->readAll());

    int newlineIndex = -1;
    while ((newlineIndex = m_streamBuffer.indexOf('\n')) != -1) {
        QByteArray line = m_streamBuffer.left(newlineIndex).trimmed();
        m_streamBuffer.remove(0, newlineIndex + 1);
        processSseLine(line);
    }
}

void OpenAICompatibleClient::processSseLine(const QByteArray& line)
{
    if (!line.startsWith("data: ")) {
        return;
    }

    QByteArray jsonData = line.mid(6).trimmed();
    if (jsonData == "[DONE]") {
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    const QJsonArray choices = obj["choices"].toArray();
    if (choices.isEmpty()) {
        return;
    }

    const QJsonObject choice = choices[0].toObject();
    const QJsonObject delta = choice["delta"].toObject();
    if (delta.contains("content")) {
        emit streamChunkReceived(delta["content"].toString());
        return;
    }

    const QJsonObject message = choice["message"].toObject();
    if (message.contains("content")) {
        emit streamChunkReceived(message["content"].toString());
    }
}

void OpenAICompatibleClient::onFinished() {
    if (!m_reply) return;

    if (!m_streamBuffer.trimmed().isEmpty()) {
        processSseLine(m_streamBuffer.trimmed());
        m_streamBuffer.clear();
    }

    if (m_reply->error() != QNetworkReply::NoError) {
        qCritical().noquote() << QStringLiteral("[AI] %1 network error: %2").arg(m_modelName, m_reply->errorString());
        emit errorOccurred(QStringLiteral("AI service unavailable."));
    } else {
        emit streamFinished();
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

