#include "AiModelRouter.h"

#include "OpenAICompatibleClient.h"
#include "../core/ClientSession.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <memory>

AiModelRouter& AiModelRouter::instance() {
    static AiModelRouter inst;
    return inst;
}

AiModelRouter::AiModelRouter(QObject* parent) : QObject(parent) {}

// ── 初始化 ────────────────────────────────────────────────────────────

void AiModelRouter::init(const ServerConfig& config)
{
    m_contextConfig = config.context;
    m_providers     = config.providers;
    m_routing       = config.routing;
    m_prompts       = config.prompts;

    qInfo().noquote() << QStringLiteral("[AI] Initialized: %1 providers, %2 routing rules, %3 custom prompts (maxTurns=%4, maxChars=%5)")
                         .arg(m_providers.size())
                         .arg(m_routing.size())
                         .arg(m_prompts.size())
                         .arg(m_contextConfig.maxTurns)
                         .arg(m_contextConfig.maxChars);

    for (auto it = m_routing.constBegin(); it != m_routing.constEnd(); ++it) {
        qInfo().noquote() << QStringLiteral("[AI]   task=%1 → %2").arg(it.key(), -12).arg(it.value());
    }
}

// ── Provider 工厂（按 type 分发） ─────────────────────────────────────

IAiProvider* AiModelRouter::getProvider(const QString& providerName)
{
    if (!m_providers.contains(providerName)) {
        qWarning().noquote() << QStringLiteral("[AI] Unknown provider: %1").arg(providerName);
        return nullptr;
    }
    const AiProviderConfig& cfg = m_providers[providerName];

    if (cfg.type == QStringLiteral("openai_compatible")) {
        return new OpenAICompatibleClient(cfg.apiKey, cfg.baseUrl, cfg.model,
                                          cfg.temperature, cfg.maxTokens, this);
    }

    qWarning().noquote() << QStringLiteral("[AI] Unsupported provider type: %1 (provider=%2)")
                            .arg(cfg.type, providerName);
    return nullptr;
}

// ── 路由消息（taskType → routing → provider） ─────────────────────────

void AiModelRouter::routeMessage(const QString& targetBotId,
    const QString& content,
    ClientSession* session,
    const QString& taskType,
    const QString& reqId) {

    const QString resolvedTask = taskType.isEmpty() ? QStringLiteral("chat") : taskType;

    // 查 routing 表决定 providerName；未命中则用 default
    QString providerName = m_routing.value(resolvedTask, m_routing.value("default"));
    if (providerName.isEmpty()) {
        qWarning().noquote() << QStringLiteral("[AI] No routing rule for task=%1, and no default").arg(resolvedTask);
        QJsonObject payload;
        payload["from"] = targetBotId;
        payload["content"] = "AI service unavailable.";
        if (!reqId.isEmpty()) payload["reqId"] = reqId;
        session->sendFrame(Aura::Protocol::MessageType::Error, payload);
        return;
    }

    IAiProvider* provider = getProvider(providerName);
    if (!provider) {
        qWarning().noquote() << QStringLiteral("[AI] No provider for task=%1 → %2").arg(resolvedTask, providerName);
        QJsonObject payload;
        payload["from"] = targetBotId;
        payload["content"] = "AI service unavailable.";
        if (!reqId.isEmpty()) payload["reqId"] = reqId;
        session->sendFrame(Aura::Protocol::MessageType::Error, payload);
        return;
    }

    QPointer<ClientSession> safeSession(session);
    const QString sessionUserId = session->getUserId();
    // conversationKey 按 user + providerName 隔离上下文
    const QString conversationKey = sessionUserId + "::" + providerName;

    QJsonArray messages = (resolvedTask == QStringLiteral("chat"))
        ? buildChatMessages(conversationKey, content)
        : buildTaskMessages(content, resolvedTask);

    if (messages.isEmpty()) {
        qWarning().noquote() << QStringLiteral("[AI] Empty message array for task=%1 user=%2").arg(resolvedTask, sessionUserId);
        QJsonObject payload;
        payload["from"] = targetBotId;
        payload["content"] = QStringLiteral("Invalid AI task or empty local context.");
        if (!reqId.isEmpty()) payload["reqId"] = reqId;
        session->sendFrame(Aura::Protocol::MessageType::Error, payload);
        provider->deleteLater();
        return;
    }

    if (resolvedTask == QStringLiteral("chat")) {
        appendChatHistory(conversationKey, LlmMessage::Role::User, content);
    }

    const int contextTurns = messages.size();
    qDebug().noquote() << QStringLiteral("[AI] %1 | task=%2 → %3 | ctx=%4 msgs | reqId=%5")
        .arg(sessionUserId, resolvedTask, providerName).arg(contextTurns).arg(reqId.left(8));

    QObject* context = new QObject(provider);
    auto streamedText = std::make_shared<QString>();

    connect(provider, &IAiProvider::streamChunkReceived, context, [safeSession, targetBotId, streamedText, reqId](const QString& chunk) {
        streamedText->append(chunk);
        if (!safeSession) {
            return;
        }
        QJsonObject payload;
        payload["from"] = targetBotId;
        payload["chunk"] = chunk;
        if (!reqId.isEmpty()) payload["reqId"] = reqId;
        safeSession->sendFrame(Aura::Protocol::MessageType::StreamChunk, payload);
    });

    connect(provider, &IAiProvider::streamFinished, context, [this, safeSession, targetBotId, provider, context, conversationKey, streamedText, resolvedTask, reqId, sessionUserId]() {
        const int chars = streamedText->size();
        qDebug().noquote() << QStringLiteral("[AI] %1 | task=%2 | done | %3 chars")
            .arg(sessionUserId, resolvedTask).arg(chars);
        if (safeSession) {
            QJsonObject payload;
            payload["from"] = targetBotId;
            if (!reqId.isEmpty()) payload["reqId"] = reqId;
            safeSession->sendFrame(Aura::Protocol::MessageType::StreamEnd, payload);
        }
        if (resolvedTask == QStringLiteral("chat") && !streamedText->isEmpty()) {
            appendChatHistory(conversationKey, LlmMessage::Role::Assistant, *streamedText);
        }
        provider->deleteLater();
        context->deleteLater();
    });

    connect(provider, &IAiProvider::errorOccurred, context, [safeSession, targetBotId, provider, context, reqId, sessionUserId, resolvedTask](const QString& err) {
        qWarning().noquote() << QStringLiteral("[AI] %1 | task=%2 | error: %3").arg(sessionUserId, resolvedTask, err);
        if (safeSession) {
            QJsonObject payload;
            payload["from"] = targetBotId;
            payload["content"] = err;
            if (!reqId.isEmpty()) payload["reqId"] = reqId;
            safeSession->sendFrame(Aura::Protocol::MessageType::Error, payload);
        }
        provider->deleteLater();
        context->deleteLater();
    });

    provider->requestChatStreaming(messages);
}

