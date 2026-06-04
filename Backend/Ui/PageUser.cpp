#include <QTimer>
#include <QListWidget>
#include <QGraphicsEffect>
#include <QLabel>
#include <QLineEdit>
#include <QDebug>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QResizeEvent>

#include "Backend/Network/Session.hpp"
#include "Session.hpp"
#include "PageUser.hpp"

static constexpr bool G_EnableMockNetwork = true;

PageUser::PageUser(QWidget* parent)
    : Extensible(parent)
{
    this->setAttribute(Qt::WA_StyledBackground, true);

    this->setMinimumHeight(500);
    this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    this->setContentsMargins(12, 12, 12, 12);

    this->setProperty("iconText", lstr("🏠"));
    this->setProperty("title", lstr("首页"));

    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    headerArea_ = new QWidget(this);
    contentArea_ = new QWidget(this);
    rootLayout->addWidget(headerArea_);
    rootLayout->addWidget(contentArea_);

    rootLayout->setStretch(0, 0);
    rootLayout->setStretch(1, 1);

    // ==========================================
    // HeaderArea
    // ==========================================
    headerArea_->setMinimumWidth(36);
    headerArea_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    auto* leftLayout = new QVBoxLayout(headerArea_);
    leftLayout->setContentsMargins(24, 32, 24, 32);
    leftLayout->setSpacing(16);

    avatarLabel_ = new QLabel(headerArea_);
    avatarLabel_->setFixedSize(88, 88);
    avatarLabel_->setAlignment(Qt::AlignCenter);
    avatarLabel_->setText(lstr("User"));
    leftLayout->addWidget(avatarLabel_, 0, Qt::AlignHCenter);

    nameLabel_ = new QLabel(lstr(" could-be-you "), headerArea_);
    leftLayout->addWidget(nameLabel_, 0, Qt::AlignHCenter);

    descLabel_ = new QLabel(lstr("高级尊享会员"), headerArea_);
    leftLayout->addWidget(descLabel_, 0, Qt::AlignHCenter);

    profileList_ = new QListWidget(headerArea_);
    profileList_->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    profileList_->addItem(lstr("我的功能中心"));
    profileList_->addItem(lstr("数据云同步"));
    profileList_->addItem(lstr("安全与隐私"));
    leftLayout->addWidget(profileList_);

    logoutBtn_ = new QPushButton(lstr("退出登录"), headerArea_);
    logoutBtn_->setFixedHeight(40);
    logoutBtn_->setCursor(Qt::PointingHandCursor);
    leftLayout->addWidget(logoutBtn_);

    loginPanelWidget_ = new QWidget(headerArea_);
    loginPanelWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* loginPanelLayout = new QVBoxLayout(loginPanelWidget_);
    loginPanelLayout->setContentsMargins(0, 0, 0, 0);
    loginPanelLayout->setSpacing(0);
    loginPanelLayout->addStretch(1);

    loginContainer_ = new QWidget(loginPanelWidget_);
    loginContainer_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto* loginLayout = new QVBoxLayout(loginContainer_);
    loginLayout->setContentsMargins(0, 0, 0, 0);
    loginLayout->setSpacing(14);

    loginHintLabel_ = new QLabel(lstr("欢迎使用系统\n请先登录您的账号"), loginContainer_);
    loginHintLabel_->setAlignment(Qt::AlignCenter);
    loginLayout->addWidget(loginHintLabel_);
    loginLayout->addSpacing(10);

    usernameEdit_ = new QLineEdit(loginContainer_);
    usernameEdit_->setPlaceholderText(lstr("用户名 / 邮箱"));
    loginLayout->addWidget(usernameEdit_);

    passwordEdit_ = new QLineEdit(loginContainer_);
    passwordEdit_->setPlaceholderText(lstr("密码"));
    passwordEdit_->setEchoMode(QLineEdit::Password);
    loginLayout->addWidget(passwordEdit_);

    loginBtn_ = new QPushButton(lstr("登录"), loginContainer_);
    loginBtn_->setMinimumHeight(44);
    loginBtn_->setCursor(Qt::PointingHandCursor);
    loginLayout->addWidget(loginBtn_);

    // 【补齐内容】实例化“前往注册”按钮并加入登录面板布局
    gotoRegisterBtn_ = new QPushButton(lstr("没有账号？立即注册"), loginContainer_);
    gotoRegisterBtn_->setCursor(Qt::PointingHandCursor);
    gotoRegisterBtn_->setFlat(true);
    loginLayout->addWidget(gotoRegisterBtn_);

    loginPanelLayout->addWidget(loginContainer_, 0, Qt::AlignHCenter);
    loginPanelLayout->addStretch(1);
    leftLayout->addWidget(loginPanelWidget_);

    // ==========================================
    // ContentArea
    // ==========================================
    contentArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* contentAreaLayout = new QVBoxLayout(contentArea_);
    contentAreaLayout->setContentsMargins(20, 20, 20, 20);
    contentAreaLayout->setSpacing(16);
    contentAreaLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    promoPanelWidget_ = new QWidget(contentArea_);
    promoPanelWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* promoPanelLayout = new QVBoxLayout(promoPanelWidget_);
    promoPanelLayout->setContentsMargins(0, 0, 0, 0);
    promoPanelLayout->setSpacing(0);
    promoPanelLayout->addStretch(1);

    promoWidget_ = new QWidget(promoPanelWidget_);
    promoWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto* promoLayout = new QVBoxLayout(promoWidget_);
    promoLayout->setAlignment(Qt::AlignCenter);
    promoLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* promoArtLabel = new QLabel(lstr("✨"), promoWidget_);
    promoArtLabel->setStyleSheet("font-size: 72px;");
    promoArtLabel->setAlignment(Qt::AlignCenter);

    promoTitleLabel_ = new QLabel(lstr("开启您的智能时代"), promoWidget_);
    promoTitleLabel_->setAlignment(Qt::AlignCenter);

    promoDescLabel_ = new QLabel(lstr("登录后查看更多健康的功能\n帮助您实时获取分析快报并享受全方位保障。"), promoWidget_);
    promoDescLabel_->setAlignment(Qt::AlignCenter);

    promoLayout->addWidget(promoArtLabel);
    promoLayout->addWidget(promoTitleLabel_);
    promoLayout->addWidget(promoDescLabel_);
    promoPanelLayout->addWidget(promoWidget_, 0, Qt::AlignHCenter);
    promoPanelLayout->addStretch(1);
    contentAreaLayout->addWidget(promoPanelWidget_);

    emptyPanelWidget_ = new QWidget(contentArea_);
    emptyPanelWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto* emptyPanelLayout = new QVBoxLayout(emptyPanelWidget_);
    emptyPanelLayout->setContentsMargins(0, 0, 0, 0);
    emptyPanelLayout->setSpacing(0);
    emptyPanelLayout->addStretch(1);

    emptyContentWidget_ = new QWidget(emptyPanelWidget_);
    emptyContentWidget_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto* emptyLayout = new QVBoxLayout(emptyContentWidget_);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setContentsMargins(0, 0, 0, 0);

    QLabel* emptyIconLabel = new QLabel(lstr("🔌"), emptyContentWidget_);
    emptyIconLabel->setStyleSheet("font-size: 56px;");
    emptyIconLabel->setAlignment(Qt::AlignCenter);

    emptyTextLabel_ = new QLabel(lstr("设备尚未连接"), emptyContentWidget_);
    emptyTextLabel_->setAlignment(Qt::AlignCenter);

    emptySubTextLabel_ = new QLabel(lstr("连接设备以浏览健康信息"), emptyContentWidget_);
    emptySubTextLabel_->setAlignment(Qt::AlignCenter);

    mockConnectBtn_ = new QPushButton(lstr("连接设备"), emptyContentWidget_);
    mockConnectBtn_->setMinimumSize(180, 36);
    connect(mockConnectBtn_, &QPushButton::clicked, this, &PageUser::requestConnection);

    emptyLayout->addWidget(emptyIconLabel);
    emptyLayout->addSpacing(8);
    emptyLayout->addWidget(emptyTextLabel_);
    emptyLayout->addWidget(emptySubTextLabel_);
    emptyLayout->addSpacing(15);
    emptyLayout->addWidget(mockConnectBtn_, 0, Qt::AlignHCenter);

    emptyPanelLayout->addWidget(emptyContentWidget_, 0, Qt::AlignHCenter);
    emptyPanelLayout->addStretch(1);
    contentAreaLayout->addWidget(emptyPanelWidget_);

    contentBoxWidget_ = new QWidget(contentArea_);
    contentBoxWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    contentHLayout_ = new QHBoxLayout(contentBoxWidget_);
    contentHLayout_->setSpacing(16);
    contentHLayout_->setContentsMargins(10, 10, 10, 10);
    contentHLayout_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    contentAreaLayout->addWidget(contentBoxWidget_);

    registerPanelWidget_ = new QWidget(headerArea_);
    registerPanelWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* registerPanelLayout = new QVBoxLayout(registerPanelWidget_);
    registerPanelLayout->setContentsMargins(0, 0, 0, 0);
    registerPanelLayout->setSpacing(0);
    registerPanelLayout->addStretch(1);

    registerContainer_ = new QWidget(registerPanelWidget_);
    registerContainer_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    auto* registerLayout = new QVBoxLayout(registerContainer_);
    registerLayout->setContentsMargins(0, 0, 0, 0);
    registerLayout->setSpacing(14);

    registerHintLabel_ = new QLabel(lstr("创建新账号"), registerContainer_);
    registerHintLabel_->setAlignment(Qt::AlignCenter);
    registerLayout->addWidget(registerHintLabel_);
    registerLayout->addSpacing(8);

    regUsernameEdit_ = new QLineEdit(registerContainer_);
    regUsernameEdit_->setPlaceholderText(lstr("用户名"));
    registerLayout->addWidget(regUsernameEdit_);

    regEmailEdit_ = new QLineEdit(registerContainer_);
    regEmailEdit_->setPlaceholderText(lstr("邮箱"));
    registerLayout->addWidget(regEmailEdit_);

    regPasswordEdit_ = new QLineEdit(registerContainer_);
    regPasswordEdit_->setPlaceholderText(lstr("密码"));
    regPasswordEdit_->setEchoMode(QLineEdit::Password);
    registerLayout->addWidget(regPasswordEdit_);

    regConfirmEdit_ = new QLineEdit(registerContainer_);
    regConfirmEdit_->setPlaceholderText(lstr("确认密码"));
    regConfirmEdit_->setEchoMode(QLineEdit::Password);
    registerLayout->addWidget(regConfirmEdit_);

    registerBtn_ = new QPushButton(lstr("注册"), registerContainer_);
    registerBtn_->setMinimumHeight(44);
    registerBtn_->setCursor(Qt::PointingHandCursor);
    registerLayout->addWidget(registerBtn_);

    gotoLoginBtn_ = new QPushButton(lstr("已有账号？返回登录"), registerContainer_);
    gotoLoginBtn_->setCursor(Qt::PointingHandCursor);
    gotoLoginBtn_->setFlat(true);
    registerLayout->addWidget(gotoLoginBtn_);

    registerPanelLayout->addWidget(registerContainer_, 0, Qt::AlignHCenter);
    registerPanelLayout->addStretch(1);

    leftLayout->addWidget(registerPanelWidget_);

    // ==========================================
    // Signal binding
    // ==========================================
    connect(loginBtn_, &QPushButton::clicked, this, &PageUser::handleLogin);
    connect(logoutBtn_, &QPushButton::clicked, this, &PageUser::handleLogout);
    connect(registerBtn_, &QPushButton::clicked, this, &PageUser::handleRegister);
    connect(gotoLoginBtn_, &QPushButton::clicked, this, &PageUser::showLoginPanel);
    // 【补齐内容】绑定“前往注册”按钮的点击事件
    connect(gotoRegisterBtn_, &QPushButton::clicked, this, &PageUser::showRegisterPanel);

    if (G_EnableMockNetwork) {
        this->updateLoginState(false);
    }
    else {
        if (auto* session = NetworkSession::instance()) {
            connect(session, &NetworkSession::userIdChanged, this, &PageUser::onLoginStatusChanged);
            connect(session, &NetworkSession::errorOccured, this, &PageUser::onLoginError);
            this->updateLoginState(session->logined());
        }
        else {
            this->updateLoginState(false);
        }
    }

    auto ins = UiSession::instance();
    connect(ins, &UiSession::themeChanged, this, &PageUser::updateStyle);
    connect(ins, &UiSession::fontChanged, this, &PageUser::updateStyle);

    this->showLoginPanel();
    this->updateStyle();
    this->adjustPageWidth();
}

