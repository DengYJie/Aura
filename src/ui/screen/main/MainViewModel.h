#pragma once

#include <memory>
#include <QList>
#include <QString>

#include "../../common/BaseViewModel.h"
#include "../../navigation/PageId.h"
#include "domain/model/Conversation.h"

namespace domain::usecase {
class GetConversationsUseCase;
}

namespace ui::screen::main {

using ConversationData = domain::model::Conversation;

struct MainState {
    ui::navigation::PageId currentPage = ui::navigation::PageId::Chat;
    QString selectedConversationId = QStringLiteral("ai_chat");
    QList<ConversationData> conversations;

    const ConversationData* selectedConversation() const
    {
        for (const auto& conv : conversations) {
            if (conv.id == selectedConversationId) {
                return &conv;
            }
        }
        return conversations.isEmpty() ? nullptr : &conversations.first();
    }

    bool operator==(const MainState& other) const
    {
        return currentPage == other.currentPage &&
               selectedConversationId == other.selectedConversationId &&
               conversations == other.conversations;
    }
};

class MainViewModel : public BaseViewModel<MainViewModel, MainState> {
    Q_OBJECT

public:
    explicit MainViewModel(QObject* parent = nullptr);
    explicit MainViewModel(std::shared_ptr<domain::usecase::GetConversationsUseCase> getConversationsUseCase,
                           QObject* parent = nullptr);

    // Intents / Actions
    void loadConversations();
    void selectPage(ui::navigation::PageId pageId);
    void selectConversation(const QString& conversationId);

signals:
    void stateChanged(const MainState& state);
    void pageSwitched(ui::navigation::PageId pageId);

protected:
    void emitStateChanged() override
    {
        emit stateChanged(m_state);
    }

private:
    std::shared_ptr<domain::usecase::GetConversationsUseCase> m_getConversationsUseCase;
};

}  // namespace ui::screen::main