QJsonObject AiModelRouter::toOpenAiMessage(const LlmMessage& message) const
{
    QString role;
    switch (message.role) {
    case LlmMessage::Role::System:
        role = QStringLiteral("system");
        break;
    case LlmMessage::Role::User:
        role = QStringLiteral("user");
        break;
    case LlmMessage::Role::Assistant:
        role = QStringLiteral("assistant");
        break;
    }

    QJsonObject obj;
    obj["role"] = role;
    obj["content"] = message.content;
    return obj;
}

QList<AiModelRouter::LlmMessage> AiModelRouter::trimConversationWindow(const QList<LlmMessage>& source,
    int maxTurns,
    int maxChars) const
{
    QList<LlmMessage> trimmed = source;
    while (trimmed.size() > maxTurns) {
        trimmed.removeFirst();
    }

    auto totalChars = [&trimmed]() {
        int chars = 0;
        for (const auto& msg : trimmed) {
            chars += msg.content.size();
        }
        return chars;
        };

    while (!trimmed.isEmpty() && totalChars() > maxChars) {
        trimmed.removeFirst();
    }

    return trimmed;
}

QJsonArray AiModelRouter::buildChatMessages(const QString& conversationKey,
    const QString& content) const
{
    const QString defaultChatPrompt = QStringLiteral("You are an intelligent desktop chat assistant. Reply naturally, clearly, and helpfully.");
    const QString systemPrompt = m_prompts.value(QStringLiteral("chat"), defaultChatPrompt);

    QList<LlmMessage> window;
    window.append({
        .role = LlmMessage::Role::System,
        .content = systemPrompt
    });

    const auto it = m_contexts.constFind(conversationKey);
    if (it != m_contexts.cend()) {
        window.append(trimConversationWindow(it->recentTurns,
                                             m_contextConfig.maxTurns,
                                             m_contextConfig.maxChars));
    }

    window.append({
        .role = LlmMessage::Role::User,
        .content = content
    });

    QJsonArray messages;
    for (const auto& msg : window) {
        messages.append(toOpenAiMessage(msg));
    }
    return messages;
}

QJsonArray AiModelRouter::buildTaskMessages(const QString& content,
    const QString& taskType) const
{
    if (content.trimmed().isEmpty()) {
        return {};
    }

    QString systemPrompt;
    if (m_prompts.contains(taskType)) {
        systemPrompt = m_prompts.value(taskType);
    } else if (taskType == QStringLiteral("translate")) {
        systemPrompt = QStringLiteral(
            "You are a professional translator. Translate the user's text directly and do not add explanation.");
    } else if (taskType == QStringLiteral("sentiment")) {
        systemPrompt = QStringLiteral(
            "The user provides a recent decrypted local human-to-human conversation transcript. "
            "Analyze the sentiment of the other person's latest message using the previous few rounds as context. "
            "Output only one word: Positive, Neutral, or Negative.");
    } else if (taskType == QStringLiteral("suggest")) {
        systemPrompt = QStringLiteral(
            "The user provides a recent decrypted local human-to-human conversation transcript. "
            "Generate exactly 3 short, natural replies for the user to send next. "
            "Use the previous few rounds as context. Separate the replies with '|', and output nothing else.");
    } else {
        return {};
    }

    QJsonArray messages;
    messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("system")},
        {QStringLiteral("content"), systemPrompt}
    });
    messages.append(QJsonObject{
        {QStringLiteral("role"), QStringLiteral("user")},
        {QStringLiteral("content"), content}
    });
    return messages;
}

void AiModelRouter::appendChatHistory(const QString& conversationKey,
    LlmMessage::Role role,
    const QString& text)
{
    auto& context = m_contexts[conversationKey];
    context.recentTurns.append({
        .role = role,
        .content = text
    });

    context.recentTurns = trimConversationWindow(context.recentTurns,
                                                 m_contextConfig.maxTurns,
                                                 m_contextConfig.maxChars);
}

