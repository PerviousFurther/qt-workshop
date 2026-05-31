#include <QLoggingCategory>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QJsonObject>

#include "Core/Utils.hpp"
#include "Backend/AiChat/Session.hpp"
#include "Session.hpp"
#include "PageAiChatPrivate.hpp"

// ====================================
// 顶层容器页面：PageChat 实现
// ====================================
PageChat::PageChat(QWidget* parent) : Extensible(parent) {
    this->setProperty("iconText", lstr("💬"));
    this->setProperty("title", lstr("AI 对话"));
    this->resize(1200, 600);

    mainLayout_ = new QHBoxLayout(this);
    mainLayout_->setContentsMargins(20, 20, 20, 20);
    mainLayout_->setSpacing(12);

    // 实例化独立子面板
    listPanel_ = new ConversationListPanel(this);
    chatPanel_ = new ChatAreaPanel(this);
    modelPanel_ = new ModelSettingsPanel(this);

    mainLayout_->addWidget(listPanel_);
    mainLayout_->addWidget(chatPanel_);
    mainLayout_->addWidget(modelPanel_);

    // 绑定联动点击事件（展开/折叠状态切换）
    connect(listPanel_, &ConversationListPanel::titleClicked, this, &PageChat::handleLeft);
    connect(chatPanel_, &ChatAreaPanel::titleClicked, this, &PageChat::handleCenter);
    connect(modelPanel_, &ModelSettingsPanel::titleClicked, this, &PageChat::handleRight);

    auto* uiSession = UiSession::instance();
    connect(uiSession, &UiSession::themeChanged, this, &PageChat::updateTopTheme);

    updateTopTheme();
    transitionToState(ChatDisplayState::ChatCenteredState); // 默认聚焦中间对话流
}

void PageChat::handleLeft() { transitionToState(ChatDisplayState::ChatLeftState); }
void PageChat::handleCenter() { transitionToState(ChatDisplayState::ChatCenteredState); }
void PageChat::handleRight() { transitionToState(ChatDisplayState::ChatRightState); }

void PageChat::transitionToState(int newState) {
    currentState = newState;

    int listStretch = 1;
    int chatStretch = 1;
    int modelStretch = 1;

    switch (currentState) {
    case ChatDisplayState::ChatLeftState:
        listStretch = 10;
        listPanel_->setExpanded(true);
        chatPanel_->setExpanded(false);
        modelPanel_->setExpanded(false);
        break;
    case ChatDisplayState::ChatCenteredState:
        chatStretch = 10;
        listPanel_->setExpanded(false);
        chatPanel_->setExpanded(true);
        modelPanel_->setExpanded(false);
        break;
    case ChatDisplayState::ChatRightState:
        modelStretch = 10;
        listPanel_->setExpanded(false);
        chatPanel_->setExpanded(false);
        modelPanel_->setExpanded(true);
        break;
    default:
        qCritical() << "[PageChat ERROR] Unknown state:" << newState;
        return;
    }

    mainLayout_->setStretchFactor(listPanel_, listStretch);
    mainLayout_->setStretchFactor(chatPanel_, chatStretch);
    mainLayout_->setStretchFactor(modelPanel_, modelStretch);
    mainLayout_->update();
}

void PageChat::updateTopTheme() {
    auto theme = UiSession::instance()->theme();
    QString bgColor = theme.value(lstr("background"), lstr("#F8F9FA")).toString();
    this->setStyleSheet(lstr("background-color: %1;").arg(bgColor));
}

// #include "PageAiChat.moc"

VerticalTitleBarAi::VerticalTitleBarAi(const QString& text, QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    QString visualText;
    for (int i = 0; i < text.length(); ++i) {
        visualText.append(text.at(i));
        if (i != text.length() - 1) visualText.append(lstr("\n"));
    }

    label_ = new QLabel(visualText, this);
    label_->setAlignment(Qt::AlignCenter);
    layout->addWidget(label_);
}

