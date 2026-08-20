#pragma once

#include <QHash>
#include <QJsonArray>
#include <QObject>

#include "IAiProvider.h"
#include "../core/ServerConfig.h"

class ClientSession;

class AiModelRouter : public QObject {
    Q_OBJECT
public:
    static AiModelRouter& instance();

    // 接收已解析的 ServerConfig 进行 AI 模块初始化
    void init(const ServerConfig& config);

    void routeMessage(const QString& targetBotId,
        const QString& content,
        ClientSession* session,
        const QString& taskType = QStringLiteral("chat"),
        const QString& reqId = QString());

private:
    struct LlmMessage {
        enum class Role { System, User, Assistant };
        Role    role = Role::User;
        QString content;
    };

    struct ConversationContext {
        QList<LlmMessage> recentTurns;
    };

    explicit AiModelRouter(QObject* parent = nullptr);
    ~AiModelRouter() override = default;

    IAiProvider* getProvider(const QString& providerName);
    QJsonObject toOpenAiMessage(const LlmMessage& message) const;
    QList<LlmMessage> trimConversationWindow(const QList<LlmMessage>& source,
        int maxTurns, int maxChars) const;
    QJsonArray buildChatMessages(const QString& conversationKey,
        const QString& content) const;
    QJsonArray buildTaskMessages(const QString& content,
        const QString& taskType) const;
    void appendChatHistory(const QString& conversationKey,
        LlmMessage::Role role, const QString& text);

    AiContextConfig                     m_contextConfig;
    QHash<QString, AiProviderConfig>    m_providers; // providerName → config
    QHash<QString, QString>             m_routing;   // taskType → providerName
    QHash<QString, QString>             m_prompts;   // taskType → systemPrompt
    QHash<QString, ConversationContext> m_contexts;
};
