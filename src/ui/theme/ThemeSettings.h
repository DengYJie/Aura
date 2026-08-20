#pragma once

#include <QObject>

#include <components/foundation/FluentElement.h>

namespace omni::ui {

enum class ThemeMode {
    System = 0,
    Light = 1,
    Dark = 2,
};

ThemeMode loadThemeMode();
void saveThemeMode(ThemeMode mode);
fluent::FluentElement::Theme resolveTheme(ThemeMode mode);
void applyThemeMode(ThemeMode mode, bool persist);
void applyStoredThemeMode();
void installSystemThemeWatcher(QObject* context);

}  // namespace omni::ui
