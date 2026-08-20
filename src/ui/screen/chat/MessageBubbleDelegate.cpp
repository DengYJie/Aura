#include "MessageBubbleDelegate.h"

#include "MessageListModel.h"

#include <FluentQt/Design.h>
#include <FluentQt/Foundation.h>

#include <QDateTime>
#include <QHelpEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTextOption>
#include <QTimer>
#include <QToolTip>
#include <algorithm>

namespace ui::screen::chat {

namespace {

constexpr int kOuterHorizontal = 18;
constexpr int kOuterVertical = 8;
constexpr int kBubblePaddingX = 16;
constexpr int kBubblePaddingY = 14;
constexpr int kHeaderGap = 6;
constexpr int kBubbleGap = 8;
constexpr qreal kBubbleMaxWidthRatio = 0.64;

QString extractBodyHtml(QString html)
{
    static const QRegularExpression bodyPattern(
        QStringLiteral(R"(<body[^>]*>([\s\S]*)</body>)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = bodyPattern.match(html);
    if (match.hasMatch()) {
        html = match.captured(1);
    }

    html.remove(QRegularExpression(QStringLiteral(R"(background-color\s*:\s*[^;"]+;?)"),
                                   QRegularExpression::CaseInsensitiveOption));
    html.remove(QRegularExpression(QStringLiteral(R"(color\s*:\s*#[0-9A-Fa-f]+;?)"),
                                   QRegularExpression::CaseInsensitiveOption));
    return html;
}

QString escapeHtml(QString text)
{
    return text.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
}

QColor sentimentColor(MessageSentimentTag tag, const fluent::FluentElement::Colors& colors)
{
    switch (tag) {
    case MessageSentimentTag::Positive:
        return colors.systemSuccess;
    case MessageSentimentTag::Neutral:
        return colors.systemCaution;
    case MessageSentimentTag::Negative:
        return colors.systemCritical;
    case MessageSentimentTag::None:
        return QColor(Qt::transparent);
    }
    return QColor(Qt::transparent);
}

QString sentimentEmoji(MessageSentimentTag tag)
{
    switch (tag) {
    case MessageSentimentTag::Positive:
        return QStringLiteral("😃");
    case MessageSentimentTag::Neutral:
        return QStringLiteral("😐");
    case MessageSentimentTag::Negative:
        return QStringLiteral("😔");
    case MessageSentimentTag::None:
        return QString();
    }
    return QString();
}

QString sentimentText(MessageSentimentTag tag)
{
    switch (tag) {
    case MessageSentimentTag::Positive:
        return QStringLiteral("😃 积极情绪");
    case MessageSentimentTag::Neutral:
        return QStringLiteral("😐 中性情绪");
    case MessageSentimentTag::Negative:
        return QStringLiteral("😔 消极情绪");
    case MessageSentimentTag::None:
        return QString();
    }
    return QString();
}

QString buildStyledHtml(const QString& rawText,
                        MessageRenderMode renderMode,
                        bool streaming,
                        const fluent::FluentElement::Colors& colors,
                        bool isSelf)
{
    const QString textColor = isSelf ? colors.textOnAccent.name(QColor::HexArgb)
                                     : colors.textPrimary.name(QColor::HexArgb);
    const QString inlineCodeBg = isSelf ? QColor(255, 255, 255, 45).name(QColor::HexArgb)
                                        : colors.subtleSecondary.name(QColor::HexArgb);
    const QString blockBg = isSelf ? QColor(255, 255, 255, 30).name(QColor::HexArgb)
                                   : colors.bgCanvas.name(QColor::HexArgb);
    const QString blockBorder = isSelf ? QColor(255, 255, 255, 60).name(QColor::HexArgb)
                                       : colors.strokeCard.name(QColor::HexArgb);
    const QString linkColor = isSelf ? colors.textOnAccent.name(QColor::HexArgb)
                                     : colors.textAccentPrimary.name(QColor::HexArgb);
    const QString secondaryText = isSelf ? colors.textOnAccent.name(QColor::HexArgb)
                                         : colors.textSecondary.name(QColor::HexArgb);
    const QString accentBorder = isSelf ? colors.textOnAccent.name(QColor::HexArgb)
                                        : colors.accentSecondary.name(QColor::HexArgb);

    QString htmlBody;
    if (renderMode == MessageRenderMode::Image) {
        htmlBody = QStringLiteral("<img src=\"%1\" width=\"200\" />").arg(rawText);
    } else if (renderMode == MessageRenderMode::Markdown && !streaming) {
        QTextDocument markdownDoc;
        markdownDoc.setMarkdown(rawText);
        htmlBody = extractBodyHtml(markdownDoc.toHtml());
    } else {
        htmlBody = QStringLiteral("<p>%1</p>").arg(escapeHtml(rawText));
    }

    return QStringLiteral(
               "<html><head><style>"
               "body { color:%1; margin:0; font-family:'Segoe UI'; font-size:14px; background:transparent; }"
               "p { margin:0 0 8px 0; }"
               "ul, ol { margin:4px 0 8px 20px; }"
               "li { margin:0 0 4px 0; }"
               "blockquote { margin:8px 0; padding:4px 0 4px 12px; border-left:3px solid %6; color:%5; }"
               "pre { white-space:pre-wrap; background:%3; border:1px solid %4; border-radius:10px; padding:12px; margin:10px 0; }"
               "code { background:%2; padding:1px 5px; border-radius:5px; font-family:'Consolas'; }"
               "a { color:%7; text-decoration:none; }"
               "</style></head><body>%8</body></html>")
        .arg(textColor,
             inlineCodeBg,
             blockBg,
             blockBorder,
             secondaryText,
             accentBorder,
             linkColor,
             htmlBody);
}

struct BubbleLayout {
    QRect bubbleRect;
    QRect contentRect;
    QRect senderRect;
    QRect timeRect;
    QRect sentimentRect;
    QRect retryRect;
    bool centerSystem = false;
};

BubbleLayout buildLayout(const QStyleOptionViewItem& option,
                         const MessageItemState& message,
                         const QSizeF& contentSize)
{
    BubbleLayout layout;
    const bool isSelf = message.senderType == MessageSenderType::Self;
    const bool isSystem = message.senderType == MessageSenderType::System || message.renderMode == MessageRenderMode::System;
    layout.centerSystem = isSystem;

    int optWidth = option.rect.width();
    if (optWidth <= 0 && option.widget) {
        if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
            optWidth = view->viewport()->width();
        } else {
            optWidth = option.widget->width();
        }
    }
    if (optWidth <= 0) {
        optWidth = 600;
    }

    if (isSystem) {
        const int badgeWidth = qMin(optWidth - 32, qMax(140, qRound(contentSize.width()) + 32));
        const int badgeHeight = qRound(contentSize.height()) + 18;
        layout.bubbleRect = QRect(option.rect.center().x() - badgeWidth / 2,
                                  option.rect.top() + kOuterVertical,
                                  badgeWidth,
                                  badgeHeight);
        layout.contentRect = layout.bubbleRect.adjusted(14, 8, -14, -8);
        return layout;
    }

    QFont captionFont = Typography::fontStyle(Typography::FontRole::Caption).toQFont();
    QFontMetrics captionFm(captionFont);
    const int senderHeight = 18;
    const int timeHeight = 16;
    const bool isPeer = message.senderType == MessageSenderType::Peer;
    const int sentimentWidth = (isPeer && message.sentimentTag != MessageSentimentTag::None) ? 22 : 0;

    QString timeDisplayText = message.isStreaming ? QStringLiteral("正在生成...") : message.timestamp;
    if (message.senderType == MessageSenderType::Self) {
        if (message.deliveryStatus == MessageDeliveryStatus::Sending) {
            timeDisplayText = message.timestamp + QStringLiteral(" · 发送中");
        } else if (message.deliveryStatus == MessageDeliveryStatus::Failed) {
            timeDisplayText = message.timestamp + QStringLiteral(" · 发送失败");
        }
    }

    const int timeTextWidth = captionFm.horizontalAdvance(timeDisplayText);
    const int senderTextWidth = captionFm.horizontalAdvance(message.senderName) + sentimentWidth;
    const int minHeaderFooterWidth = qMax(timeTextWidth, senderTextWidth) + (kBubblePaddingX * 2) + 8;

    const int bubbleWidth = qMin(qRound(optWidth * kBubbleMaxWidthRatio),
                                 std::max({130, minHeaderFooterWidth, qRound(contentSize.width()) + kBubblePaddingX * 2}));
    const int bubbleHeight = senderHeight + kHeaderGap + qRound(contentSize.height()) + kBubblePaddingY * 2 + kBubbleGap + timeHeight;

    const int bubbleX = isSelf
        ? option.rect.right() - kOuterHorizontal - bubbleWidth + 1
        : option.rect.left() + kOuterHorizontal;
    const int bubbleY = option.rect.top() + kOuterVertical;

    layout.bubbleRect = QRect(bubbleX, bubbleY, bubbleWidth, bubbleHeight);
    if (isSelf && (message.deliveryStatus == MessageDeliveryStatus::Failed || message.deliveryStatus == MessageDeliveryStatus::Sending)) {
        layout.retryRect = QRect(bubbleX - 26, bubbleY + (bubbleHeight - 20) / 2, 20, 20);
    }
    layout.senderRect = QRect(bubbleX + kBubblePaddingX,
                              bubbleY + 8,
                              bubbleWidth - (kBubblePaddingX * 2) - sentimentWidth,
                              senderHeight);
    layout.sentimentRect = QRect(layout.senderRect.right() + 4,
                                 layout.senderRect.top(),
                                 sentimentWidth,
                                 senderHeight);
    layout.contentRect = QRect(bubbleX + kBubblePaddingX,
                                layout.senderRect.bottom() + 1 + kHeaderGap,
                                bubbleWidth - (kBubblePaddingX * 2),
                                qRound(contentSize.height()));
    layout.timeRect = QRect(bubbleX + kBubblePaddingX,
                            layout.bubbleRect.bottom() - timeHeight - 8,
                            bubbleWidth - (kBubblePaddingX * 2),
                            timeHeight);
    return layout;
}

void configureDocument(QTextDocument& document,
                       const MessageItemState& message,
                       qreal textWidth,
                       const fluent::FluentElement::Colors& colors)
{
    const bool isSelf = message.senderType == MessageSenderType::Self;
    document.setDocumentMargin(0);
    document.setUndoRedoEnabled(false);
    document.setDefaultFont(Typography::fontStyle(Typography::FontRole::Body).toQFont());
    document.setTextWidth(textWidth);
    document.setHtml(buildStyledHtml(message.rawText, message.renderMode, message.isStreaming, colors, isSelf));

    QTextOption option = document.defaultTextOption();
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document.setDefaultTextOption(option);
}

MessageItemState itemFromIndex(const QModelIndex& index)
{
    MessageItemState message;
    message.id = index.data(MessageListModel::IdRole).toString();
    message.senderType = static_cast<MessageSenderType>(index.data(MessageListModel::SenderTypeRole).toInt());
    message.senderName = index.data(MessageListModel::SenderNameRole).toString();
    message.timestamp = index.data(MessageListModel::TimestampRole).toString();
    message.rawText = index.data(MessageListModel::RawTextRole).toString();
    message.renderMode = static_cast<MessageRenderMode>(index.data(MessageListModel::RenderModeRole).toInt());
    message.sentimentTag = static_cast<MessageSentimentTag>(index.data(MessageListModel::SentimentTagRole).toInt());
    message.deliveryStatus = static_cast<MessageDeliveryStatus>(index.data(MessageListModel::DeliveryStatusRole).toInt());
    message.isStreaming = index.data(MessageListModel::StreamingRole).toBool();
    return message;
}

}  // namespace

MessageBubbleDelegate::MessageBubbleDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(40);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        if (auto* view = qobject_cast<QAbstractItemView*>(this->parent())) {
            view->viewport()->update();
        }
    });
    m_animTimer->start();
}

