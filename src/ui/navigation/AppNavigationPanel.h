#pragma once

#include <QList>
#include <QVBoxLayout>
#include <QWidget>

#include <FluentQt/Foundation.h>

#include "../screen/main/MainViewModel.h"

namespace ui::navigation {

using ui::screen::main::ConversationData;
using ui::screen::main::MainState;
using ui::screen::main::MainViewModel;

class ConversationItemWidget : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
    Q_OBJECT
public:
    explicit ConversationItemWidget(const ConversationData& data, QWidget* parent = nullptr);

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
    const ConversationData& data() const { return m_data; }

    void onThemeUpdated() override { update(); }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(FluentEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    ConversationData m_data;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_selected = false;
};

class AppNavigationPanel : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
    Q_OBJECT

public:
    explicit AppNavigationPanel(QWidget* parent = nullptr);

    void setViewModel(MainViewModel* viewModel);
    void renderState(const MainState& state);

    void onThemeUpdated() override;
    QSize sizeHint() const override;

signals:
    void conversationSelected(const QString& id);

private:
    void rebuildItems(const QList<ConversationData>& conversations);

    QVBoxLayout* m_listLayout = nullptr;
    QList<ConversationItemWidget*> m_items;
    MainViewModel* m_viewModel = nullptr;
};

}  // namespace ui::navigation
