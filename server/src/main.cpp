#include <cstdio>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QtMessageHandler>
#ifdef Q_OS_WIN
#  include <windows.h>
#endif
#include "core/ServerConfig.h"
#include "core/ChatServer.h"
#include "ai/AiModelRouter.h"

static void auraMessageHandler(QtMsgType type, const QMessageLogContext& /*ctx*/, const QString& msg)
{
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    const char* level = nullptr;

    switch (type) {
    case QtDebugMsg:    level = " INFO "; break;
    case QtInfoMsg:     level = " INFO "; break;
    case QtWarningMsg:  level = " WARN "; break;
    case QtCriticalMsg: level = "ERROR "; break;
    case QtFatalMsg:    level = "FATAL "; break;
    }

    std::fprintf(stderr, "[%s][%s] %s\n",
        ts.toUtf8().constData(),
        level,
        msg.toUtf8().constData());

    if (type == QtFatalMsg) {
        std::abort();
    }
}

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    qInstallMessageHandler(auraMessageHandler);

    QCoreApplication a(argc, argv);
    a.setApplicationName(QStringLiteral("AuraServer"));

    // 配置文件与 exe 同目录，或通过命令行参数指定
    const QString configPath = (argc > 1)
        ? QString::fromLocal8Bit(argv[1])
        : QDir(QCoreApplication::applicationDirPath()).filePath("server_config.json");

    // 1. 全局配置解析
    const ServerConfig config = ServerConfig::loadFromFile(configPath);

    // 2. 初始化 AI 路由子系统
    AiModelRouter::instance().init(config);

    // 3. 启动 TCP 聊天服务端
    ChatServer server(config.server.databasePath);
    if (!server.startServer(config.server.host, config.server.port)) {
        return -1;
    }

    return a.exec();
}

