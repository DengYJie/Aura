#include "RemoteAiDataSource.h"
#include "TcpChatClient.h"
#include <QDebug>
#include <QUuid>

namespace data::remote {

RemoteAiDataSource::RemoteAiDataSource(QObject* parent)
    : QObject(parent)
{
    connect(&TcpChatClient::instance(), &TcpChatClient::streamChunk, this, [this](const QString&, const QString& chunk, const QString& reqId) {
        if (reqId.isEmpty() || !m_callbacks.contains(reqId)) {
            return;
        }
        m_accumulators[reqId] += chunk;
        m_callbacks[reqId](m_accumulators[reqId], false);
    });

    connect(&TcpChatClient::instance(), &TcpChatClient::streamEnd, this, [this](const QString&, const QString& reqId) {
        if (reqId.isEmpty() || !m_callbacks.contains(reqId)) {
            return;
        }
        m_callbacks[reqId](m_accumulators[reqId], true);
        m_callbacks.remove(reqId);
        m_accumulators.remove(reqId);
    });

    connect(&TcpChatClient::instance(), &TcpChatClient::errorOccurred, this, [this](const QString& errorMsg, const QString& reqId) {
        if (reqId.isEmpty() || !m_callbacks.contains(reqId)) {
            return;
        }
        const QString msg = errorMsg.isEmpty() ? QStringLiteral("AI 服务暂时不可用。") : QStringLiteral("AI 服务错误: %1").arg(errorMsg);
        m_callbacks[reqId](msg, true);
        m_callbacks.remove(reqId);
        m_accumulators.remove(reqId);
    });
}

QString RemoteAiDataSource::resolveTargetId(const QString& modelName) const
{
    const QString lower = modelName.toLower();
    if (lower.contains(QStringLiteral("openai")) || lower.contains(QStringLiteral("gpt"))) {
        return QStringLiteral("ai_openai");
    }
    if (lower.contains(QStringLiteral("ollama")) || lower.contains(QStringLiteral("llama"))) {
        return QStringLiteral("ai_ollama");
    }
    return QStringLiteral("ai_deepseek");
}

void RemoteAiDataSource::streamGenerate(const QString& prompt,
                                        const QString& modelName,
                                        StreamCallback callback,
                                        const QString& taskType)
{
    if (!callback) {
        return;
    }

    if (!TcpChatClient::instance().isConnected()) {
        callback(QStringLiteral("网络连接已断开，无法与服务端通信。"), true);
        return;
    }

    const QString targetId = resolveTargetId(modelName);
    const QString reqId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    m_callbacks[reqId] = std::move(callback);
    m_accumulators[reqId].clear();

    // 经由 TCP 客户端发送带有 reqId 和 taskType 的数据帧
    if (!TcpChatClient::instance().sendMessage(targetId, prompt, taskType, reqId)) {
        if (m_callbacks.contains(reqId)) {
            m_callbacks[reqId](QStringLiteral("消息发送失败，无法写入网络套接字。"), true);
            m_callbacks.remove(reqId);
            m_accumulators.remove(reqId);
        }
    }
}

}  // namespace data::remote