QSize MessageBubbleDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    int availableWidth = option.rect.width();
    if (availableWidth <= 0 && option.widget) {
        if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
            availableWidth = view->viewport()->width();
        } else {
            availableWidth = option.widget->width();
        }
    }
    if (availableWidth <= 0) {
        availableWidth = 600;
    }

    const MessageItemState message = itemFromIndex(index);
    const auto& colors = themeColorsRef();
    QTextDocument document;
    const int maxTextWidth = qMax(120, qRound(availableWidth * kBubbleMaxWidthRatio) - (kBubblePaddingX * 2));
    configureDocument(document, message, maxTextWidth, colors);

    QStyleOptionViewItem normalizedOption = option;
    if (normalizedOption.rect.width() <= 0) {
        normalizedOption.rect.setWidth(availableWidth);
    }

    const QSizeF contentSize(qMin<qreal>(maxTextWidth, document.idealWidth()), document.size().height());
    const BubbleLayout layout = buildLayout(normalizedOption, message, contentSize);
    return QSize(availableWidth, layout.bubbleRect.height() + (kOuterVertical * 2));
}

void MessageBubbleDelegate::paint(QPainter* painter,
                                  const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    int availableWidth = option.rect.width();
    if (availableWidth <= 0 && option.widget) {
        if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
            availableWidth = view->viewport()->width();
        } else {
            availableWidth = option.widget->width();
        }
    }
    if (availableWidth <= 0) {
        availableWidth = 600;
    }

    const MessageItemState message = itemFromIndex(index);
    const auto& colors = themeColorsRef();
    QTextDocument document;
    const int maxTextWidth = qMax(120, qRound(availableWidth * kBubbleMaxWidthRatio) - (kBubblePaddingX * 2));
    configureDocument(document, message, maxTextWidth, colors);

    QStyleOptionViewItem normalizedOption = option;
    if (normalizedOption.rect.width() <= 0) {
        normalizedOption.rect.setWidth(availableWidth);
    }

    const QSizeF contentSize(qMin<qreal>(maxTextWidth, document.idealWidth()), document.size().height());
    const BubbleLayout layout = buildLayout(normalizedOption, message, contentSize);

    QColor bubbleFill = colors.bgCanvas;
    QColor bubbleStroke = colors.strokeCard;
    QColor titleColor = colors.textSecondary;
    QColor timeColor = colors.textTertiary;

    if (message.senderType == MessageSenderType::Self) {
        bubbleFill = colors.accentDefault;
        bubbleStroke = colors.accentDefault;
        titleColor = colors.textOnAccent;
        timeColor = colors.textOnAccent;
    } else if (message.senderType == MessageSenderType::Assistant) {
        bubbleFill = colors.bgLayerAlt;
        bubbleStroke = colors.strokeSurface;
    } else if (layout.centerSystem) {
        bubbleFill = colors.subtleSecondary;
        bubbleStroke = QColor(Qt::transparent);
    }

    painter->setPen(QPen(bubbleStroke, message.senderType == MessageSenderType::Self ? 0 : 1));
    painter->setBrush(bubbleFill);
    painter->drawRoundedRect(layout.bubbleRect.adjusted(0, 0, -1, -1),
                             layout.centerSystem ? 12.0 : 16.0,
                             layout.centerSystem ? 12.0 : 16.0);

    if (!layout.centerSystem) {
        painter->setPen(titleColor);
        painter->setFont(Typography::fontStyle(Typography::FontRole::Caption).toQFont());
        painter->drawText(layout.senderRect,
                          Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                          message.senderName);

        // 仅对真人好友 (Peer) 绘制情绪 Emoji 表情
        if (message.senderType == MessageSenderType::Peer && message.sentimentTag != MessageSentimentTag::None) {
            QFont emojiFont = Typography::fontStyle(Typography::FontRole::Caption).toQFont();
            emojiFont.setPointSize(11);
            painter->setFont(emojiFont);
            painter->setPen(titleColor);
            painter->drawText(layout.sentimentRect,
                              Qt::AlignCenter,
                              sentimentEmoji(message.sentimentTag));
        }

        QString timeDisplayText = message.isStreaming ? QStringLiteral("正在生成...") : message.timestamp;
        if (message.senderType == MessageSenderType::Self) {
            if (message.deliveryStatus == MessageDeliveryStatus::Sending) {
                timeDisplayText = message.timestamp + QStringLiteral(" · 发送中");
            } else if (message.deliveryStatus == MessageDeliveryStatus::Failed) {
                timeDisplayText = message.timestamp + QStringLiteral(" · 发送失败");
            }
        }

        painter->setPen(timeColor);
        painter->drawText(layout.timeRect,
                          message.senderType == MessageSenderType::Self
                              ? Qt::AlignRight | Qt::AlignVCenter
                              : Qt::AlignLeft | Qt::AlignVCenter,
                          timeDisplayText);
    }

    // 绘制发送中加载动画圆环 (Loading Spinner)
    if (message.senderType == MessageSenderType::Self && message.deliveryStatus == MessageDeliveryStatus::Sending && layout.retryRect.isValid()) {
        painter->setPen(QPen(colors.accentDefault, 2.2));
        painter->setBrush(Qt::NoBrush);
        const int startAngle = static_cast<int>((QDateTime::currentMSecsSinceEpoch() / 3) % 360) * 16;
        painter->drawArc(layout.retryRect.adjusted(3, 3, -3, -3), -startAngle, 260 * 16);
    }
    // 绘制发送失败红色感叹号圆标
    else if (message.senderType == MessageSenderType::Self && message.deliveryStatus == MessageDeliveryStatus::Failed && layout.retryRect.isValid()) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#D13438"));
        painter->drawEllipse(layout.retryRect);
        painter->setPen(Qt::white);
        QFont alertFont = Typography::fontStyle(Typography::FontRole::Caption).toQFont();
        alertFont.setBold(true);
        alertFont.setPointSize(10);
        painter->setFont(alertFont);
        painter->drawText(layout.retryRect, Qt::AlignCenter, QStringLiteral("!"));
    }

    painter->translate(layout.contentRect.topLeft());
    document.drawContents(painter, QRectF(QPointF(0, 0), layout.contentRect.size()));
    painter->restore();
}

