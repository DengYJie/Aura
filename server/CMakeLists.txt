cmake_minimum_required(VERSION 3.16)
project(AuraServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Network Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql)

set(SERVER_SOURCES
    src/main.cpp
    src/core/ServerConfig.h
    src/core/ServerConfig.cpp
    src/core/ChatServer.h
    src/core/ChatServer.cpp
    src/core/ClientSession.h
    src/core/ClientSession.cpp
    src/core/ServerDatabase.h
    src/core/ServerDatabase.cpp
    src/ai/IAiProvider.h
    src/ai/OpenAICompatibleClient.h
    src/ai/OpenAICompatibleClient.cpp
    src/ai/AiModelRouter.h
    src/ai/AiModelRouter.cpp
)

qt_add_executable(${PROJECT_NAME} ${SERVER_SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt::Core
        Qt::Network
        Qt::Sql
)

# 若源码根目录下不存在 server_config.json，则自动从 .example 复制一份
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/server_config.json")
    if(EXISTS "${CMAKE_SOURCE_DIR}/server_config.json.example")
        file(COPY_FILE
            "${CMAKE_SOURCE_DIR}/server_config.json.example"
            "${CMAKE_SOURCE_DIR}/server_config.json"
        )
        message(STATUS "Created server_config.json from server_config.json.example")
    endif()
endif()

# 构建后自动同步 server_config.json 到可执行文件输出目录
if(EXISTS "${CMAKE_SOURCE_DIR}/server_config.json")
    set(CONFIG_SRC_FILE "${CMAKE_SOURCE_DIR}/server_config.json")
else()
    set(CONFIG_SRC_FILE "${CMAKE_SOURCE_DIR}/server_config.json.example")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONFIG_SRC_FILE}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/server_config.json"
    COMMENT "Ensuring server_config.json exists in output directory"
)
