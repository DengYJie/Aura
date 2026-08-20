#include "AppContainer.h"

#include "../local/LocalChatDataSource.h"
#include "../remote/RemoteAiDataSource.h"
#include "../remote/RemoteTranslationDataSource.h"

#include "../repository/ChatRepositoryImpl.h"
#include "../repository/TranslationRepositoryImpl.h"
#include "../local/DatabaseManager.h"

#include "domain/usecase/GetConversationsUseCase.h"
#include "domain/usecase/GetMessagesUseCase.h"
#include "domain/usecase/SendMessageUseCase.h"
#include "domain/usecase/TranslateUseCase.h"

// 静态成员定义
std::shared_ptr<data::local::LocalChatDataSource> AppContainer::s_localChatDataSource = nullptr;
std::shared_ptr<data::remote::RemoteAiDataSource> AppContainer::s_remoteAiDataSource = nullptr;
std::shared_ptr<data::remote::RemoteTranslationDataSource> AppContainer::s_remoteTranslationDataSource = nullptr;

std::shared_ptr<domain::repository::IChatRepository> AppContainer::s_chatRepository = nullptr;
std::shared_ptr<domain::repository::ITranslationRepository> AppContainer::s_translationRepository = nullptr;

std::shared_ptr<domain::usecase::GetConversationsUseCase> AppContainer::s_getConversationsUseCase = nullptr;
std::shared_ptr<domain::usecase::GetMessagesUseCase> AppContainer::s_getMessagesUseCase = nullptr;
std::shared_ptr<domain::usecase::SendMessageUseCase> AppContainer::s_sendMessageUseCase = nullptr;
std::shared_ptr<domain::usecase::TranslateUseCase> AppContainer::s_translateUseCase = nullptr;

#include "../remote/TcpChatClient.h"

void AppContainer::init(const QString& userId)
{
    // 初始化数据库
    DatabaseManager::instance().init(QString("aura_chat_%1.db").arg(userId));

    // 连接服务端并模拟当前用户身份。
    // 首次 connected 时认证即可；重连后由 TcpChatClient::onConnected 自动重认证。
    // 这里用共享指针保存 connection，在 lambda 内自断以模拟 SingleShot。
    auto& tcp = TcpChatClient::instance();
    auto* connHolder = new QMetaObject::Connection();
    *connHolder = QObject::connect(&tcp, &TcpChatClient::connected, [userId, connHolder]() {
        TcpChatClient::instance().authenticate(userId);
        QObject::disconnect(*connHolder); // 仅触发一次，后续重连由 TcpChatClient 内部处理
        delete connHolder;
    });
    tcp.connectToServer("127.0.0.1", 8080);

    // 1. 初始化 DataSources
    s_localChatDataSource = std::make_shared<data::local::LocalChatDataSource>(userId);
    s_remoteAiDataSource = std::make_shared<data::remote::RemoteAiDataSource>();
    s_remoteTranslationDataSource = std::make_shared<data::remote::RemoteTranslationDataSource>(s_remoteAiDataSource);

    // 2. 初始化 Repositories
    s_chatRepository = std::make_shared<data::repository::ChatRepositoryImpl>(
        s_localChatDataSource, s_remoteAiDataSource);
    s_translationRepository = std::make_shared<data::repository::TranslationRepositoryImpl>(
        s_remoteTranslationDataSource);

    // 3. 初始化 UseCases
    s_getConversationsUseCase = std::make_shared<domain::usecase::GetConversationsUseCase>(s_chatRepository);
    s_getMessagesUseCase = std::make_shared<domain::usecase::GetMessagesUseCase>(s_chatRepository);
    s_sendMessageUseCase = std::make_shared<domain::usecase::SendMessageUseCase>(s_chatRepository);
    s_translateUseCase = std::make_shared<domain::usecase::TranslateUseCase>(s_translationRepository);
}

void AppContainer::shutdown()
{
    s_getConversationsUseCase.reset();
    s_getMessagesUseCase.reset();
    s_sendMessageUseCase.reset();
    s_translateUseCase.reset();

    s_chatRepository.reset();
    s_translationRepository.reset();

    s_localChatDataSource.reset();
    s_remoteAiDataSource.reset();
    s_remoteTranslationDataSource.reset();
}

std::shared_ptr<domain::usecase::GetConversationsUseCase> AppContainer::getConversationsUseCase()
{
    if (!s_getConversationsUseCase) {
        init("default");
    }
    return s_getConversationsUseCase;
}

std::shared_ptr<domain::usecase::GetMessagesUseCase> AppContainer::getMessagesUseCase()
{
    if (!s_getMessagesUseCase) {
        init("default");
    }
    return s_getMessagesUseCase;
}

std::shared_ptr<domain::usecase::SendMessageUseCase> AppContainer::getSendMessageUseCase()
{
    if (!s_sendMessageUseCase) {
        init("default");
    }
    return s_sendMessageUseCase;
}

std::shared_ptr<domain::usecase::TranslateUseCase> AppContainer::getTranslateUseCase()
{
    if (!s_translateUseCase) {
        init("default");
    }
    return s_translateUseCase;
}
  
