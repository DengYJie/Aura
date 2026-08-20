#include "GetConversationsUseCase.h"

namespace domain::usecase {

GetConversationsUseCase::GetConversationsUseCase(std::shared_ptr<repository::IChatRepository> repository)
    : m_repository(std::move(repository))
{}

QList<model::Conversation> GetConversationsUseCase::execute() const
{
    if (!m_repository) {
        return {};
    }
    return m_repository->getConversations();
}

}  // namespace domain::usecase
