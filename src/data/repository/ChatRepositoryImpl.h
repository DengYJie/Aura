#pragma once

#include <memory>

#include "domain/repository/IChatRepository.h"
#include "../local/LocalChatDataSource.h"
#include "../remote/RemoteAiDataSource.h"

namespace data::repository {

class ChatRepositoryImpl : public domain::repository::IChatRepository {
    Q_OBJECT
public:
    ChatRepositoryImpl(std::shared_ptr<local::LocalChatDataSource> localDataSource,
                       std::shared_ptr<remote::RemoteAiDataSource> remoteDataSource,
                       QObject* parent = nullptr);

    QList<domain::model::Conversation> getConversations() override;
    QVector<domain::model::Message> getMessages(const QString& conversationId) override;
    void saveMessage(const QString& conversationId, const domain::model::Message& message) override;
    void requestAiStreaming(const QString& conversationId,
                            const QString& prompt,
                            const QString& modelName,
                            StreamChunkCallback callback) override;
    void requestAiTask(const QString& conversationId,
                       const QString& taskType,
                       const QString& prompt,
                       const QString& modelName,
                       StreamChunkCallback callback) override;

private:
    QString buildRecentHumanContext(const QString& conversationId,
                                    int maxMessages) const;

    std::shared_ptr<local::LocalChatDataSource> m_localDataSource;
    std::shared_ptr<remote::RemoteAiDataSource> m_remoteDataSource;
};

}  // namespace data::repository
