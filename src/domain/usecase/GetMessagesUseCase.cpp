#include "GetMessagesUseCase.h"

namespace domain::usecase {

GetMessagesUseCase::GetMessagesUseCase(std::shared_ptr<repository::IChatRepository> repository, QObject* parent)
    : QObject(parent), m_repository(std::move(repository))
{
    if (m_repository) {
        connect(m_repository.get(), &repository::IChatRepository::messageReceived,
                this, &GetMessagesUseCase::messageReceived);
        connect(m_repository.get(), &repository::IChatRepository::messageDeliveryStatusChanged,
                this, &GetMessagesUseCase::messageDeliveryStatusChanged);
    }
}

QVector<model::Message> GetMessagesUseCase::execute(const QString& conversationId) const
{
    if (!m_repository || conversationId.isEmpty()) {
        return {};
    }
    return m_repository->getMessages(conversationId);
}

}  // namespace domain::usecase
