#include "AppNavigationPanel.h"

#include <FluentQt/Design.h>

#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

namespace ui::navigation {

ConversationItemWidget::ConversationItemWidget(const ConversationData& data, QWidget* parent)
    : QWidget(parent)
    , m_data(data)
{
    setFixedHeight(56);
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
}

void ConversationItemWidget::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

void ConversationItemWidget::enterEvent(FluentEnterEvent*)
{
    m_hovered = true;
    update();
}

void ConversationItemWidget::leaveEvent(QEvent*)
{
    m_hovered = false;
    m_pressed = false;
    update();
}

void ConversationItemWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        update();
    }
}

void ConversationItemWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_pressed) {
        m_pressed = false;
        update();
        if (rect().contains(event->pos())) {
            emit clicked();
        }
    }
}

void ConversationItemWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const auto& colors = themeColorsRef();
    const QRect r = rect();

    // 1. 背景绘制 (Hover/Pressed/Selected)
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

    // 2. 选中指示条
    if (m_selected) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(colors.accentDefault);
        painter.drawRoundedRect(QRect(4, r.center().y() - 10, 3, 20), 1.5, 1.5);
    }

    // 3. 圆形彩色头像
    const int avatarSize = 36;
    const QRect avatarRect(14, r.center().y() - avatarSize / 2, avatarSize, avatarSize);
    QColor avBg = m_data.avatarBg.isValid() ? m_data.avatarBg : colors.accentDefault;
    painter.setPen(Qt::NoPen);
    painter.setBrush(avBg);
    painter.drawEllipse(avatarRect);

    if (!m_data.glyph.isEmpty()) {
        painter.setPen(Qt::white);
        QFont glyphFont(Typography::FontFamily::FluentIcons);
        glyphFont.setPixelSize(18);
        painter.setFont(glyphFont);
        painter.drawText(avatarRect, Qt::AlignCenter, m_data.glyph);
    }

    // 4. 文字区域排版
    const int textLeft = avatarRect.right() + 12;
    const int textRight = r.right() - 12;
    const int textWidth = textRight - textLeft;

    // 标题与时间行
    const int topY = r.top() + 10;
    QFont titleFont = Typography::fontStyle(Typography::FontRole::BodyStrong).toQFont();
    painter.setFont(titleFont);
    painter.setPen(colors.textPrimary);

    QFont timeFont = Typography::fontStyle(Typography::FontRole::Caption).toQFont();
    QFontMetrics timeFm(timeFont);
    const int timeWidth = timeFm.horizontalAdvance(m_data.time);

    // 绘制标题
    const QRect titleRect(textLeft, topY, textWidth - timeWidth - 8, 18);
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(m_data.name, Qt::ElideRight, titleRect.width()));

    // 绘制时间
    painter.setFont(timeFont);
    painter.setPen(colors.textTertiary);
    const QRect timeRect(textRight - timeWidth, topY, timeWidth, 18);
    painter.drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter, m_data.time);

    // 副标题/最新消息摘要
    const int bottomY = topY + 20;
    QFont descFont = Typography::fontStyle(Typography::FontRole::Caption).toQFont();
    painter.setFont(descFont);
    painter.setPen(colors.textSecondary);
    const QRect descRect(textLeft, bottomY, textWidth, 16);
    painter.drawText(descRect, Qt::AlignLeft | Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(m_data.lastMessage, Qt::ElideRight, descRect.width()));
}

// ─────────────────────────────────────────────────────────────────────────────

AppNavigationPanel::AppNavigationPanel(QWidget* parent)
    : QWidget(parent)
{
    m_listLayout = new QVBoxLayout(this);
    m_listLayout->setContentsMargins(6, 6, 6, 6);
    m_listLayout->setSpacing(2);
    m_listLayout->addStretch(1);
}

void AppNavigationPanel::setViewModel(MainViewModel* viewModel)
{
    m_viewModel = viewModel;
    if (m_viewModel) {
        m_viewModel->observe(this, &AppNavigationPanel::renderState);
    }
}

void AppNavigationPanel::renderState(const MainState& state)
{
    // 如果列表数量或项有变动，重新构建
    if (m_items.size() != state.conversations.size()) {
        rebuildItems(state.conversations);
    }

    // 更新选中项
    const bool isChatActive = (state.currentPage == ui::navigation::PageId::Chat);
    for (auto* item : m_items) {
        const bool match = isChatActive && (item->data().id == state.selectedConversationId);
        item->setSelected(match);
    }
}

void AppNavigationPanel::rebuildItems(const QList<ConversationData>& conversations)
{
    // 清除旧项
    for (auto* item : m_items) {
        m_listLayout->removeWidget(item);
        item->deleteLater();
    }
    m_items.clear();

    // 重新构建，插入到末尾 stretch 之前
    int insertIndex = 0;
    for (const auto& data : conversations) {
        auto* item = new ConversationItemWidget(data, this);
        m_listLayout->insertWidget(insertIndex++, item);
        m_items.append(item);

        connect(item, &ConversationItemWidget::clicked, this, [this, data]() {
            if (m_viewModel) {
                m_viewModel->selectConversation(data.id);
            }
            emit conversationSelected(data.id);
        });
    }
}

void AppNavigationPanel::onThemeUpdated()
{
    update();
}

QSize AppNavigationPanel::sizeHint() const
{
    return QSize(260, 380);
}

}  // namespace ui::navigation
