#pragma once

#include <memory>
#include <QString>

namespace domain::usecase {
class GetConversationsUseCase;
class GetMessagesUseCase;
class SendMessageUseCase;
class TranslateUseCase;
}

namespace data::local {
class LocalChatDataSource;
}

namespace data::remote {
class RemoteAiDataSource;
class RemoteTranslationDataSource;
}

namespace domain::repository {
class IChatRepository;
class ITranslationRepository;
}

/**
 * @brief 依赖注入中心 / 服务定位器 (Service Locator)
 *
 * 组装所有 DataSources, Repositories, 和 UseCases。
 */
class AppContainer {
public:
    static void init(const QString& userId);
    static void shutdown();

    static std::shared_ptr<domain::usecase::GetConversationsUseCase> getConversationsUseCase();
    static std::shared_ptr<domain::usecase::GetMessagesUseCase> getMessagesUseCase();
    static std::shared_ptr<domain::usecase::SendMessageUseCase> getSendMessageUseCase();
    static std::shared_ptr<domain::usecase::TranslateUseCase> getTranslateUseCase();

private:
    static std::shared_ptr<data::local::LocalChatDataSource> s_localChatDataSource;
    static std::shared_ptr<data::remote::RemoteAiDataSource> s_remoteAiDataSource;
    static std::shared_ptr<data::remote::RemoteTranslationDataSource> s_remoteTranslationDataSource;

    static std::shared_ptr<domain::repository::IChatRepository> s_chatRepository;
    static std::shared_ptr<domain::repository::ITranslationRepository> s_translationRepository;

    static std::shared_ptr<domain::usecase::GetConversationsUseCase> s_getConversationsUseCase;
    static std::shared_ptr<domain::usecase::GetMessagesUseCase> s_getMessagesUseCase;
    static std::shared_ptr<domain::usecase::SendMessageUseCase> s_sendMessageUseCase;
    static std::shared_ptr<domain::usecase::TranslateUseCase> s_translateUseCase;
};