qsizetype PageUser::appendWidget(QWidget* widget) {
    qsizetype idx = Extensible::appendWidget(widget);
    widget->setParent(this->contentBoxWidget_);
    widget->setVisible(isLoggedIn_);
    refreshContent();
    return idx;
}

QWidget* PageUser::widget(qsizetype index, QWidget* widget) {
    QWidget* old = Extensible::widget(index, widget);
    if (old) old->setParent(nullptr);
    refreshContent();
    return old;
}

QWidget* PageUser::removeWidget(qsizetype index) noexcept {
    QWidget* old = Extensible::removeWidget(index);
    if (old) {
        old->setParent(nullptr);
        contentHLayout_->removeWidget(old);
    }
    refreshContent();
    return old;
}

void PageUser::updateLoginState(bool loggedIn) {
    this->isLoggedIn_ = loggedIn;

    avatarLabel_->setVisible(isLoggedIn_);
    nameLabel_->setVisible(isLoggedIn_);
    descLabel_->setVisible(isLoggedIn_);
    profileList_->setVisible(isLoggedIn_);
    logoutBtn_->setVisible(isLoggedIn_);

    loginPanelWidget_->setVisible(!isLoggedIn_ && !isRegisterMode_);
    registerPanelWidget_->setVisible(!isLoggedIn_ && isRegisterMode_);
    promoPanelWidget_->setVisible(!isLoggedIn_);

    if (isLoggedIn_) {
        refreshContent();
    }
    else {
        usernameEdit_->clear();
        passwordEdit_->clear();

        if (regUsernameEdit_) regUsernameEdit_->clear();
        if (regEmailEdit_) regEmailEdit_->clear();
        if (regPasswordEdit_) regPasswordEdit_->clear();
        if (regConfirmEdit_) regConfirmEdit_->clear();

        emptyPanelWidget_->setVisible(false);
        contentBoxWidget_->setVisible(false);

        this->adjustPageWidth();
    }
}