ConversationListPanel::ConversationListPanel(QWidget* parent) : QWidget(parent) {
    auto* masterLayout = new QHBoxLayout(this);
    masterLayout->setContentsMargins(0, 0, 0, 0);
    masterLayout->setSpacing(0);

    titleBar_ = new VerticalTitleBarAi(lstr("对话历史"), this);
    masterLayout->addWidget(titleBar_);
    connect(titleBar_, &VerticalTitleBarAi::clicked, this, &ConversationListPanel::titleClicked);

    contentWidget_ = new QWidget(this);
    auto* vLayout = new QVBoxLayout(contentWidget_);
    vLayout->setContentsMargins(15, 15, 15, 15);

    newChatBtn_ = new QPushButton(lstr("+ 新建对话"), contentWidget_);
    newChatBtn_->setFixedHeight(35);
    vLayout->addWidget(newChatBtn_);

    historyList_ = new QListWidget(contentWidget_);
    historyList_->setStyleSheet(lstr("border: none; background: transparent;"));
    vLayout->addWidget(historyList_);

    masterLayout->addWidget(contentWidget_);

    connect(newChatBtn_, &QPushButton::clicked, this, []() {
        AiChatSession::instance()->switchConversation(-1); // 传 -1 新建对话
        });

    connect(historyList_, &QListWidget::currentRowChanged, this, [](int row) {
        if (row >= 0) AiChatSession::instance()->switchConversation(row);
        });

    auto* uiSession = UiSession::instance();
    connect(uiSession, &UiSession::themeChanged, this, &ConversationListPanel::updateThemeAndStyles);
    connect(uiSession, &UiSession::fontChanged, this, &ConversationListPanel::updateThemeAndStyles);

    updateThemeAndStyles();
}

void ConversationListPanel::setExpanded(bool expanded) {
    contentWidget_->setVisible(expanded);
    titleBar_->setVisible(!expanded);
}

void ConversationListPanel::updateThemeAndStyles() {
    auto theme = UiSession::instance()->theme();
    auto font = UiSession::instance()->font();

    QString surfaceColor = theme.value(lstr("surface"), lstr("#FFFFFF")).toString();
    QString borderColor = theme.value(lstr("border"), lstr("#D9E2EC")).toString();
    QString textSecondary = theme.value(lstr("textSecondary"), lstr("#64748B")).toString();
    QString primaryColor = theme.value(lstr("primary"), lstr("#3B82F6")).toString();

    QString fontFamily = font.value(lstr("family"), lstr("Microsoft YaHei")).toString();
    int sizeNormal = font.value(lstr("sizeNormal"), 14).toInt();
    int sizeSubtitle = font.value(lstr("sizeLarge"), 16).toInt();

    this->setStyleSheet(lstr("background-color: %1; border-radius: 16px; border: 1px solid %2;").arg(surfaceColor, borderColor));
    titleBar_->setStyleSheet(lstr("background-color: %1; border: none;").arg(surfaceColor));
    titleBar_->setLabelStyle(lstr("font-family: '%1'; font-weight: bold; font-size: %2px; color: %3; line-height: 1.2;")
        .arg(fontFamily).arg(sizeSubtitle).arg(textSecondary));

    newChatBtn_->setStyleSheet(lstr("font-family: '%1'; font-size: %2px; background-color: %3; border: 1px dashed %4; color: %5; border-radius: 8px; font-weight: bold;")
        .arg(fontFamily).arg(sizeNormal).arg(surfaceColor, borderColor, primaryColor));
}

