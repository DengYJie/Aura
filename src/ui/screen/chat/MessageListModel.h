#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace ui::screen::chat {

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

struct MessageItemState {
    QString id;
    MessageSenderType senderType = MessageSenderType::Assistant;
    QString senderName;
    QString timestamp;
    QString rawText;
    MessageRenderMode renderMode = MessageRenderMode::PlainText;
    MessageSentimentTag sentimentTag = MessageSentimentTag::None;
    MessageDeliveryStatus deliveryStatus = MessageDeliveryStatus::Sent;
    bool isStreaming = false;

    bool operator==(const MessageItemState& other) const
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

class MessageListModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        SenderTypeRole,
        SenderNameRole,
        TimestampRole,
        RawTextRole,
        RenderModeRole,
        SentimentTagRole,
        StreamingRole,
        DeliveryStatusRole,
    };

    explicit MessageListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMessages(QVector<MessageItemState> messages);
    void appendMessage(const MessageItemState& message);
    bool updateMessageText(int row, const QString& rawText, bool streaming, MessageRenderMode renderMode);

    const QVector<MessageItemState>& messages() const { return m_messages; }
    MessageItemState itemAt(int row) const {
        if (row >= 0 && row < m_messages.size()) {
            return m_messages[row];
        }
        return {};
    }

private:
    QVector<MessageItemState> m_messages;
};

}  // namespace ui::screen::chat
