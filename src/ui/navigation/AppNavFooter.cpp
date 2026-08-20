#include "AppNavFooter.h"

#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace ui::navigation {

NavFooterButton::NavFooterButton(const QString& text, const QString& glyph, QWidget* parent)
    : QWidget(parent)
    , m_text(text)
    , m_glyph(glyph)
{
    setFixedHeight(40);
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
}

void NavFooterButton::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

void NavFooterButton::enterEvent(FluentEnterEvent*)
{
    m_hovered = true;
    update();
}

void NavFooterButton::leaveEvent(QEvent*)
{
    m_hovered = false;
    m_pressed = false;
    update();
}

void NavFooterButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
    }
}

void NavFooterButton::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        update();
        if (rect().contains(event->pos())) {
            emit clicked();
        }
    }
}

void NavFooterButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const auto& colors = themeColorsRef();
    const QRect r = rect();

    // 1. Background
    QColor bg = Qt::transparent;
    if (m_selected) {
        bg = colors.subtleSecondary;
    } else if (m_pressed) {
        bg = colors.subtleTertiary;
    } else if (m_hovered) {
        bg = colors.subtleSecondary;
        bg.setAlpha(120);
    }

    if (bg != Qt::transparent) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(bg);
        painter.drawRoundedRect(r.adjusted(4, 2, -4, -2), 6, 6);
    }

    // 2. Left selection indicator
    if (m_selected) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(colors.accentDefault);
        painter.drawRoundedRect(QRect(4, r.center().y() - 8, 3, 16), 1.5, 1.5);
    }

    // 3. Icon (Left-aligned at x = 14 to match the avatar center alignment above)
    const QRect iconRect(14, r.center().y() - 10, 20, 20);
    QFont iconFont(Typography::FontFamily::FluentIcons);
    iconFont.setPixelSize(18);
    painter.setFont(iconFont);
    painter.setPen(m_selected ? colors.accentDefault : colors.textPrimary);
    painter.drawText(iconRect, Qt::AlignCenter, m_glyph);

    // 4. Text (Left-aligned next to icon)
    painter.setFont(Typography::fontStyle(Typography::FontRole::Body).toQFont());
    painter.setPen(m_selected ? colors.accentDefault : colors.textPrimary);
    const QRect textRect(44, r.top(), r.width() - 50, r.height());
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_text);
}

// ── AppNavFooter ────────────────────────────────────────────────────

AppNavFooter::AppNavFooter(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 8);
    layout->setSpacing(0);

    m_settingsButton = new NavFooterButton(QStringLiteral("设置"), Typography::Icons::Settings, this);
    layout->addWidget(m_settingsButton);

    connect(m_settingsButton, &NavFooterButton::clicked, this, [this]() {
        if (m_viewModel) {
            m_viewModel->selectPage(PageId::Settings);
        }
        emit pageRequested(PageId::Settings);
    });
}

void AppNavFooter::setViewModel(MainViewModel* viewModel)
{
    m_viewModel = viewModel;
    if (m_viewModel) {
        m_viewModel->observe(this, &AppNavFooter::renderState);
    }
}

void AppNavFooter::renderState(const MainState& state)
{
    m_settingsButton->setSelected(state.currentPage == PageId::Settings);
}

void AppNavFooter::onThemeUpdated()
{
    update();
}

void AppNavFooter::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    // Draw subtle top hairline separator
    painter.setPen(themeColorsRef().strokeDefault);
    painter.drawLine(12, 0, width() - 12, 0);
}

QSize AppNavFooter::sizeHint() const
{
    return QSize(260, 54);
}

}  // namespace ui::navigation