ChatAreaPanel::ChatAreaPanel(QWidget* parent) : QWidget(parent) {
    auto* masterLayout = new QHBoxLayout(this);
    masterLayout->setContentsMargins(0, 0, 0, 0);
    masterLayout->setSpacing(0);

    titleBar_ = new VerticalTitleBarAi(lstr("AI 对话"), this);
    masterLayout->addWidget(titleBar_);
    connect(titleBar_, &VerticalTitleBarAi::clicked, this, &ChatAreaPanel::titleClicked);

    contentWidget_ = new QWidget(this);
    auto* chatLayout = new QVBoxLayout(contentWidget_);
    chatLayout->setContentsMargins(15, 15, 15, 15);

    // 消息气泡展示流
    chatViewer_ = new QListWidget(contentWidget_);
    chatViewer_->setStyleSheet(lstr("border: none; background: transparent;"));
    chatViewer_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    chatLayout->addWidget(chatViewer_, 1);

    // 底端输入栏区域
    auto* inputLayout = new QHBoxLayout();
    inputLine_ = new QLineEdit(contentWidget_);
    inputLine_->setFixedHeight(40);
    sendBtn_ = new QPushButton(lstr("发送"), contentWidget_);
    sendBtn_->setFixedSize(80, 40);
    sendBtn_->setCursor(Qt::PointingHandCursor);

    inputLayout->addWidget(inputLine_);
    inputLayout->addWidget(sendBtn_);
    chatLayout->addLayout(inputLayout);

    masterLayout->addWidget(contentWidget_);

    // 绑定信号
    connect(sendBtn_, &QPushButton::clicked, this, &ChatAreaPanel::onSendTriggered);
    connect(inputLine_, &QLineEdit::returnPressed, this, &ChatAreaPanel::onSendTriggered);

    auto* session = AiChatSession::instance();
    connect(session, &AiChatSession::messageReceive, this, &ChatAreaPanel::onMessageReceived);
    connect(session, &AiChatSession::responeError, this, &ChatAreaPanel::onResponseError);

    auto* uiSession = UiSession::instance();
    connect(uiSession, &UiSession::themeChanged, this, &ChatAreaPanel::updateThemeAndStyles);
    connect(uiSession, &UiSession::fontChanged, this, &ChatAreaPanel::updateThemeAndStyles);

    updateThemeAndStyles();
}

void ChatAreaPanel::setExpanded(bool expanded) {
    contentWidget_->setVisible(expanded);
    titleBar_->setVisible(!expanded);
}

// private slots:
void ChatAreaPanel::onSendTriggered() {
    QString text = inputLine_->text().trimmed();
    if (text.isEmpty()) return;

    appendMessageItem(lstr("我"), text, true);
    inputLine_->clear();

    // 构造发送所需的上下文 JSON
    QJsonObject context;
    context.insert(lstr("prompt"), text);

    // 发送消息，此处传入自增或固定参数作为 requestCode
    AiChatSession::instance()->sendMessage(context, 1001);
}

void ChatAreaPanel::onMessageReceived(int requestCode, QString modelName, QString message) {
    appendMessageItem(modelName, message, false);
}

void ChatAreaPanel::onResponseError(int requestCode, int errorCode, QString msg) {
    QString errorMsg = lstr("[错误 %1] : %2").arg(errorCode).arg(msg);
    appendMessageItem(lstr("系统"), errorMsg, false);
    QMessageBox::warning(this, lstr("请求失败"), lstr("AI 服务响应失败: %1").arg(msg));
}

void ChatAreaPanel::appendMessageItem(const QString& sender, const QString& text, bool isSelf) {
    auto font = UiSession::instance()->font();
    auto theme = UiSession::instance()->theme();

    QString fontFamily = font.value(lstr("family"), lstr("Microsoft YaHei")).toString();
    int sizeNormal = font.value(lstr("sizeNormal"), 14).toInt();

    QString textColor = isSelf ? theme.value(lstr("primary"), lstr("#2F73F6")).toString()
        : theme.value(lstr("textPrimary"), lstr("#333333")).toString();

    auto* item = new QListWidgetItem(chatViewer_);
    auto* label = new QLabel(lstr("<b>%1:</b> %2").arg(sender, text));
    label->setWordWrap(true);
    label->setStyleSheet(lstr("font-family: '%1'; font-size: %2px; color: %3; background: transparent; border: none;")
        .arg(fontFamily).arg(sizeNormal).arg(textColor));

    item->setSizeHint(QSize(0, label->sizeHint().height() + 15));
    chatViewer_->addItem(item);
    chatViewer_->setItemWidget(item, label);
    chatViewer_->scrollToBottom();
}