bool MessageBubbleDelegate::helpEvent(QHelpEvent* event,
                                      QAbstractItemView* view,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index)
{
    if (!event || !view) {
        return false;
    }

    const MessageItemState message = itemFromIndex(index);
    const auto& colors = themeColorsRef();
    QTextDocument document;
    configureDocument(document,
                      message,
                      qMax(120, qRound(option.rect.width() * kBubbleMaxWidthRatio) - (kBubblePaddingX * 2)),
                      colors);
    const BubbleLayout layout = buildLayout(option, message, document.size());

    if (message.senderType == MessageSenderType::Peer && message.sentimentTag != MessageSentimentTag::None) {
        if (layout.sentimentRect.contains(event->pos())) {
            QToolTip::showText(event->globalPos(), sentimentText(message.sentimentTag), view, layout.sentimentRect);
            return true;
        }
    }

    if (message.senderType == MessageSenderType::Self && message.deliveryStatus == MessageDeliveryStatus::Failed) {
        if (layout.retryRect.isValid() && layout.retryRect.contains(event->pos())) {
            QToolTip::showText(event->globalPos(), QStringLiteral("发送失败，点击重新发送"), view, layout.retryRect);
            return true;
        }
    }

    return false;
}

bool MessageBubbleDelegate::editorEvent(QEvent* event,
                                        QAbstractItemModel* model,
                                        const QStyleOptionViewItem& option,
                                        const QModelIndex& index)
{
    if (!event || !index.isValid()) {
        return false;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            const MessageItemState message = itemFromIndex(index);
            if (message.senderType == MessageSenderType::Self && message.deliveryStatus == MessageDeliveryStatus::Failed) {
                const auto& colors = themeColorsRef();
                QTextDocument document;
                const int maxTextWidth = qMax(120, qRound(option.rect.width() * kBubbleMaxWidthRatio) - (kBubblePaddingX * 2));
                configureDocument(document, message, maxTextWidth, colors);

                const BubbleLayout layout = buildLayout(option, message, document.size());
                if (layout.retryRect.isValid() && layout.retryRect.contains(mouseEvent->pos())) {
                    emit retryClicked(message.id);
                    return true;
                }
            }
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

bool MessageBubbleDelegate::hitTestRetry(const QModelIndex& index, const QPoint& pos, const QRect& visualRect) const
{
    if (!index.isValid()) {
        return false;
    }
    const MessageItemState message = itemFromIndex(index);
    if (message.senderType != MessageSenderType::Self || message.deliveryStatus != MessageDeliveryStatus::Failed) {
        return false;
    }

    const auto& colors = themeColorsRef();
    QTextDocument document;
    const int maxTextWidth = qMax(120, qRound(visualRect.width() * kBubbleMaxWidthRatio) - (kBubblePaddingX * 2));
    configureDocument(document, message, maxTextWidth, colors);

    QStyleOptionViewItem option;
    option.rect = visualRect;
    const QSizeF contentSize(qMin<qreal>(maxTextWidth, document.idealWidth()), document.size().height());
    const BubbleLayout layout = buildLayout(option, message, contentSize);

    // 扩大点击判定范围（32x32 px）确保鼠标轻松精准点击
    const QRect hitRect = layout.retryRect.adjusted(-6, -6, 6, 6);
    return hitRect.contains(pos);
}

}  // namespace ui::screen::chat