void PageUser::showLoginPanel() {
    isRegisterMode_ = false;
    if (!isLoggedIn_) {
        loginPanelWidget_->setVisible(true);
        registerPanelWidget_->setVisible(false);
    }
    loginHintLabel_->setText(lstr("欢迎使用系统\n请先登录您的账号"));
    this->adjustPageWidth();
}

void PageUser::showRegisterPanel() {
    isRegisterMode_ = true;
    if (!isLoggedIn_) {
        loginPanelWidget_->setVisible(false);
        registerPanelWidget_->setVisible(true);
    }
    registerHintLabel_->setText(lstr("创建新账号"));
    this->adjustPageWidth();
}

void PageUser::handleRegister() {
    QString username = regUsernameEdit_->text().trimmed();
    QString email = regEmailEdit_->text().trimmed();
    QString password = regPasswordEdit_->text();
    QString confirm = regConfirmEdit_->text();

    if (username.isEmpty() || email.isEmpty() || password.isEmpty() || confirm.isEmpty()) {
        registerHintLabel_->setStyleSheet(lstr("color: #FF4D4F; font-size: 14px; font-weight: bold; background: transparent;"));
        registerHintLabel_->setText(lstr("用户名、邮箱和密码都不能为空"));
        return;
    }

    if (password != confirm) {
        registerHintLabel_->setStyleSheet(lstr("color: #FF4D4F; font-size: 14px; font-weight: bold; background: transparent;"));
        registerHintLabel_->setText(lstr("两次输入的密码不一致"));
        return;
    }

    registerBtn_->setEnabled(false);
    registerHintLabel_->setStyleSheet(lstr("color: %1; font-size: 14px; font-weight: bold; background: transparent;")
        .arg(UiSession::instance()->theme().value(lstr("textPrimary")).toString()));
    registerHintLabel_->setText(lstr("正在注册，请稍候..."));

    if (G_EnableMockNetwork) {
        QTimer::singleShot(500, this, [this, username]() {
            registerBtn_->setEnabled(true);
            nameLabel_->setText(username + lstr(" (新用户)"));
            this->updateLoginState(true);
        });
    } else {
        auto* session = NetworkSession::instance();
        if (session) {
            session->create(username, password);
        } else {
            registerBtn_->setEnabled(true);
            registerHintLabel_->setStyleSheet(lstr("color: #FF4D4F; font-size: 14px; font-weight: bold; background: transparent;"));
            registerHintLabel_->setText(lstr("网络模块未加载，无法注册"));
        }
    }
}