void ChatAreaPanel::updateThemeAndStyles() {
    auto theme = UiSession::instance()->theme();
    auto font = UiSession::instance()->font();

    QString bgColor = theme.value(lstr("background"), lstr("#F8F9FA")).toString();
    QString surfaceColor = theme.value(lstr("surface"), lstr("#FFFFFF")).toString();
    QString borderColor = theme.value(lstr("border"), lstr("#D9E2EC")).toString();
    QString primaryColor = theme.value(lstr("primary"), lstr("#3B82F6")).toString();
    QString textSecondary = theme.value(lstr("textSecondary"), lstr("#64748B")).toString();

    QString fontFamily = font.value(lstr("family"), lstr("Microsoft YaHei")).toString();
    int sizeNormal = font.value(lstr("sizeNormal"), 14).toInt();
    int sizeSubtitle = font.value(lstr("sizeLarge"), 16).toInt();

    this->setStyleSheet(lstr("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border-radius: 16px; border: 1px solid %3;")
        .arg(bgColor, surfaceColor, borderColor));

    titleBar_->setStyleSheet(lstr("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border: none;").arg(bgColor, surfaceColor));
    titleBar_->setLabelStyle(lstr("font-family: '%1'; font-weight: bold; font-size: %2px; color: %3; line-height: 1.2;")
        .arg(fontFamily).arg(sizeSubtitle).arg(textSecondary));

    inputLine_->setStyleSheet(lstr("font-family: '%1'; font-size: %2px; background: %3; border: 1px solid %4; border-radius: 8px; padding-left: 10px;")
        .arg(fontFamily).arg(sizeNormal).arg(surfaceColor, borderColor));

    sendBtn_->setStyleSheet(lstr("font-family: '%1'; background-color: %2; color: white; border-radius: 8px; font-size: %3px; font-weight: bold;")
        .arg(fontFamily).arg(primaryColor).arg(sizeNormal));
}

inline ModelSettingsPanel::ModelSettingsPanel(QWidget* parent) : QWidget(parent) {
    auto* masterLayout = new QHBoxLayout(this);
    masterLayout->setContentsMargins(0, 0, 0, 0);
    masterLayout->setSpacing(0);

    titleBar_ = new VerticalTitleBarAi(lstr("模型配置"), this);
    masterLayout->addWidget(titleBar_);
    connect(titleBar_, &VerticalTitleBarAi::clicked, this, &ModelSettingsPanel::titleClicked);

    contentWidget_ = new QWidget(this);
    auto* vLayout = new QVBoxLayout(contentWidget_);
    vLayout->setContentsMargins(15, 15, 15, 15);

    modelList_ = new QListWidget(contentWidget_);
    modelList_->setStyleSheet(lstr("border: none; background: transparent;"));
    vLayout->addWidget(modelList_);

    masterLayout->addWidget(contentWidget_);

    // 双击或单选模型切换
    connect(modelList_, &QListWidget::currentTextChanged, this, [](const QString& modelName) {
        if (!modelName.isEmpty()) AiChatSession::instance()->switchModel(modelName);
        });

    auto* session = AiChatSession::instance();
    connect(session, &AiChatSession::availbleModelChanged, this, &ModelSettingsPanel::refreshModels);

    auto* uiSession = UiSession::instance();
    connect(uiSession, &UiSession::themeChanged, this, &ModelSettingsPanel::updateThemeAndStyles);
    connect(uiSession, &UiSession::fontChanged, this, &ModelSettingsPanel::updateThemeAndStyles);

    updateThemeAndStyles();
    refreshModels();
}

inline void ModelSettingsPanel::setExpanded(bool expanded) {
    contentWidget_->setVisible(expanded);
    titleBar_->setVisible(!expanded);
}

inline void ModelSettingsPanel::refreshModels() {
    modelList_->clear();
    // 严格遵循接口声明的拼写：availbleModel()
    QStringList models = AiChatSession::instance()->availbleModel();
    modelList_->addItems(models);
}

inline void ModelSettingsPanel::updateThemeAndStyles() {
    auto theme = UiSession::instance()->theme();
    auto font = UiSession::instance()->font();

    QString surfaceColor = theme.value(lstr("surface"), lstr("#FFFFFF")).toString();
    QString borderColor = theme.value(lstr("border"), lstr("#D9E2EC")).toString();
    QString textSecondary = theme.value(lstr("textSecondary"), lstr("#64748B")).toString();

    QString fontFamily = font.value(lstr("family"), lstr("Microsoft YaHei")).toString();
    int sizeSubtitle = font.value(lstr("sizeLarge"), 16).toInt();

    this->setStyleSheet(lstr("background-color: %1; border-radius: 16px; border: 1px solid %2;").arg(surfaceColor, borderColor));
    titleBar_->setStyleSheet(lstr("background-color: %1; border: none;").arg(surfaceColor));
    titleBar_->setLabelStyle(lstr("font-family: '%1'; font-weight: bold; font-size: %2px; color: %3; line-height: 1.2;")
        .arg(fontFamily).arg(sizeSubtitle).arg(textSecondary));
}
