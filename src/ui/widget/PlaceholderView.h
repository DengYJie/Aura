#pragma once

#include <QWidget>

#include <FluentQt/BasicInput.h>
#include <FluentQt/Foundation.h>

namespace ui::widget {

class PlaceholderView : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
    Q_OBJECT

public:
    explicit PlaceholderView(QWidget* parent = nullptr);

    void onThemeUpdated() override;

protected:
    void paintEvent(QPaintEvent* event) override;
};

}  // namespace ui::widget
