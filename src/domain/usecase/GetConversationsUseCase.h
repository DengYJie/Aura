#pragma once

#include <memory>
#include <QList>

#include "../model/Conversation.h"
#include "../repository/IChatRepository.h"

namespace domain::usecase {

class GetConversationsUseCase {
public:
    explicit GetConversationsUseCase(std::shared_ptr<repository::IChatRepository> repository);

    QList<model::Conversation> execute() const;

private:
    std::shared_ptr<repository::IChatRepository> m_repository;
};

}  // namespace domain::usecase
