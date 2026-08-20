cmake_minimum_required(VERSION 3.16)
project(Aura LANGUAGES CXX)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network Sql)

qt_standard_project_setup()

add_subdirectory(server)

add_subdirectory(third_party/FluentQt)

include(FetchContent)
set(QWINDOWKIT_LOCAL_SOURCE "F:/source/homework/AIChatDraw/3rdparty/qwindowkit")
if(EXISTS "${QWINDOWKIT_LOCAL_SOURCE}/CMakeLists.txt")
    add_subdirectory("${QWINDOWKIT_LOCAL_SOURCE}" "${CMAKE_BINARY_DIR}/qwindowkit")
else()
    FetchContent_Declare(
        qwindowkit
        GIT_REPOSITORY https://github.com/stdware/qwindowkit.git
        GIT_TAG        1.5.0
    )
    FetchContent_MakeAvailable(qwindowkit)
endif()

set(PROJECT_SOURCES
    src/main.cpp

    src/ui/theme/ThemeSettings.h
    src/ui/theme/ThemeSettings.cpp

    src/ui/window/WindowBase.h
    src/ui/window/WindowBase.cpp
    src/ui/window/TitleBar.h
    src/ui/window/TitleBar.cpp
    src/ui/window/LoginWindow.h
    src/ui/window/LoginWindow.cpp

    src/ui/animation/AnimatedVisualSource.h
    src/ui/animation/AnimatedBackVisualSource.h
    src/ui/animation/AnimatedGlobalNavVisualSource.h
    src/ui/animation/AnimatedSettingsVisualSource.h
    src/ui/animation/AnimatedIcon.h
    src/ui/animation/AnimatedIcon.cpp

    # Domain Layer
    src/domain/model/Conversation.h
    src/domain/model/Message.h
    src/domain/repository/IChatRepository.h
    src/domain/repository/ITranslationRepository.h
    src/domain/usecase/GetConversationsUseCase.h
    src/domain/usecase/GetConversationsUseCase.cpp
    src/domain/usecase/GetMessagesUseCase.h
    src/domain/usecase/GetMessagesUseCase.cpp
    src/domain/usecase/SendMessageUseCase.h
    src/domain/usecase/SendMessageUseCase.cpp
    src/domain/usecase/TranslateUseCase.h
    src/domain/usecase/TranslateUseCase.cpp

    # Data Layer
    src/data/local/DatabaseManager.h
    src/data/local/DatabaseManager.cpp
    src/data/local/LocalChatDataSource.h
    src/data/local/LocalChatDataSource.cpp
    src/data/remote/TcpChatClient.h
    src/data/remote/TcpChatClient.cpp
    src/data/remote/RemoteAiDataSource.h
    src/data/remote/RemoteAiDataSource.cpp
    src/data/remote/RemoteTranslationDataSource.h
    src/data/remote/RemoteTranslationDataSource.cpp
    src/data/repository/ChatRepositoryImpl.h
    src/data/repository/ChatRepositoryImpl.cpp
    src/data/repository/TranslationRepositoryImpl.h
    src/data/repository/TranslationRepositoryImpl.cpp
    src/data/di/AppContainer.h
    src/data/di/AppContainer.cpp

    src/ui/common/BaseViewModel.h

    src/ui/screen/main/MainViewModel.h
    src/ui/screen/main/MainViewModel.cpp
    src/ui/screen/main/MainWindow.h
    src/ui/screen/main/MainWindow.cpp
    src/ui/navigation/PageId.h
    src/ui/navigation/AppNavigationPanel.h
    src/ui/navigation/AppNavigationPanel.cpp
    src/ui/navigation/AppNavFooter.h
    src/ui/navigation/AppNavFooter.cpp
    src/ui/widget/PlaceholderView.h
    src/ui/widget/PlaceholderView.cpp

    src/utils/CryptoUtils.h
    src/utils/CryptoUtils.cpp
    src/utils/ImageUtils.h
    src/utils/ImageUtils.cpp

    src/ui/screen/chat/ChatViewModel.h
    src/ui/screen/chat/ChatViewModel.cpp
    src/ui/screen/chat/ChatPage.h
    src/ui/screen/chat/ChatPage.cpp
    src/ui/screen/chat/MessageListModel.h
    src/ui/screen/chat/MessageListModel.cpp
    src/ui/screen/chat/MessageBubbleDelegate.h
    src/ui/screen/chat/MessageBubbleDelegate.cpp
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network
        Qt::Sql
        FluentQt::FluentQt
        QWindowKit::Widgets
)
