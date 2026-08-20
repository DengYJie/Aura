#include "ServerConfig.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ServerConfig ServerConfig::loadFromFile(const QString& configPath, bool* ok) {
    ServerConfig config;
    if (ok) *ok = false;

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning().noquote() << QStringLiteral("[CONFIG] Config file not found: %1").arg(configPath);
        return config;
    }

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        qCritical().noquote() << QStringLiteral("[CONFIG] JSON parse error in %1: %2").arg(configPath, parseErr.errorString());
        return config;
    }

    const QJsonObject root = doc.object();

    // 1. server
    if (root.contains("server")) {
        const QJsonObject srv = root["server"].toObject();
        if (srv.contains("host")) config.server.host = srv["host"].toString("0.0.0.0");
        if (srv.contains("port")) config.server.port = static_cast<quint16>(srv["port"].toInt(8080));
        if (srv.contains("database_path")) config.server.databasePath = srv["database_path"].toString();
    }

    // 2. context
    if (root.contains("context")) {
        const QJsonObject ctx = root["context"].toObject();
        if (ctx.contains("max_turns")) config.context.maxTurns = ctx["max_turns"].toInt(12);
        if (ctx.contains("max_chars")) config.context.maxChars = ctx["max_chars"].toInt(4000);
    }

    // 3. providers
    if (root.contains("providers")) {
        const QJsonObject providers = root["providers"].toObject();
        for (auto it = providers.constBegin(); it != providers.constEnd(); ++it) {
            const QJsonObject cfg = it.value().toObject();
            const QString type = cfg["type"].toString("openai_compatible");
            config.providers[it.key()] = {
                type,
                cfg["baseUrl"].toString(),
                cfg["model"].toString(),
                cfg["apiKey"].toString(),
                cfg.contains("temperature") ? cfg["temperature"].toDouble(0.7) : 0.7,
                cfg.contains("max_tokens") ? cfg["max_tokens"].toInt(2048) : 2048
            };
            qInfo().noquote() << QStringLiteral("[CONFIG] Provider: %1 type=%2 model=%3 temp=%4 max_tok=%5")
                                 .arg(it.key(), -10)
                                 .arg(type, -18)
                                 .arg(cfg["model"].toString(), -16)
                                 .arg(config.providers[it.key()].temperature, 0, 'f', 2)
                                 .arg(config.providers[it.key()].maxTokens);
        }
    }

    // 4. routing
    if (root.contains("routing")) {
        const QJsonObject routing = root["routing"].toObject();
        for (auto it = routing.constBegin(); it != routing.constEnd(); ++it) {
            config.routing[it.key()] = it.value().toString();
        }
    }
    if (!config.routing.contains("default") && !config.providers.isEmpty()) {
        config.routing["default"] = config.providers.constBegin().key();
    }

    // 5. prompts
    if (root.contains("prompts")) {
        const QJsonObject prompts = root["prompts"].toObject();
        for (auto it = prompts.constBegin(); it != prompts.constEnd(); ++it) {
            config.prompts[it.key()] = it.value().toString();
        }
    }

    qInfo().noquote() << QStringLiteral("[CONFIG] Loaded from: %1 (%2 providers, %3 routing rules, %4 custom prompts)")
                         .arg(configPath)
                         .arg(config.providers.size())
                         .arg(config.routing.size())
                         .arg(config.prompts.size());

    if (ok) *ok = true;
    return config;
}