void PageUser::refreshContent() {
    if (!isLoggedIn_) {
        this->adjustPageWidth();
        return;
    }

    while (auto item = contentHLayout_->takeAt(0)) {
        delete item;
    }

    auto currentWidgets = widgets();
    if (currentWidgets.isEmpty()) {
        emptyPanelWidget_->setVisible(true);
        contentBoxWidget_->setVisible(false);
    }
    else {
        emptyPanelWidget_->setVisible(false);
        contentBoxWidget_->setVisible(true);

        for (int i = 0; i < currentWidgets.size(); ++i) {
            QWidget* w = currentWidgets.at(i);
            if (!w) continue;

            w->setParent(contentBoxWidget_);
            w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            w->show();
            contentHLayout_->addWidget(w);
        }

        contentHLayout_->addStretch(1);
    }

    contentBoxWidget_->updateGeometry();
    contentArea_->updateGeometry();
    this->adjustPageWidth();
}

void PageUser::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    this->adjustPageWidth();
}

void PageUser::handleLogin() {
    QString username = usernameEdit_->text().trimmed();
    QString password = passwordEdit_->text();

    if (username.isEmpty() || password.isEmpty()) {
        loginHintLabel_->setText(lstr("用户名和密码都不能为空\n请重新输入！"));
        return;
    }

    loginBtn_->setEnabled(false);
    loginHintLabel_->setText(lstr("正在登录，请稍候..."));

    if (G_EnableMockNetwork) {
        QTimer::singleShot(500, this, [this, username]() {
            loginBtn_->setEnabled(true);
            nameLabel_->setText(username + lstr(" (UI测试)"));
            this->updateLoginState(true);
            });
    }
    else {
        auto* session = NetworkSession::instance();
        if (session) {
            session->login(username, password);
        }
        else {
            loginBtn_->setEnabled(true);
            loginHintLabel_->setText(lstr("网络模块未加载，重启应用再试。"));
        }
    }
}

