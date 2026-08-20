#include "ChatPage.h"

#include <FluentQt/BasicInput.h>
#include <FluentQt/Collections.h>
#include <FluentQt/TextFields.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QListView>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>

#include "MessageBubbleDelegate.h"
#include "MessageListModel.h"
#include "../../../utils/ImageUtils.h"

namespace ui::screen::chat {

    class SurfacePanel : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
        public:
            explicit SurfacePanel(QWidget* parent = nullptr)
                : QWidget(parent)
            {}

            void setPanelColors(const QColor& fill, const QColor& stroke)
            {
                m_fill = fill;
                m_stroke = stroke;
                update();
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
                const QColor fill = m_fill.isValid() ? m_fill : themeColorsRef().bgCanvas;
                const QColor stroke = m_stroke.isValid() ? m_stroke : themeColorsRef().strokeCard;
                painter.setPen(stroke);
                painter.setBrush(fill);
                painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), themeRadius().overlay, themeRadius().overlay);
            }

        private:
            QColor m_fill;
            QColor m_stroke;
        };

        class ChatHeaderWidget : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
        public:
            explicit ChatHeaderWidget(QWidget* parent = nullptr)
                : QWidget(parent)
            {
                setFixedHeight(46);
                auto* layout = new QHBoxLayout(this);
                layout->setContentsMargins(20, 0, 16, 0);
                layout->setSpacing(10);

                m_titleLabel = new fluent::textfields::Label(QString(), this);
                m_titleLabel->setFluentTypography(Typography::FontRole::BodyStrong);
                layout->addWidget(m_titleLabel, 1, Qt::AlignVCenter);

                m_translateBtn = new fluent::basicinput::Button(QStringLiteral("翻译"), this);
                m_translateBtn->setFluentStyle(fluent::basicinput::Button::Subtle);
                m_translateBtn->setFluentSize(fluent::basicinput::Button::Small);
                m_translateBtn->setFluentLayout(fluent::basicinput::Button::IconBefore);
                m_translateBtn->setIconGlyph(Typography::Icons::World, Typography::IconSize::Compact);
                layout->addWidget(m_translateBtn, 0, Qt::AlignVCenter);
            }

            void setTitle(const QString& title)
            {
                if (m_titleLabel) {
                    m_titleLabel->setText(title);
                }
            }

            fluent::basicinput::Button* translateButton() const
            {
                return m_translateBtn;
            }

            void onThemeUpdated() override
            {
                update();
            }

        protected:
            void paintEvent(QPaintEvent*) override
            {
                QPainter painter(this);
                // 绘制底部发丝分割线
                painter.setPen(themeColorsRef().strokeDefault);
                painter.drawLine(0, height() - 1, width(), height() - 1);
            }

        private:
            fluent::textfields::Label* m_titleLabel = nullptr;
            fluent::basicinput::Button* m_translateBtn = nullptr;
        };

        class TranslateSidePane : public QWidget, public fluent::FluentElement, public fluent::QMLPlus {
        public:
            explicit TranslateSidePane(QWidget* parent = nullptr)
                : QWidget(parent)
            {
                auto* rootLayout = new QVBoxLayout(this);
                rootLayout->setContentsMargins(0, 0, 0, 0);
                rootLayout->setSpacing(0);

                // 顶部标题栏（高度 46px，与聊天主窗口 Header 严格水平对齐）
                auto* header = new QWidget(this);
                header->setFixedHeight(46);
                auto* headerLayout = new QHBoxLayout(header);
                headerLayout->setContentsMargins(16, 0, 12, 0);
                headerLayout->setSpacing(8);

                auto* titleLabel = new fluent::textfields::Label(QStringLiteral("实时翻译"), header);
                titleLabel->setFluentTypography(Typography::FontRole::BodyStrong);
                headerLayout->addWidget(titleLabel, 1, Qt::AlignVCenter);

                m_closeBtn = new fluent::basicinput::Button(QStringLiteral(""), header);
                m_closeBtn->setFluentStyle(fluent::basicinput::Button::Subtle);
                m_closeBtn->setFluentSize(fluent::basicinput::Button::Small);
                m_closeBtn->setIconGlyph(Typography::Icons::ChromeClose, Typography::IconSize::Compact);
                m_closeBtn->setFixedSize(30, 30);
                headerLayout->addWidget(m_closeBtn, 0, Qt::AlignVCenter);

                rootLayout->addWidget(header);

                // 内容区（顶部对齐，去除异常间隙）
                auto* contentWidget = new QWidget(this);
                auto* contentLayout = new QVBoxLayout(contentWidget);
                contentLayout->setContentsMargins(16, 14, 16, 16);
                contentLayout->setSpacing(10);
                contentLayout->setAlignment(Qt::AlignTop);

                auto* langRow = new QHBoxLayout;
                langRow->setSpacing(8);
                auto* langLabel = new fluent::textfields::Label(QStringLiteral("目标语言:"), contentWidget);
                langLabel->setFluentTypography(Typography::FontRole::Caption);
                langLabel->setTextColorRole(fluent::textfields::Label::TextColorRole::Secondary);
                langRow->addWidget(langLabel);

                m_langCombo = new fluent::basicinput::ComboBox(contentWidget);
                m_langCombo->addItem(QStringLiteral("英语 (English)"));
                m_langCombo->addItem(QStringLiteral("中文 (Chinese)"));
                m_langCombo->addItem(QStringLiteral("日语 (Japanese)"));
                m_langCombo->addItem(QStringLiteral("法语 (French)"));
                m_langCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                langRow->addWidget(m_langCombo, 1);
                contentLayout->addLayout(langRow);

                auto* resLabel = new fluent::textfields::Label(QStringLiteral("译文预览:"), contentWidget);
                resLabel->setFluentTypography(Typography::FontRole::Caption);
                resLabel->setTextColorRole(fluent::textfields::Label::TextColorRole::Secondary);
                contentLayout->addWidget(resLabel);

                m_editor = new fluent::textfields::TextEdit(contentWidget);
                m_editor->setReadOnly(true);
                m_editor->setMinVisibleLines(8);
                m_editor->setMaxVisibleLines(20);
                m_editor->setPlainText(QStringLiteral("欢迎使用实时翻译功能。发送或选中的消息将在此即时翻译为目标语言。"));
                contentLayout->addWidget(m_editor);

                contentLayout->addStretch(1);
                rootLayout->addWidget(contentWidget, 1);
            }

            fluent::textfields::TextEdit* editor() const { return m_editor; }
            fluent::basicinput::ComboBox* langComboBox() const { return m_langCombo; }
            fluent::basicinput::Button* closeButton() const { return m_closeBtn; }

            void setTranslatedText(const QString& text)
            {
                if (m_editor && m_editor->toPlainText() != text) {
                    m_editor->setPlainText(text);
                }
            }

            void setTargetLanguage(const QString& lang)
            {
                if (m_langCombo && m_langCombo->currentText() != lang) {
                    const int idx = m_langCombo->findText(lang);
                    if (idx >= 0) {
                        m_langCombo->setCurrentIndex(idx);
                    }
                }
            }

            void onThemeUpdated() override
            {
                update();
            }

        protected:
            void paintEvent(QPaintEvent*) override
            {
                QPainter painter(this);
                const auto& colors = themeColorsRef();
                // 背景填充
                painter.fillRect(rect(), colors.bgLayer);
                // 左边框分割线
                painter.setPen(colors.strokeDefault);
                painter.drawLine(0, 0, 0, height());
                // Header 底部发丝分割线 (y = 45)
                painter.drawLine(0, 45, width(), 45);
            }

        private:
            fluent::basicinput::Button* m_closeBtn = nullptr;
            fluent::basicinput::ComboBox* m_langCombo = nullptr;
            fluent::textfields::TextEdit* m_editor = nullptr;
        };

        ChatPage::ChatPage(QWidget* parent)
        : QWidget(parent)
        , m_viewModel(new ChatViewModel(this))
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);

        auto* splitView = new fluent::collections::SplitView(this);

        auto* conversationSurface = new QWidget(splitView);
        auto* conversationLayout = new QVBoxLayout(conversationSurface);
        conversationLayout->setContentsMargins(0, 0, 0, 0);
        conversationLayout->setSpacing(0);

        m_headerPanel = new ChatHeaderWidget(conversationSurface);
        conversationLayout->addWidget(m_headerPanel);

        auto* messageCard = new QWidget(conversationSurface);
        auto* messageCardLayout = new QVBoxLayout(messageCard);
        messageCardLayout->setContentsMargins(16, 8, 16, 0);
        messageCardLayout->setSpacing(0);

        m_messageView = new QListView(messageCard);
        m_messageView->setFrameShape(QFrame::NoFrame);
        m_messageView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_messageView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        m_messageView->setSelectionMode(QAbstractItemView::NoSelection);
        m_messageView->setFocusPolicy(Qt::NoFocus);
        m_messageView->setUniformItemSizes(false);
        m_messageView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_messageView->setSpacing(0);
        m_messageView->setStyleSheet(QStringLiteral("QListView { background: transparent; border: none; outline: none; } QListView::item { background: transparent; border: none; outline: none; } QListView::item:selected { background: transparent; } QListView::item:hover { background: transparent; }"));
        m_messageView->setMouseTracking(true); // For ToolTip
        m_messageView->viewport()->setAttribute(Qt::WA_Hover, true); // Extra guard for hover events

        m_model = new MessageListModel(this);
        m_delegate = new MessageBubbleDelegate(this);
        m_messageView->setModel(m_model);
        m_messageView->setItemDelegate(m_delegate);
        m_messageView->viewport()->installEventFilter(this);
        connect(m_delegate, &MessageBubbleDelegate::retryClicked, m_viewModel, &ChatViewModel::retryMessage);
        messageCardLayout->addWidget(m_messageView, 1);

        conversationLayout->addWidget(messageCard, 1);

        auto* composerPanel = new QWidget(conversationSurface);
        auto* composerLayout = new QVBoxLayout(composerPanel);
        composerLayout->setContentsMargins(16, 6, 16, 16);
        composerLayout->setSpacing(8);

        auto* quickReplyRow = new QHBoxLayout;
        quickReplyRow->setContentsMargins(0, 0, 0, 0);
        quickReplyRow->setSpacing(8);
        for (int i = 0; i < 3; ++i) {
            auto* button = new fluent::basicinput::Button(QString(), composerPanel);
            button->setFluentStyle(fluent::basicinput::Button::Subtle);
            button->setFluentSize(fluent::basicinput::Button::Small);
            button->hide();
            quickReplyRow->addWidget(button);
            m_quickReplyButtons.append(button);
        }
        quickReplyRow->addStretch(1);

        composerLayout->addLayout(quickReplyRow);

        m_editor = new fluent::textfields::TextEdit(composerPanel);
        m_editor->setMinVisibleLines(3);
        m_editor->setMaxVisibleLines(7);

        // 左下角图片上传按钮
        m_imageButton = new fluent::basicinput::Button(QStringLiteral(""), m_editor);
        m_imageButton->setFluentStyle(fluent::basicinput::Button::Subtle);
        m_imageButton->setIconGlyph(Typography::Icons::Camera, Typography::IconSize::Standard);
        m_imageButton->setFixedSize(32, 32);

        // 右下角“发送”按钮
        m_sendButton = new fluent::basicinput::Button(QStringLiteral(""), m_editor);
        m_sendButton->setFluentStyle(fluent::basicinput::Button::Accent);
        m_sendButton->setIconGlyph(Typography::Icons::Send, Typography::IconSize::Standard);
        m_sendButton->setFixedSize(32, 32);

        class EditorButtonsFilter : public QObject {
        public:
            EditorButtonsFilter(QWidget* leftBtn, QWidget* rightBtn, ChatViewModel* vm, fluent::textfields::TextEdit* editor, QObject* parent = nullptr)
                : QObject(parent), m_leftBtn(leftBtn), m_rightBtn(rightBtn), m_vm(vm), m_editor(editor) {}
        protected:
            bool eventFilter(QObject* obj, QEvent* event) override {
                if (event->type() == QEvent::Resize) {
                    auto* w = static_cast<QWidget*>(obj);
                    m_leftBtn->move(8, w->height() - m_leftBtn->height() - 8);
                    m_rightBtn->move(w->width() - m_rightBtn->width() - 8, w->height() - m_rightBtn->height() - 8);
                } else if (event->type() == QEvent::KeyPress) {
                    auto* keyEvent = static_cast<QKeyEvent*>(event);
                    if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
                        && !(keyEvent->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))) {
                        const QString text = m_editor->toPlainText();
                        if (!text.trimmed().isEmpty()) {
                            if (m_vm && m_vm->sendMessage(text)) {
                                m_editor->clear();
                            }
                        }
                        return true;
                    }
                }
                return false;
            }
        private:
            QWidget* m_leftBtn;
            QWidget* m_rightBtn;
            ChatViewModel* m_vm;
            fluent::textfields::TextEdit* m_editor;
        };
        m_editor->installEventFilter(new EditorButtonsFilter(m_imageButton, m_sendButton, m_viewModel, m_editor, m_editor));

        // 绑定点击图片按钮打开文件选取 -> 派发 Intent
        connect(m_imageButton, &QPushButton::clicked, this, [this]() {
            const QString filePath = QFileDialog::getOpenFileName(
                this,
                QStringLiteral("选择图片"),
                QString(),
                QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.webp *.bmp *.gif)"));
            if (!filePath.isEmpty()) {
                QString dataUri = ImageUtils::imageToDataUri(filePath);
                if (!dataUri.isEmpty()) {
                    m_viewModel->sendMessage(dataUri);
                }
            }
        });

        composerLayout->addWidget(m_editor);

        // 发送按钮点击 -> 派发 Intent
        connect(m_sendButton, &QPushButton::clicked, this, [this]() {
            const QString text = m_editor->toPlainText();
            if (!text.trimmed().isEmpty()) {
                if (m_viewModel->sendMessage(text)) {
                    m_editor->clear();
                }
            }
        });

        conversationLayout->addWidget(composerPanel);

        // Setup translate pane
        m_translatePane = new TranslateSidePane(splitView);

        // 绑定翻译按钮与关闭按钮
        connect(m_headerPanel->translateButton(), &QPushButton::clicked, this, [this]() {
            m_viewModel->toggleTranslatePane();
        });

        connect(m_translatePane->closeButton(), &QPushButton::clicked, this, [this]() {
            m_viewModel->setTranslateVisible(false);
        });

        connect(m_translatePane->langComboBox(), &fluent::basicinput::ComboBox::currentTextChanged, this, [this](const QString& lang) {
            m_viewModel->setTranslateTargetLanguage(lang);
        });

        fluent::collections::SplitViewPaneOptions contentOptions;
        contentOptions.fill = true;
        splitView->addPane(conversationSurface, contentOptions);

        fluent::collections::SplitViewPaneOptions paneOptions;
        paneOptions.fill = false;
        paneOptions.preferredSize = 340;
        splitView->addPane(m_translatePane, paneOptions);

        root->addWidget(splitView, 1);

        // 订阅 ViewModel 单向数据流 (UDF)
        m_viewModel->observe(this, &ChatPage::renderState);
    }

    void ChatPage::setConversation(const QString& id, const QString& title)
    {
        if (m_viewModel) {
            m_viewModel->switchConversation(id, title);
        }
    }

    void ChatPage::renderState(const ChatState& state)
    {
        // 1. 渲染 Header 标题
        if (m_headerPanel) {
            m_headerPanel->setTitle(state.conversationTitle);
        }

        // 2. 渲染消息列表
        if (m_model) {
            m_model->setMessages(state.messages);
            scrollToBottom();
        }

        // 3. 禁用/启用输入与发送控件（AI 生成中或处于 Sending 状态时不可发送下一条消息）
        bool isBusy = state.isStreaming;
        for (const auto& msg : state.messages) {
            if (msg.deliveryStatus == MessageDeliveryStatus::Sending) {
                isBusy = true;
                break;
            }
        }

        if (m_sendButton) {
            m_sendButton->setEnabled(!isBusy);
        }
        if (m_imageButton) {
            m_imageButton->setEnabled(!isBusy);
        }
        if (m_editor) {
            m_editor->setPlaceholderText(isBusy
                ? QStringLiteral("正在发送/生成中，请稍候...")
                : QStringLiteral("输入消息..."));
        }

        for (int i = 0; i < m_quickReplyButtons.size(); ++i) {
            auto* button = m_quickReplyButtons[i];
            if (!button) {
                continue;
            }
            button->setEnabled(!isBusy);
            if (i < state.quickReplies.size()) {
                const QString reply = state.quickReplies.at(i);
                button->setText(reply);
                button->show();
                button->disconnect();
                connect(button, &QPushButton::clicked, this, [this, reply]() {
                    m_viewModel->sendMessage(reply);
                });
            }
            else {
                button->hide();
            }
        }

        // 4. 渲染翻译侧边栏
        if (m_translatePane) {
            m_translatePane->setVisible(state.isTranslateOpen);
            m_translatePane->setTargetLanguage(state.translateTargetLang);
            m_translatePane->setTranslatedText(state.translatedContent);
        }
    }

    void ChatPage::scrollToBottom()
    {
        if (m_messageView && m_model && m_model->rowCount() > 0) {
            QTimer::singleShot(10, this, [this]() {
                m_messageView->scrollTo(m_model->index(m_model->rowCount() - 1, 0), QAbstractItemView::PositionAtBottom);
            });
        }
    }

    bool ChatPage::eventFilter(QObject* watched, QEvent* event)
    {
        if (m_messageView && watched == m_messageView->viewport()) {
            if (event->type() == QEvent::MouseMove) {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                const QPoint pos = mouseEvent->pos();
                const QModelIndex idx = m_messageView->indexAt(pos);
                if (idx.isValid() && m_delegate && m_delegate->hitTestRetry(idx, pos, m_messageView->visualRect(idx))) {
                    m_messageView->viewport()->setCursor(Qt::PointingHandCursor);
                } else {
                    m_messageView->viewport()->unsetCursor();
                }
            } else if (event->type() == QEvent::MouseButtonRelease) {
                auto* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    const QPoint pos = mouseEvent->pos();
                    const QModelIndex idx = m_messageView->indexAt(pos);
                    if (idx.isValid() && m_delegate && m_delegate->hitTestRetry(idx, pos, m_messageView->visualRect(idx))) {
                        const QString msgId = idx.data(MessageListModel::IdRole).toString();
                        if (!msgId.isEmpty()) {
                            m_viewModel->retryMessage(msgId);
                            return true;
                        }
                    }
                }
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void ChatPage::onThemeUpdated()
    {
        update();
    }

}  // namespace ui::screen::chat
