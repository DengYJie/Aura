#include "MainViewModel.h"

#include "data/di/AppContainer.h"
#include "domain/usecase/GetConversationsUseCase.h"

namespace ui::screen::main {

MainViewModel::MainViewModel(QObject* parent)
    : MainViewModel(nullptr, parent)
{}

MainViewModel::MainViewModel(std::shared_ptr<domain::usecase::GetConversationsUseCase> getConversationsUseCase,
                             QObject* parent)
    : BaseViewModel<MainViewModel, MainState>(parent)
    , m_getConversationsUseCase(getConversationsUseCase ? std::move(getConversationsUseCase)
                                                       : AppContainer::getConversationsUseCase())
{
    loadConversations();
}

void MainViewModel::loadConversations()
{
    QList<ConversationData> convs;
    if (m_getConversationsUseCase) {
        convs = m_getConversationsUseCase->execute();
    }

    const QString initialId = convs.isEmpty() ? QString() : convs.first().id;

    updateState([convs, initialId](MainState& state) {
        state.currentPage = ui::navigation::PageId::Chat;
        state.conversations = convs;
        state.selectedConversationId = initialId;
    });
}

void MainViewModel::selectPage(ui::navigation::PageId pageId)
{
    updateState([pageId](MainState& state) {
        state.currentPage = pageId;
    });
    emit pageSwitched(pageId);
}

void MainViewModel::selectConversation(const QString& conversationId)
{
    updateState([conversationId](MainState& state) {
        state.selectedConversationId = conversationId;
        state.currentPage = ui::navigation::PageId::Chat;
    });
    emit pageSwitched(ui::navigation::PageId::Chat);
}

}  // namespace ui::screen::main
