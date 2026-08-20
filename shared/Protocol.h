#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

namespace Aura {
namespace Protocol {

    enum class MessageType {
        Unknown,
        Auth,           // Client -> Server: Authenticate / Register session
        AuthAck,        // Server -> Client: Auth success/fail
        Chat,           // Client -> Server: Send message
        ChatAck,        // Server -> Client: Message delivered
        IncomingChat,   // Server -> Client: Received message
        StreamChunk,    // Server -> Client: AI streaming chunk
        StreamEnd,      // Server -> Client: AI stream finished
        Error           // Server -> Client: Error occurred
    };

    inline QString typeToString(MessageType type) {
        switch (type) {
            case MessageType::Auth: return "auth";
            case MessageType::AuthAck: return "auth_ack";
            case MessageType::Chat: return "chat";
            case MessageType::ChatAck: return "chat_ack";
            case MessageType::IncomingChat: return "incoming_chat";
            case MessageType::StreamChunk: return "stream_chunk";
            case MessageType::StreamEnd: return "stream_end";
            case MessageType::Error: return "error";
            default: return "unknown";
        }
    }

    inline MessageType stringToType(const QString& str) {
        if (str == "auth") return MessageType::Auth;
        if (str == "auth_ack") return MessageType::AuthAck;
        if (str == "chat") return MessageType::Chat;
        if (str == "chat_ack") return MessageType::ChatAck;
        if (str == "incoming_chat") return MessageType::IncomingChat;
        if (str == "stream_chunk") return MessageType::StreamChunk;
        if (str == "stream_end") return MessageType::StreamEnd;
        if (str == "error") return MessageType::Error;
        return MessageType::Unknown;
    }

    // Helper to create a basic JSON frame
    inline QByteArray createFrame(MessageType type, const QJsonObject& payload = QJsonObject()) {
        QJsonObject frame;
        frame["type"] = typeToString(type);
        frame["payload"] = payload;
        QJsonDocument doc(frame);
        return doc.toJson(QJsonDocument::Compact) + '\n';
    }

    // Helper to parse a JSON frame
    inline bool parseFrame(const QByteArray& data, MessageType& outType, QJsonObject& outPayload) {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            return false;
        }
        
        QJsonObject frame = doc.object();
        outType = stringToType(frame["type"].toString());
        outPayload = frame["payload"].toObject();
        return true;
    }

    inline QJsonObject makeAckPayload(bool success,
                                      const QString& message = {},
                                      const QString& messageId = {})
    {
        QJsonObject payload;
        payload["success"] = success;
        if (!message.isEmpty()) {
            payload["message"] = message;
        }
        if (!messageId.isEmpty()) {
            payload["messageId"] = messageId;
        }
        return payload;
    }

} // namespace Protocol
} // namespace Aura