void PageUser::onLoginStatusChanged() {
    if (G_EnableMockNetwork) return;

    auto* session = NetworkSession::instance();
    bool loggedIn = session->logined();
    loginBtn_->setEnabled(true);

    if (loggedIn) {
        QString realName = session->userName().isEmpty() ? lstr("未命名用户") : session->userName();
        nameLabel_->setText(realName);
    }
    else {
        loginHintLabel_->setText(lstr("欢迎使用系统\n请先登录您的账号"));
        this->updateStyle();
    }

    this->updateLoginState(loggedIn);
}

void PageUser::onLoginError(int error, QString message) {
    if (G_EnableMockNetwork) return;

    loginBtn_->setEnabled(true);
    loginHintLabel_->setStyleSheet(lstr("color: #FF4D4F; font-size: 14px; font-weight: bold;"));
    loginHintLabel_->setText(lstr("登录失败 (%1):\n%2").arg(QString::number(error), message));
}

void PageUser::handleLogout() {
    if (G_EnableMockNetwork) {
        this->updateLoginState(false);
        loginHintLabel_->setText(lstr("欢迎使用系统\n请先登录您的账号"));
    }
    else {
        auto* session = NetworkSession::instance();
        session->logout();
        this->updateLoginState(false);
    }
}

// 【补齐内容】实现设备连接槽函数以支持 Mock 模拟和实际行为
//void PageUser::requestConnection() {
//    if (G_EnableMockNetwork) {
//        // 创建一个模拟的健康数据监控卡片
//        auto* mockCard = new QWidget();
//        auto* cardLayout = new QVBoxLayout(mockCard);
//        cardLayout->setContentsMargins(16, 16, 16, 16);
//
//        auto* titleLabel = new QLabel(lstr("❤️ 实时健康守护中心"), mockCard);
//        titleLabel->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: bold; background: transparent;");
//
//        auto* infoLabel = new QLabel(lstr("设备状态：在线\n• 当前心率: 72 bpm\n• 今日步数: 8432 步\n• 睡眠评分: 88分"), mockCard);
//        infoLabel->setStyleSheet("color: rgba(255, 255, 255, 0.8); font-size: 14px; background: transparent;");
//
//        cardLayout->addWidget(titleLabel);
//        cardLayout->addSpacing(6);
//        cardLayout->addWidget(infoLabel);
//
//        mockCard->setStyleSheet("QWidget { background-color: rgba(255, 255, 255, 0.12); border: 1px solid rgba(255, 255, 255, 0.2); border-radius: 12px; }");
//
//        // 动态向可扩展页面容器添加该组件
//        this->appendWidget(mockCard);
//    } else {
//        qDebug() << "正在尝试连接实际底层硬件设备...";
//    }
//}

