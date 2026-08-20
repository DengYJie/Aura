#pragma once

#include <QWidget>

#include <FluentQt/Foundation.h>

#include "PageId.h"
#include "../screen/main/MainViewModel.h"

namespace ui::navigation {

using ui::screen::main::MainState;
using ui::screen::main::MainViewModel;

class NavFooterButton : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
    Q_OBJECT
public:
    explicit NavFooterButton(const QString& text, const QString& glyph, QWidget* parent = nullptr);

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
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
    QString m_text;
    QString m_glyph;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_selected = false;
};

class AppNavFooter : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
    Q_OBJECT

public:
    explicit AppNavFooter(QWidget* parent = nullptr);

    void setViewModel(MainViewModel* viewModel);
    void renderState(const MainState& state);

    void onThemeUpdated() override;
    QSize sizeHint() const override;

signals:
    void pageRequested(PageId pageId);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    NavFooterButton* m_settingsButton = nullptr;
    MainViewModel* m_viewModel = nullptr;
};

}  // namespace ui::navigation
