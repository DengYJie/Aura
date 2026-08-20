#pragma once

#include <QString>

namespace domain::model {

enum class MessageSenderType {
    Self,
    Peer,
    Assistant,
    System,
};

enum class MessageRenderMode {
    PlainText,
    Markdown,
    System,
    Image,
};

enum class MessageSentimentTag {
    None,
    Positive,
    Neutral,
    Negative,
};

enum class MessageDeliveryStatus {
    Sending,
    Sent,
    Failed,
};

struct Message {
    QString id;
    MessageSenderType senderType = MessageSenderType::Assistant;
    QString senderName;
    QString timestamp;
    QString rawText;
    MessageRenderMode renderMode = MessageRenderMode::PlainText;
    MessageSentimentTag sentimentTag = MessageSentimentTag::None;
    MessageDeliveryStatus deliveryStatus = MessageDeliveryStatus::Sent;
    bool isStreaming = false;

    bool operator==(const Message& other) const
    {
        return id == other.id &&
               senderType == other.senderType &&
               senderName == other.senderName &&
               timestamp == other.timestamp &&
               rawText == other.rawText &&
               renderMode == other.renderMode &&
               sentimentTag == other.sentimentTag &&
               deliveryStatus == other.deliveryStatus &&
               isStreaming == other.isStreaming;
    }
};

}  // namespace domain::model
