#pragma once

#include <QHash>

#include <FluentQt/Navigation.h>

#include "../../navigation/PageId.h"
#include "../../window/WindowBase.h"
#include "MainViewModel.h"

namespace ui::screen::chat {
class ChatPage;
}

namespace ui::navigation {
class AppNavigationPanel;
class AppNavFooter;
}

namespace ui::widget {
class PlaceholderView;
}

namespace ui::screen::main {

class MainWindow : public WindowBase {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    MainViewModel* viewModel() const { return m_viewModel; }

private:
    void renderState(const MainState& state);
    int pageIndex(ui::navigation::PageId pageId) const;

    MainViewModel* m_viewModel = nullptr;
    fluent::navigation::NavigationView* m_navigationView = nullptr;
    ui::navigation::AppNavigationPanel* m_navPanel = nullptr;
    ui::navigation::AppNavFooter* m_navFooter = nullptr;
    ui::widget::PlaceholderView* m_placeholderView = nullptr;
    ui::screen::chat::ChatPage* m_chatPage = nullptr;
    QHash<ui::navigation::PageId, int> m_pageIndices;
};

}  // namespace ui::screen::main
