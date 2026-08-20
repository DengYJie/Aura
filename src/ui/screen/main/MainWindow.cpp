#include "MainWindow.h"

#include "../../navigation/AppNavFooter.h"
#include "../../navigation/AppNavigationPanel.h"
#include "../../widget/PlaceholderView.h"
#include "../../window/TitleBar.h"
#include "../chat/ChatPage.h"
#include "../../../data/remote/TcpChatClient.h"

#include <FluentQt/Design.h>

namespace ui::screen::main {

    MainWindow::MainWindow(QWidget* parent)
        : WindowBase(parent)
        , m_viewModel(new MainViewModel(this))
    {
        setWindowTitle(QStringLiteral("Aura AI"));
        resize(1420, 920);
        setMinimumSize(1180, 760);
        setBackdropEffect(fluent::windowing::BackdropEffect::Mica);

        titleBar()->setTitle(QStringLiteral("Aura AI"));
        titleBar()->setSubtitle(QStringLiteral("智能聊天助手"));
        titleBar()->setPaneToggleButtonVisible(false);
        titleBar()->setBackButtonVisible(false);

        m_navigationView = new fluent::navigation::NavigationView(this);
        m_navigationView->setDisplayMode(fluent::navigation::NavigationView::DisplayMode::Left);
        m_navigationView->setPaneOpen(true);
        m_navigationView->setExpandedPaneWidth(260);
        m_navigationView->setAnimationEnabled(false);

        m_navPanel = new ui::navigation::AppNavigationPanel(m_navigationView);
        m_navFooter = new ui::navigation::AppNavFooter(m_navigationView);

        // 绑定 ViewModel
        m_navPanel->setViewModel(m_viewModel);
        m_navFooter->setViewModel(m_viewModel);

        m_navigationView->setMainChromeWidget(m_navPanel);
        m_navigationView->setFooterChromeWidget(m_navFooter);

        auto* host = m_navigationView->contentHost();
        host->setTransitionAnimationEnabled(true);
        host->setTransitionEffect(fluent::navigation::StackContentHost::TransitionEffect::SlideFromLeft);
        host->setContentSurface(themeColorsRef().bgLayer,
            themeRadius().overlay,
            themeColorsRef().strokeSurface);

        m_placeholderView = new ui::widget::PlaceholderView(host);
        m_chatPage = new ui::screen::chat::ChatPage(host);

        m_pageIndices.insert(ui::navigation::PageId::Placeholder, 0);
        m_pageIndices.insert(ui::navigation::PageId::Chat, 1);

        host->insertPage(0, m_placeholderView);
        host->insertPage(1, m_chatPage);

        setContentWidget(m_navigationView);

        // 订阅 MainViewModel 单向数据流 (UDF)
        m_viewModel->observe(this, &MainWindow::renderState);
        
        // 监听 TCP 状态
        connect(&TcpChatClient::instance(), &TcpChatClient::connected, this, [this]() {
            titleBar()->setSubtitle(QStringLiteral("智能聊天助手 - 已连接"));
        });
        connect(&TcpChatClient::instance(), &TcpChatClient::disconnected, this, [this]() {
            titleBar()->setSubtitle(QStringLiteral("智能聊天助手 - 连接已断开"));
        });
        connect(&TcpChatClient::instance(), &TcpChatClient::reconnecting, this, [this](int delaySec) {
            titleBar()->setSubtitle(
                QStringLiteral("智能聊天助手 - 正在重连中 (%1s)...").arg(delaySec));
        });
        connect(&TcpChatClient::instance(), &TcpChatClient::errorOccurred, this, [this](const QString& err, const QString&) {
            // 只有非重连期间的独立错误才更新标题（重连信号会覆盖）
            if (!TcpChatClient::instance().isConnected()) {
                titleBar()->setSubtitle(QStringLiteral("智能聊天助手 - 连接异常"));
            }
        });

        if (TcpChatClient::instance().isConnected()) {
            titleBar()->setSubtitle(QStringLiteral("智能聊天助手 - 已连接"));
        } else {
            titleBar()->setSubtitle(QStringLiteral("智能聊天助手 - 未连接"));
        }
    }

    void MainWindow::renderState(const MainState& state)
    {
        if (!m_navigationView) {
            return;
        }

        auto* host = m_navigationView->contentHost();
        const int index = pageIndex(state.currentPage);
        if (index >= 0) {
            const int direction = host->currentIndex() < 0 || index >= host->currentIndex() ? 1 : -1;
            host->setCurrentIndex(index, direction, state.currentPage != ui::navigation::PageId::Placeholder);
        }

        // 单向数据流驱动：根据 State 中当前选中的会话模型同步给 ChatPage
        if (m_chatPage && state.currentPage == ui::navigation::PageId::Chat) {
            if (const auto* conv = state.selectedConversation()) {
                m_chatPage->setConversation(conv->id, conv->name);
            }
        }
    }

    int MainWindow::pageIndex(ui::navigation::PageId pageId) const
    {
        return m_pageIndices.value(pageId, -1);
    }

}  // namespace ui::screen::main
