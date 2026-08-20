#include "PlaceholderView.h"

#include <FluentQt/TextFields.h>

#include <QHBoxLayout>
#include <QPainter>
#include <QVBoxLayout>

namespace ui::widget {

namespace {

class HeroBadge : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
public:
    explicit HeroBadge(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setFixedSize(96, 96);
    }

    void onThemeUpdated() override
    {
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const auto colors = themeColors();
        painter.setPen(Qt::NoPen);
        painter.setBrush(colors.subtleSecondary);
        painter.drawEllipse(rect().adjusted(0, 0, -1, -1));

        QFont iconFont(Typography::FontFamily::FluentIcons);
        iconFont.setPixelSize(32);
        painter.setFont(iconFont);
        painter.setPen(colors.accentDefault);
        painter.drawText(rect(), Qt::AlignCenter, Typography::Icons::Message);
    }
};

}  // namespace

PlaceholderView::PlaceholderView(QWidget* parent)
    : QWidget(parent)
{
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 24, 24, 24);
    outer->addStretch(1);

    auto* row = new QHBoxLayout;
    row->addStretch(1);

    auto* center = new QWidget(this);
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(12);
    centerLayout->setAlignment(Qt::AlignCenter);

    auto* badge = new HeroBadge(center);
    auto* title = new fluent::textfields::Label(QStringLiteral("Aura AI 智能聊天助手"), center);
    title->setFluentTypography(Typography::FontRole::Title);
    title->setAlignment(Qt::AlignCenter);

    centerLayout->addWidget(badge, 0, Qt::AlignCenter);
    centerLayout->addWidget(title);

    row->addWidget(center, 0, Qt::AlignCenter);
    row->addStretch(1);
    outer->addLayout(row);
    outer->addStretch(1);
}

void PlaceholderView::onThemeUpdated()
{
    update();
}

void PlaceholderView::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), themeColorsRef().bgLayer);
}

}  // namespace ui::widget
