#include "LoginWindow.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <FluentQt/Design.h>

LoginWindow::LoginWindow(QWidget* parent)
    : WindowBase(parent)
{
    setWindowTitle("Aura Chat - 登录");
    resize(400, 300);

    setBackdropEffect(fluent::windowing::BackdropEffect::Mica);
    
    QWidget* centralWidget = new QWidget(this);
    auto layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    auto title = new QLabel("Aura Chat 登录", centralWidget);
    title->setFont(Typography::fontStyle(Typography::FontRole::Title).toQFont());
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    m_idEdit = new fluent::textfields::LineEdit(centralWidget);
    m_idEdit->setPlaceholderText("请输入用户ID (如: 01, 02)");
    layout->addWidget(m_idEdit);

    m_loginBtn = new fluent::basicinput::Button(centralWidget);
    m_loginBtn->setText("登录");
    m_loginBtn->setFluentStyle(fluent::basicinput::Button::Accent);
    layout->addWidget(m_loginBtn);

    layout->addStretch();
    setContentWidget(centralWidget);

    connect(m_loginBtn, &fluent::basicinput::Button::clicked, this, &LoginWindow::onLoginClicked);

    
    // Auto login on enter
    connect(m_idEdit, &fluent::textfields::LineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
}

void LoginWindow::onLoginClicked()
{
    QString userId = m_idEdit->text().trimmed();
    if (userId.isEmpty()) {
        QMessageBox::warning(this, "错误", "用户ID不能为空！");
        return;
    }
    emit loginSuccess(userId);
}
