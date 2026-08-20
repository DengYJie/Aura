#include "ThemeSettings.h"

#include <QSettings>
#include <QtGlobal>

#include <compatibility/QtCompat.h>

namespace omni::ui {
namespace {

constexpr int kThemeSystem = static_cast<int>(ThemeMode::System);
constexpr int kThemeDark = static_cast<int>(ThemeMode::Dark);
const char* kThemeModeKey = "appearance/themeMode";

QSettings uiSettings() {
    return QSettings(QStringLiteral("OmniAuth"), QStringLiteral("UiSettings"));
}

ThemeMode normalizeThemeMode(int value) {
    return static_cast<ThemeMode>(qBound(kThemeSystem, value, kThemeDark));
}

}  // namespace

ThemeMode loadThemeMode() {
    const QSettings settings = uiSettings();
    return normalizeThemeMode(
        settings.value(QString::fromLatin1(kThemeModeKey), kThemeSystem).toInt());
}

void saveThemeMode(ThemeMode mode) {
    QSettings settings = uiSettings();
    settings.setValue(QString::fromLatin1(kThemeModeKey), static_cast<int>(mode));
}

fluent::FluentElement::Theme resolveTheme(ThemeMode mode) {
    if (mode == ThemeMode::Light) {
        return fluent::FluentElement::Light;
    }
    if (mode == ThemeMode::Dark) {
        return fluent::FluentElement::Dark;
    }

    const FluentSystemColorScheme scheme = fluentSystemColorScheme();
    return scheme == FluentSystemColorScheme::Dark ? fluent::FluentElement::Dark
                                                   : fluent::FluentElement::Light;
}

void applyThemeMode(ThemeMode mode, bool persist) {
    if (persist) {
        saveThemeMode(mode);
    }
    fluent::FluentElement::setThemeDeferred(resolveTheme(mode));
}

void applyStoredThemeMode() {
    applyThemeMode(loadThemeMode(), false);
}

void installSystemThemeWatcher(QObject* context) {
    if (!context) {
        return;
    }

    fluentConnectSystemColorSchemeChanged(context, []() {
        if (loadThemeMode() == ThemeMode::System) {
            applyThemeMode(ThemeMode::System, false);
        }
    });
}

}  // namespace omni::ui
