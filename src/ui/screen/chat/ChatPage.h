#pragma once

#include <FluentQt/Collections.h>
#include <FluentQt/Foundation.h>
#include <FluentQt/TextFields.h>
#include <QVector>
#include <QWidget>

#include "ChatViewModel.h"

namespace ui::screen::chat {

class MessageListModel;
class MessageBubbleDelegate;
class ChatHeaderWidget;
class TranslateSidePane;

class ChatPage : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
    Q_OBJECT

public:
    explicit ChatPage(QWidget* parent = nullptr);

    ChatViewModel* viewModel() const { return m_viewModel; }
    void renderState(const ChatState& state);
    void setConversation(const QString& id, const QString& title);

    void onThemeUpdated() override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void scrollToBottom();

    ChatViewModel* m_viewModel = nullptr;
    ChatHeaderWidget* m_headerPanel = nullptr;
    QListView* m_messageView = nullptr;
    MessageListModel* m_model = nullptr;
    MessageBubbleDelegate* m_delegate = nullptr;
    fluent::textfields::TextEdit* m_editor = nullptr;
    fluent::basicinput::Button* m_imageButton = nullptr;
    fluent::basicinput::Button* m_sendButton = nullptr;
    TranslateSidePane* m_translatePane = nullptr;
    QVector<fluent::basicinput::Button*> m_quickReplyButtons;
};

}  // namespace ui::screen::chat