void PageUser::adjustPageWidth() {
    if (!headerArea_ || !contentArea_) return;

    if (this->layout()) {
        this->layout()->activate();
    }
    if (contentArea_->layout()) {
        contentArea_->layout()->activate();
    }
    if (contentBoxWidget_ && contentBoxWidget_->layout()) {
        contentBoxWidget_->layout()->activate();
    }

    const int leftWidth = headerArea_->sizeHint().width();
    const int rightWidth = contentArea_->sizeHint().width();
    const int totalWidth = leftWidth + rightWidth;

    this->setMinimumWidth(totalWidth);
    this->updateGeometry();
}

void PageUser::updateStyle() {
    auto* ui = UiSession::instance();
    QVariantMap theme = ui->theme();
    QVariantMap font = ui->font();

    QString bgColor = theme.value(lstr("background")).toString();
    QString surfaceColor = theme.value(lstr("surface")).toString();
    QString borderColor = theme.value(lstr("border")).toString();
    QString primaryColor = theme.value(lstr("primary")).toString();
    QString txtPrimary = theme.value(lstr("textPrimary")).toString();
    QString txtSecondary = theme.value(lstr("textSecondary")).toString();
    QString fontFamily = font.value(lstr("family")).toString();

    int titleSize = font.value(lstr("titleSize")).toInt();
    int titleWeight = font.value(lstr("titleWeight")).toInt();
    int subtitleSize = font.value(lstr("subtitleSize")).toInt();
    int subtitleWeight = font.value(lstr("subtitleWeight")).toInt();
    int bodySize = font.value(lstr("bodySize")).toInt();
    int bodyWeight = font.value(lstr("bodyWeight")).toInt();
    int captionSize = font.value(lstr("captionSize")).toInt();
    int captionWeight = font.value(lstr("captionWeight")).toInt();

    // =================================================================
    // 1. 最外层窗口 (PageUser) - 统一配置【全屏线性渐变】与【大圆角外边框】
    // =================================================================
    this->setStyleSheet(lstr(
        "PageUser {"
        "   background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2);"
        "   border: 1px solid %3;"
        "   border-radius: 24px;"
        "   font-family: '%4';"
        "   font-size: %5px;"
        "   font-weight: %6;"
        "}"
    ).arg(bgColor, surfaceColor, borderColor, fontFamily, QString::number(bodySize), QString::number(bodyWeight)));

    // =================================================================
    // 2. 内部主面板与容器 - 彻底去除边框与背景，全透明透出底层渐变
    // =================================================================
    QString transparentStyle = "QWidget { background: transparent; border: none; }";

    headerArea_->setStyleSheet(transparentStyle);
    contentArea_->setStyleSheet(transparentStyle);
    promoPanelWidget_->setStyleSheet(transparentStyle);
    promoWidget_->setStyleSheet(transparentStyle);
    emptyPanelWidget_->setStyleSheet(transparentStyle);
    emptyContentWidget_->setStyleSheet(transparentStyle);
    contentBoxWidget_->setStyleSheet(transparentStyle);
    loginPanelWidget_->setStyleSheet(transparentStyle);
    loginContainer_->setStyleSheet(transparentStyle);

    // =================================================================
    // 3. 内部独立子控件样式 - 恢复常规扁平设计（无额外渐变与独立背景框）
    // =================================================================

    // 头像：保留头像本身的圆形边界，内部填充主题色
    avatarLabel_->setStyleSheet(lstr(
        "QLabel {"
        "   background-color: %1;"
        "   border-radius: 44px;"
        "   color: white;"
        "   font-size: %2px;"
        "   font-weight: %3;"
        "}"
    ).arg(primaryColor, QString::number(titleSize), QString::number(titleWeight)));

    nameLabel_->setStyleSheet(lstr("font-size: %1px; font-weight: %2; color: %3; background: transparent;")
        .arg(QString::number(subtitleSize), QString::number(titleWeight), txtPrimary));

    descLabel_->setStyleSheet(lstr("color: %1; font-size: %2px; font-weight: %3; background: transparent;")
        .arg(txtSecondary, QString::number(captionSize), QString::number(captionWeight)));

    // 列表：完全透明，仅靠下划线分隔，无独立底色
    profileList_->setStyleSheet(lstr(
        "QListWidget { border: none; background: transparent; outline: none; }"
        "QListWidget::item { height: 40px; padding-left: 4px; border-bottom: 1px solid %1; color: %2; font-size: %3px; font-weight: %4; }"
        "QListWidget::item:hover { background-color: rgba(255, 255, 255, 0.08); }"
    ).arg(borderColor, txtPrimary, QString::number(bodySize), QString::number(bodyWeight)));

    loginHintLabel_->setStyleSheet(lstr("color: %1; font-size: %2px; font-weight: %3; background: transparent;")
        .arg(txtPrimary, QString::number(subtitleSize), QString::number(titleWeight)));

    // 输入框：采用微弱的白色/暗色半透明作为输入区背景，防止完全透明导致文字与渐变底色粘连
    QString editStyle = lstr(
        "QLineEdit {"
        "   border: 1px solid %1;"
        "   border-radius: 10px;"
        "   padding: 10px;"
        "   background-color: rgba(255, 255, 255, 0.15);"
        "   color: %2;"
        "   font-size: %3px;"
        "   font-weight: %4;"
        "}"
        "QLineEdit:focus {"
        "   border: 1px solid %5;"
        "   background-color: rgba(255, 255, 255, 0.25);"
        "}"
    ).arg(borderColor, txtPrimary, QString::number(bodySize), QString::number(bodyWeight), primaryColor);
    usernameEdit_->setStyleSheet(editStyle);
    passwordEdit_->setStyleSheet(editStyle);

    // 登录按钮：纯色扁平
    loginBtn_->setStyleSheet(lstr(
        "QPushButton { background-color: %1; color: white; border: none; border-radius: 10px; font-size: %2px; font-weight: %3; }"
        "QPushButton:hover { background-color: %1; opacity: 0.9; }"
        "QPushButton:pressed { background-color: %1; opacity: 0.8; }"
    ).arg(primaryColor, QString::number(bodySize), QString::number(titleWeight)));

    // 退出按钮：透明背景，红字红边框
    logoutBtn_->setStyleSheet(lstr(
        "QPushButton {"
        "    background-color: transparent;"
        "    border: 1px solid #FF4D4F;"
        "    color: #FF4D4F;"
        "    border-radius: 10px;"
        "    font-size: %1px;"
        "    font-weight: %2;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 77, 79, 0.1);"
        "}"
    ).arg(QString::number(bodySize), QString::number(titleWeight)));

    promoTitleLabel_->setStyleSheet(lstr("font-size: %1px; font-weight: %2; color: %3; background: transparent; margin-top: 10px;")
        .arg(QString::number(titleSize), QString::number(titleWeight), txtPrimary));

    promoDescLabel_->setStyleSheet(lstr("font-size: %1px; font-weight: %2; color: %3; background: transparent; line-height: 20px; margin-top: 8px;")
        .arg(QString::number(bodySize), QString::number(bodyWeight), txtSecondary));

    emptyTextLabel_->setStyleSheet(lstr("font-size: %1px; font-weight: %2; color: %3; background: transparent;")
        .arg(QString::number(subtitleSize), QString::number(titleWeight), txtPrimary));

    emptySubTextLabel_->setStyleSheet(lstr("font-size: %1px; font-weight: %2; color: %3; background: transparent;")
        .arg(QString::number(bodySize), QString::number(bodyWeight), txtSecondary));

    // 连接设备按钮：线框扁平设计
    mockConnectBtn_->setStyleSheet(lstr(
        "QPushButton { background: transparent; border: 1px solid %1; color: %1; border-radius: 8px; font-size: %2px; font-weight: %3; }"
        "QPushButton:hover { background: rgba(255, 255, 255, 0.1); }"
    ).arg(primaryColor, QString::number(captionSize), QString::number(captionWeight)));

    registerHintLabel_->setStyleSheet(lstr("color: %1; font-size: %2px; font-weight: %3; background: transparent;")
        .arg(txtPrimary, QString::number(subtitleSize), QString::number(titleWeight)));

    regUsernameEdit_->setStyleSheet(editStyle);
    regEmailEdit_->setStyleSheet(editStyle);
    regPasswordEdit_->setStyleSheet(editStyle);
    regConfirmEdit_->setStyleSheet(editStyle);

    registerBtn_->setStyleSheet(lstr(
        "QPushButton { background-color: %1; color: white; border: none; border-radius: 10px; font-size: %2px; font-weight: %3; }"
        "QPushButton:hover { background-color: %1; opacity: 0.9; }"
        "QPushButton:pressed { background-color: %1; opacity: 0.8; }"
    ).arg(primaryColor, QString::number(bodySize), QString::number(titleWeight)));

    gotoRegisterBtn_->setStyleSheet(lstr(
        "QPushButton { background: transparent; border: none; color: %1; font-size: %2px; }"
        "QPushButton:hover { text-decoration: underline; }"
    ).arg(txtSecondary, QString::number(captionSize)));

    gotoLoginBtn_->setStyleSheet(lstr(
        "QPushButton { background: transparent; border: none; color: %1; font-size: %2px; }"
        "QPushButton:hover { text-decoration: underline; }"
    ).arg(txtSecondary, QString::number(captionSize)));
}