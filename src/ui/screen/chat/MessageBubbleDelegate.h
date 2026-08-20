#pragma once

#include <QStyledItemDelegate>
#include <FluentQt/Foundation.h>

namespace ui::screen::chat {

class MessageBubbleDelegate : public QStyledItemDelegate, public fluent::FluentElement {
    Q_OBJECT

public:
    explicit MessageBubbleDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter,
               const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
    bool helpEvent(QHelpEvent* event,
                   QAbstractItemView* view,
                   const QStyleOptionViewItem& option,
                   const QModelIndex& index) override;
    bool editorEvent(QEvent* event,
                     QAbstractItemModel* model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

    bool hitTestRetry(const QModelIndex& index, const QPoint& pos, const QRect& visualRect) const;

signals:
    void retryClicked(const QString& messageId);

private:
    QTimer* m_animTimer = nullptr;
};

}  // namespace ui::screen::chat
