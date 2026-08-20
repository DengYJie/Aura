#pragma once
#include <QString>
#include <QHash>
#include <cstdint>

struct ServerNetConfig {
    QString host = QStringLiteral("0.0.0.0");
    quint16 port = 8080;
    QString databasePath;
};

struct AiContextConfig {
    int maxTurns = 12;
    int maxChars = 4000;
};

struct AiProviderConfig {
    QString type = QStringLiteral("openai_compatible");
    QString baseUrl;
    QString model;
    QString apiKey;
    double  temperature = 0.7;
    int     maxTokens = 2048;
};

struct ServerConfig {
    ServerNetConfig                  server;
    AiContextConfig                  context;
    QHash<QString, AiProviderConfig> providers;
    QHash<QString, QString>          routing;
    QHash<QString, QString>          prompts;

    static ServerConfig loadFromFile(const QString& configPath, bool* ok = nullptr);
};
