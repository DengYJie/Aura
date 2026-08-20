#include <FluentQt/FluentQt.h>

#include "data/di/AppContainer.h"
#include "ui/screen/main/MainWindow.h"
#include "ui/theme/ThemeSettings.h"

#include <QtWidgets/QApplication>

#include "ui/window/LoginWindow.h"

int main(int argc, char* argv[])
{
    fluent::prepareHighDpiApplication();
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Aura"));
    app.setOrganizationName(QStringLiteral("AuraHomework"));

    fluent::initializeResources();
    app.setFont(Typography::fontStyle(Typography::FontRole::Body).toQFont());
    fluent::StyleThemeCatalog::apply(fluent::StyleTheme::Fluent);
    omni::ui::applyStoredThemeMode();
    omni::ui::installSystemThemeWatcher(&app);

    ui::screen::main::MainWindow* window = nullptr;
    LoginWindow loginWin;
    QObject::connect(&loginWin, &LoginWindow::loginSuccess, [&](const QString& userId) {
        AppContainer::init(userId);
        window = new ui::screen::main::MainWindow();
        window->show();
        loginWin.close();
    });
    loginWin.show();

    const int result = QApplication::exec();
    
    if (window) {
        delete window;
    }
    AppContainer::shutdown();
    return result;
}
