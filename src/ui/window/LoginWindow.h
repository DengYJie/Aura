#pragma once

#include <QWidget>
#include <FluentQt/Foundation.h>
#include <FluentQt/TextFields.h>
#include <FluentQt/BasicInput.h>
#include "WindowBase.h"

class LoginWindow : public WindowBase {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);

signals:
    void loginSuccess(const QString& userId);

private slots:
    void onLoginClicked();

private:
    fluent::textfields::LineEdit* m_idEdit;
    fluent::basicinput::Button* m_loginBtn;
};
