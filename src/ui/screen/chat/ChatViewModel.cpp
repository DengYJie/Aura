#include "ChatViewModel.h"

#include <QDateTime>

#include "data/di/AppContainer.h"
#include "domain/usecase/GetMessagesUseCase.h"
#include "domain/usecase/SendMessageUseCase.h"
#include "domain/usecase/TranslateUseCase.h"

namespace ui::screen::chat {

namespace {

MessageItemState toUiMessage(const domain::model::Message& m)
{
    return {
        m.id,
        static_cast<MessageSenderType>(m.senderType),
        m.senderName,
        m.timestamp,
        m.rawText,
        static_cast<MessageRenderMode>(m.renderMode),
        static_cast<MessageSentimentTag>(m.sentimentTag),
        static_cast<MessageDeliveryStatus>(m.deliveryStatus),
        m.isStreaming
    };
}

domain::model::Message toDomainMessage(const MessageItemState& m)
{
    return {
        m.id,
        static_cast<domain::model::MessageSenderType>(m.senderType),
        m.senderName,
        m.timestamp,
        m.rawText,
        static_cast<domain::model::MessageRenderMode>(m.renderMode),
        static_cast<domain::model::MessageSentimentTag>(m.sentimentTag),
        static_cast<domain::model::MessageDeliveryStatus>(m.deliveryStatus),
        m.isStreaming
    };
}

QVector<MessageItemState> toUiMessages(const QVector<domain::model::Message>& list)
{
    QVector<MessageItemState> result;
    result.reserve(list.size());
    for (const auto& item : list) {
        result.append(toUiMessage(item));
    }
    return result;
}

MessageSentimentTag parseSentimentTag(const QString& text)
{
    const QString normalized = text.trimmed().toLower();
    if (normalized.contains(QStringLiteral("positive")) || normalized.contains(QStringLiteral("积极"))) {
        return MessageSentimentTag::Positive;
    }
    if (normalized.contains(QStringLiteral("negative")) || normalized.contains(QStringLiteral("消极"))) {
        return MessageSentimentTag::Negative;
    }
    if (normalized.contains(QStringLiteral("neutral")) || normalized.contains(QStringLiteral("中性"))) {
        return MessageSentimentTag::Neutral;
    }
    return MessageSentimentTag::None;
}

}  // namespace

ChatViewModel::ChatViewModel(QObject* parent)
    : ChatViewModel(nullptr, nullptr, nullptr, parent)
{}

ChatViewModel::ChatViewModel(std::shared_ptr<domain::usecase::GetMessagesUseCase> getMessagesUseCase,
                             std::shared_ptr<domain::usecase::SendMessageUseCase> sendMessageUseCase,
                             std::shared_ptr<domain::usecase::TranslateUseCase> translateUseCase,
                             QObject* parent)
    : BaseViewModel<ChatViewModel, ChatState>(parent)
    , m_getMessagesUseCase(getMessagesUseCase ? std::move(getMessagesUseCase)
                                             : AppContainer::getMessagesUseCase())
    , m_sendMessageUseCase(sendMessageUseCase ? std::move(sendMessageUseCase)
                                             : AppContainer::getSendMessageUseCase())
    , m_translateUseCase(translateUseCase ? std::move(translateUseCase)
                                         : AppContainer::getTranslateUseCase())
{
    // 初始化首选默认会话
    const QString initialId = QStringLiteral("ai_chat");
    const QString initialTitle = QStringLiteral("DeepSeek 助手");
    QVector<MessageItemState> initialMsgs;
    if (m_getMessagesUseCase) {
        initialMsgs = toUiMessages(m_getMessagesUseCase->execute(initialId));
    }

    updateState([initialId, initialTitle, initialMsgs](ChatState& state) {
        state.conversationId = initialId;
        state.conversationTitle = initialTitle;
        state.currentModel = QStringLiteral("DeepSeek-V3");
        state.isStreaming = false;
        state.isTranslateOpen = false;
        state.translateTargetLang = QStringLiteral("英语 (English)");
        state.translatedContent = QStringLiteral("欢迎使用实时翻译功能。发送或选中的消息将在此即时翻译为目标语言。");
        state.messages = initialMsgs;
    });

    if (m_getMessagesUseCase) {
        connect(m_getMessagesUseCase.get(), &domain::usecase::GetMessagesUseCase::messageReceived,
                this, [this](const QString& conversationId, const domain::model::Message& msg) {
            if (m_state.conversationId == conversationId) {
                updateState([msg](ChatState& state) {
                    state.messages.append(toUiMessage(msg));
                });
                if (!conversationId.startsWith(QStringLiteral("ai_"))) {
                    refreshHumanAssistFeatures();
                }
            }
        });

        connect(m_getMessagesUseCase.get(), &domain::usecase::GetMessagesUseCase::messageDeliveryStatusChanged,
                this, [this](const QString& conversationId, const QString& messageId, domain::model::MessageDeliveryStatus status) {
            if (m_state.conversationId == conversationId) {
                updateState([messageId, status](ChatState& state) {
                    for (auto& msg : state.messages) {
                        if (msg.id == messageId) {
                            msg.deliveryStatus = static_cast<MessageDeliveryStatus>(status);
                            break;
                        }
                    }
                });
            }
        });
    }
}

bool ChatViewModel::sendMessage(const QString& rawText)
{
    if (rawText.trimmed().isEmpty() || m_state.isStreaming) {
        return false;
    }

    // AI 会话或普通聊天中，若存在正在 Sending 或 isStreaming 的消息，禁止发送下一条
    for (const auto& msg : m_state.messages) {
        if (msg.deliveryStatus == MessageDeliveryStatus::Sending || msg.isStreaming) {
            return false;
        }
    }

    const QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm"));
    const QString msgId = QStringLiteral("self-%1").arg(QDateTime::currentMSecsSinceEpoch());

    MessageItemState userMsg;
    userMsg.id = msgId;
    userMsg.senderType = MessageSenderType::Self;
    userMsg.senderName = QStringLiteral("我");
    userMsg.timestamp = timeStr;
    userMsg.rawText = rawText;
    
    if (rawText.startsWith("data:image/")) {
        userMsg.renderMode = MessageRenderMode::Image;
    } else {
        userMsg.renderMode = MessageRenderMode::PlainText;
    }
    
    userMsg.sentimentTag = MessageSentimentTag::None;
    userMsg.deliveryStatus = m_state.conversationId.startsWith("ai_") ? MessageDeliveryStatus::Sent : MessageDeliveryStatus::Sending;
    userMsg.isStreaming = false;

    // 持久化到数据层
    if (m_sendMessageUseCase) {
        m_sendMessageUseCase->saveMessage(m_state.conversationId, toDomainMessage(userMsg));
    }

    updateState([userMsg](ChatState& state) {
        state.messages.append(userMsg);
        state.quickReplies.clear(); // 发送消息后，清空上一轮建议回复
    });

    emit messageSent();

    if (m_state.isTranslateOpen) {
        translateText(rawText);
    }

    if (!m_state.conversationId.startsWith("ai_")) {
        // 人人聊天：自己刚发完，对方尚未回复，不触发建议
        return true;
    }

    // 发起 AI 流式推理请求（令牌安全）
    const quint64 reqId = beginRequest();
    const QString aiMsgId = QStringLiteral("ai-%1").arg(QDateTime::currentMSecsSinceEpoch());
    const QString currentModelName = m_state.currentModel;

    MessageItemState aiMsg;
    aiMsg.id = aiMsgId;
    aiMsg.senderType = MessageSenderType::Assistant;
    aiMsg.senderName = currentModelName.isEmpty() ? QStringLiteral("Aura AI") : currentModelName;
    aiMsg.timestamp = timeStr;
    aiMsg.rawText = QStringLiteral("正在思考中...");
    aiMsg.renderMode = MessageRenderMode::PlainText;
    aiMsg.sentimentTag = MessageSentimentTag::None;
    aiMsg.deliveryStatus = MessageDeliveryStatus::Sending;
    aiMsg.isStreaming = true;

    updateState([aiMsg](ChatState& state) {
        state.messages.append(aiMsg);
        state.isStreaming = true;
    });

    if (m_sendMessageUseCase) {
        m_sendMessageUseCase->requestAiStreaming(
            m_state.conversationId, rawText, currentModelName,
            [this, reqId, aiMsgId](const QString& chunkSlice, bool isFinished) {
                if (!isRequestCurrent(reqId)) {
                    return;
                }

                MessageItemState targetMsg;

                updateState([aiMsgId, chunkSlice, isFinished, &targetMsg](ChatState& state) {
                    for (auto& msg : state.messages) {
                        if (msg.id == aiMsgId) {
                            msg.rawText = chunkSlice;
                            msg.isStreaming = !isFinished;
                            msg.deliveryStatus = isFinished ? MessageDeliveryStatus::Sent : MessageDeliveryStatus::Sending;
                            msg.renderMode = isFinished ? MessageRenderMode::Markdown : MessageRenderMode::PlainText;
                            targetMsg = msg;
                            break;
                        }
                    }
                    state.isStreaming = !isFinished;
                });

                if (isFinished && m_sendMessageUseCase && !targetMsg.id.isEmpty()) {
                    m_sendMessageUseCase->saveMessage(m_state.conversationId, toDomainMessage(targetMsg));
                }
            });
    }

    return true;
}

void ChatViewModel::selectModel(const QString& model)
{
    updateState([model](ChatState& state) {
        state.currentModel = model;
    });
}

void ChatViewModel::toggleTranslatePane()
{
    updateState([](ChatState& state) {
        state.isTranslateOpen = !state.isTranslateOpen;
    });
}

void ChatViewModel::setTranslateVisible(bool visible)
{
    updateState([visible](ChatState& state) {
        state.isTranslateOpen = visible;
    });
}

void ChatViewModel::setTranslateTargetLanguage(const QString& lang)
{
    updateState([lang](ChatState& state) {
        state.translateTargetLang = lang;
    });

    if (!m_state.messages.isEmpty()) {
        translateText(m_state.messages.last().rawText);
    }
}

void ChatViewModel::translateText(const QString& text)
{
    if (text.trimmed().isEmpty()) {
        return;
    }

    const quint64 reqId = beginRequest();
    updateState([](ChatState& state) {
        state.isTranslating = true;
        state.translatedContent = QStringLiteral("正在翻译中...");
    });

    if (m_translateUseCase) {
        m_translateUseCase->execute(text, m_state.translateTargetLang, [this, reqId](const QString& result, bool isFinished) {
            if (!isRequestCurrent(reqId)) {
                return;
            }
            updateState([result, isFinished](ChatState& state) {
                state.isTranslating = !isFinished;
                state.translatedContent = result.isEmpty() ? QStringLiteral("翻译失败") : result;
            });
        });
    }
}

void ChatViewModel::switchConversation(const QString& conversationId, const QString& title)
{
    if (m_state.conversationId == conversationId && m_state.conversationTitle == title) {
        return;
    }

    invalidateRequests();

    QVector<MessageItemState> nextMessages;
    if (m_getMessagesUseCase) {
        nextMessages = toUiMessages(m_getMessagesUseCase->execute(conversationId));
    }

    updateState([conversationId, title, nextMessages](ChatState& state) {
        state.conversationId = conversationId;
        state.conversationTitle = title;
        state.messages = nextMessages;
        state.isStreaming = false;
    });

    if (!conversationId.startsWith(QStringLiteral("ai_"))) {
        if (!nextMessages.isEmpty() && nextMessages.last().senderType == MessageSenderType::Peer) {
            refreshHumanAssistFeatures();
        } else {
            updateState([](ChatState& state) {
                state.quickReplies.clear();
            });
        }
    }
}

QString ChatViewModel::normalizeSuggestion(const QString& text) const
{
    QString value = text.trimmed();
    if (value.startsWith(QStringLiteral("- "))) {
        value.remove(0, 2);
    }
    return value;
}

void ChatViewModel::refreshHumanAssistFeatures()
{
    if (m_state.conversationId.startsWith(QStringLiteral("ai_")) || !m_sendMessageUseCase) {
        return;
    }

    // 仅在最后一条消息是对方发送时才请求建议与分析
    if (m_state.messages.isEmpty() || m_state.messages.last().senderType != MessageSenderType::Peer) {
        updateState([](ChatState& state) {
            state.quickReplies.clear();
        });
        return;
    }

    requestSentimentAnalysis();
    requestReplySuggestions();
}

void ChatViewModel::requestSentimentAnalysis()
{
    // 防重复：如果最后一条来自对方的消息已经存在情绪标签，则无需重复请求
    for (auto it = m_state.messages.crbegin(); it != m_state.messages.crend(); ++it) {
        if (it->senderType == MessageSenderType::Peer) {
            if (it->sentimentTag != MessageSentimentTag::None) {
                return;
            }
            break;
        }
    }

    const quint64 reqId = ++m_sentimentRequestId;
    m_sendMessageUseCase->requestAiTask(
        m_state.conversationId,
        QStringLiteral("sentiment"),
        QString(),
        m_state.currentModel,
        [this, reqId](const QString& result, bool isFinished) {
            if (!isFinished || reqId != m_sentimentRequestId) {
                return;
            }

            const MessageSentimentTag tag = parseSentimentTag(result);
            if (tag == MessageSentimentTag::None) {
                return;
            }

            MessageItemState updatedMsg;
            bool found = false;

            updateState([tag, &updatedMsg, &found](ChatState& state) {
                for (auto it = state.messages.rbegin(); it != state.messages.rend(); ++it) {
                    if (it->senderType == MessageSenderType::Peer) {
                        it->sentimentTag = tag;
                        updatedMsg = *it;
                        found = true;
                        break;
                    }
                }
            });

            // 将分析的情绪标签持久化保存到本地数据库
            if (found && m_sendMessageUseCase) {
                m_sendMessageUseCase->saveMessage(m_state.conversationId, toDomainMessage(updatedMsg));
            }
        });
}

void ChatViewModel::requestReplySuggestions()
{
    // 防重复：如果当前会话已有建议，无需重复拉取
    if (!m_state.quickReplies.isEmpty()) {
        return;
    }

    const quint64 reqId = ++m_suggestionRequestId;
    m_sendMessageUseCase->requestAiTask(
        m_state.conversationId,
        QStringLiteral("suggest"),
        QString(),
        m_state.currentModel,
        [this, reqId](const QString& result, bool isFinished) {
            if (!isFinished || reqId != m_suggestionRequestId) {
                return;
            }

            const QStringList rawItems = result.split('|', Qt::SkipEmptyParts);
            QStringList suggestions;
            for (const QString& item : rawItems) {
                const QString normalized = normalizeSuggestion(item);
                if (!normalized.isEmpty()) {
                    suggestions.append(normalized);
                }
            }

            if (suggestions.isEmpty()) {
                return;
            }

            while (suggestions.size() > 3) {
                suggestions.removeLast();
            }

            updateState([suggestions](ChatState& state) {
                state.quickReplies = suggestions;
            });
        });
}

void ChatViewModel::retryMessage(const QString& messageId)
{
    MessageItemState targetMsg;
    bool found = false;
    for (const auto& msg : m_state.messages) {
        if (msg.id == messageId && msg.deliveryStatus == MessageDeliveryStatus::Failed) {
            targetMsg = msg;
            found = true;
            break;
        }
    }
    if (!found) {
        return;
    }

    // 更新当前消息状态为 Sending
    updateState([messageId](ChatState& state) {
        for (auto& item : state.messages) {
            if (item.id == messageId) {
                item.deliveryStatus = MessageDeliveryStatus::Sending;
                break;
            }
        }
    });

    if (m_state.conversationId.startsWith(QStringLiteral("ai_"))) {
        // AI 会话重试：重新发起 AI 流式推理
        const quint64 reqId = beginRequest();
        const QString aiMsgId = QStringLiteral("ai-%1").arg(QDateTime::currentMSecsSinceEpoch());
        const QString currentModelName = m_state.currentModel;
        const QString timeStr = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm"));

        MessageItemState aiMsg;
        aiMsg.id = aiMsgId;
        aiMsg.senderType = MessageSenderType::Assistant;
        aiMsg.senderName = currentModelName.isEmpty() ? QStringLiteral("Aura AI") : currentModelName;
        aiMsg.timestamp = timeStr;
        aiMsg.rawText = QStringLiteral("正在重新思考中...");
        aiMsg.renderMode = MessageRenderMode::PlainText;
        aiMsg.sentimentTag = MessageSentimentTag::None;
        aiMsg.deliveryStatus = MessageDeliveryStatus::Sending;
        aiMsg.isStreaming = true;

        updateState([messageId, aiMsg](ChatState& state) {
            for (auto& item : state.messages) {
                if (item.id == messageId) {
                    item.deliveryStatus = MessageDeliveryStatus::Sent;
                    break;
                }
            }
            state.messages.append(aiMsg);
            state.isStreaming = true;
        });

        if (m_sendMessageUseCase) {
            m_sendMessageUseCase->requestAiStreaming(
                m_state.conversationId, targetMsg.rawText, currentModelName,
                [this, reqId, aiMsgId](const QString& chunkSlice, bool isFinished) {
                    if (!isRequestCurrent(reqId)) {
                        return;
                    }

                    MessageItemState targetAiMsg;
                    updateState([aiMsgId, chunkSlice, isFinished, &targetAiMsg](ChatState& state) {
                        for (auto& msg : state.messages) {
                            if (msg.id == aiMsgId) {
                                msg.rawText = chunkSlice;
                                msg.isStreaming = !isFinished;
                                msg.deliveryStatus = isFinished ? MessageDeliveryStatus::Sent : MessageDeliveryStatus::Sending;
                                msg.renderMode = isFinished ? MessageRenderMode::Markdown : MessageRenderMode::PlainText;
                                targetAiMsg = msg;
                                break;
                            }
                        }
                        state.isStreaming = !isFinished;
                    });

                    if (isFinished && m_sendMessageUseCase && !targetAiMsg.id.isEmpty()) {
                        m_sendMessageUseCase->saveMessage(m_state.conversationId, toDomainMessage(targetAiMsg));
                    }
                });
        }
    } else {
        // 人人聊天重试
        if (m_sendMessageUseCase) {
            auto domainMsg = toDomainMessage(targetMsg);
            domainMsg.deliveryStatus = domain::model::MessageDeliveryStatus::Sending;
            m_sendMessageUseCase->saveMessage(m_state.conversationId, domainMsg);
        }
    }
}

}  // namespace ui::screen::chat
