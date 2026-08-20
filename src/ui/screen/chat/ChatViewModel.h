#pragma once

#include <memory>
#include <QStringList>
#include <QVector>

#include "../../common/BaseViewModel.h"
#include "MessageListModel.h"
#include "domain/model/Message.h"

namespace domain::usecase {
class GetMessagesUseCase;
class SendMessageUseCase;
class TranslateUseCase;
}

namespace ui::screen::chat {

struct ChatState {
    QString conversationId;
    QString conversationTitle;
    QString currentModel = QStringLiteral("DeepSeek-V3");
    bool isStreaming = false;
    bool isTranslateOpen = false;
    QString translateTargetLang = QStringLiteral("英语 (English)");
    QString translatedContent = QStringLiteral("欢迎使用实时翻译功能。发送或选中的消息将在此即时翻译为目标语言。");
    bool isTranslating = false;
    QStringList quickReplies;
    QVector<MessageItemState> messages;

    bool operator==(const ChatState& other) const
    {
        return conversationId == other.conversationId &&
               conversationTitle == other.conversationTitle &&
               currentModel == other.currentModel &&
               isStreaming == other.isStreaming &&
               isTranslateOpen == other.isTranslateOpen &&
               translateTargetLang == other.translateTargetLang &&
               translatedContent == other.translatedContent &&
               isTranslating == other.isTranslating &&
               quickReplies == other.quickReplies &&
               messages == other.messages;
    }
};

class ChatViewModel : public BaseViewModel<ChatViewModel, ChatState> {
    Q_OBJECT

public:
    explicit ChatViewModel(QObject* parent = nullptr);
    explicit ChatViewModel(std::shared_ptr<domain::usecase::GetMessagesUseCase> getMessagesUseCase,
                           std::shared_ptr<domain::usecase::SendMessageUseCase> sendMessageUseCase,
                           std::shared_ptr<domain::usecase::TranslateUseCase> translateUseCase,
                           QObject* parent = nullptr);

    // Intents / Actions
    bool sendMessage(const QString& rawText);
    void selectModel(const QString& model);
    void toggleTranslatePane();
    void setTranslateVisible(bool visible);
    void setTranslateTargetLanguage(const QString& lang);
    void translateText(const QString& text);
    void switchConversation(const QString& conversationId, const QString& title);
    void refreshHumanAssistFeatures();
    void retryMessage(const QString& messageId);

signals:
    void stateChanged(const ChatState& state);
    void messageSent();
    void errorOccurred(const QString& errorMessage);

protected:
    void emitStateChanged() override
    {
        emit stateChanged(m_state);
    }

private:
    QString normalizeSuggestion(const QString& text) const;
    void requestSentimentAnalysis();
    void requestReplySuggestions();

    std::shared_ptr<domain::usecase::GetMessagesUseCase> m_getMessagesUseCase;
    std::shared_ptr<domain::usecase::SendMessageUseCase> m_sendMessageUseCase;
    std::shared_ptr<domain::usecase::TranslateUseCase> m_translateUseCase;
    std::atomic<quint64> m_sentimentRequestId{0};
    std::atomic<quint64> m_suggestionRequestId{0};
};

}  // namespace ui::screen::chat
