#include "MessageListModel.h"

namespace ui::screen::chat {

MessageListModel::MessageListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int MessageListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_messages.size();
}

QVariant MessageListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
        return {};
    }

    const MessageItemState& message = m_messages.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case RawTextRole:
        return message.rawText;
    case IdRole:
        return message.id;
    case SenderTypeRole:
        return static_cast<int>(message.senderType);
    case SenderNameRole:
        return message.senderName;
    case TimestampRole:
        return message.timestamp;
    case RenderModeRole:
        return static_cast<int>(message.renderMode);
    case SentimentTagRole:
        return static_cast<int>(message.sentimentTag);
    case StreamingRole:
        return message.isStreaming;
    case DeliveryStatusRole:
        return static_cast<int>(message.deliveryStatus);
    default:
        return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {SenderTypeRole, "senderType"},
        {SenderNameRole, "senderName"},
        {TimestampRole, "timestamp"},
        {RawTextRole, "rawText"},
        {RenderModeRole, "renderMode"},
        {SentimentTagRole, "sentimentTag"},
        {StreamingRole, "streaming"},
        {DeliveryStatusRole, "deliveryStatus"},
    };
}

void MessageListModel::setMessages(QVector<MessageItemState> messages)
{
    beginResetModel();
    m_messages = std::move(messages);
    endResetModel();
}

void MessageListModel::appendMessage(const MessageItemState& message)
{
    const int row = m_messages.size();
    beginInsertRows(QModelIndex(), row, row);
    m_messages.append(message);
    endInsertRows();
}

bool MessageListModel::updateMessageText(int row,
                                         const QString& rawText,
                                         bool streaming,
                                         MessageRenderMode renderMode)
{
    if (row < 0 || row >= m_messages.size()) {
        return false;
    }

    auto& message = m_messages[row];
    message.rawText = rawText;
    message.isStreaming = streaming;
    message.renderMode = renderMode;

    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, {Qt::DisplayRole, RawTextRole, RenderModeRole, StreamingRole});
    return true;
}

}  // namespace ui::screen::chat
