#pragma once

#include <memory>
#include <QString>
#include <QVector>

#include "../model/Message.h"
#include "../repository/IChatRepository.h"

namespace domain::usecase {

class GetMessagesUseCase : public QObject {
    Q_OBJECT
public:
    explicit GetMessagesUseCase(std::shared_ptr<repository::IChatRepository> repository, QObject* parent = nullptr);

    QVector<model::Message> execute(const QString& conversationId) const;

signals:
    void messageReceived(const QString& conversationId, const model::Message& message);
    void messageDeliveryStatusChanged(const QString& conversationId, const QString& messageId, model::MessageDeliveryStatus status);

private:
    std::shared_ptr<repository::IChatRepository> m_repository;
};

}  // namespace domain::usecase
