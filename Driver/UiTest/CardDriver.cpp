// CardDriver.cpp
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
// #include <QGraphicsDropShadowEffect>
#include <QFontMetrics>
#include <QSpacerItem>

#include "CardDriver.hpp"
#include "Driver.hpp"
#include "Backend/Ui/Session.hpp"

// ------------------------------------------------------------
// 小工具：给卡片加一点很轻的阴影，让层次更干净
// ------------------------------------------------------------
static void applyCardShadow(QFrame* frame)
{
    // auto* shadow = new QGraphicsDropShadowEffect(frame);
    // shadow->setBlurRadius(18);
    // shadow->setOffset(0, 2);
    // shadow->setColor(QColor(0, 0, 0, 28));
    // frame->setGraphicsEffect(shadow);
}

// ============================================================
// 1. DeviceSearchCard 搜索设备卡片
// ============================================================
DeviceSearchCard::DeviceSearchCard(int id, const QString& name, const QString& mac, QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 6, 10, 6);

    auto* cardFrame = new QFrame(this);
    cardFrame->setObjectName("CardFrame");
    cardFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    applyCardShadow(cardFrame);

    auto* cardLayout = new QHBoxLayout(cardFrame);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(14);

    auto* infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(4);

    auto* nameLabel = new QLabel(name, cardFrame);
    nameLabel->setObjectName("DeviceName");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setWordWrap(false);
    nameLabel->setWordWrap(Qt::ElideRight);

    auto* macLabel = new QLabel(mac, cardFrame);
    macLabel->setObjectName("DeviceMac");
    macLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    macLabel->setWordWrap(false);
    macLabel->setWordWrap(Qt::ElideRight);

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(macLabel);

    auto* connectBtn = new QPushButton("连接", cardFrame);
    connectBtn->setObjectName("ConnectButton");
    connectBtn->setFixedSize(82, 34);

    cardLayout->addLayout(infoLayout, 1);
    cardLayout->addWidget(connectBtn, 0, Qt::AlignVCenter);

    mainLayout->addWidget(cardFrame);

    connect(connectBtn, &QPushButton::clicked, this, [=]() {
        connectBtn->setEnabled(false);
        macLabel->setText("正在连接中…");

        auto* ui = UITestModule::instance();
        if (!ui) {
            connectBtn->setEnabled(true);
            macLabel->setText(mac);
            return;
        }

        connect(ui, &UITestModule::deviceConnectionError, connectBtn,
            [=](int deviceId, int /*code*/, const QString& /*message*/) {
                if (deviceId == id) {
                    connectBtn->setEnabled(true);
                    macLabel->setText(mac);
                }
            },
            Qt::SingleShotConnection);

        ui->connectDevice(id);
        });

    if (auto* session = UiSession::instance()) {
        connect(session, &UiSession::themeChanged, this, &DeviceSearchCard::updateStyle);
        connect(session, &UiSession::fontChanged, this, &DeviceSearchCard::updateStyle);
    }
    updateStyle();
}

void DeviceSearchCard::updateStyle()
{
    auto* session = UiSession::instance();
    if (!session) return;

    const auto theme = session->theme();
    const auto fontMap = session->font();

    const QString fontFamily = fontMap.value("family", "Microsoft YaHei").toString();
    const int sizeNormal = fontMap.value("sizeNormal", 14).toInt();
    const int sizeSmall = fontMap.value("sizeSmall", 12).toInt();

    const QString background = theme.value("background").toString();
    const QString border = theme.value("border").toString();
    const QString surface = theme.value("surface").toString();
    const QString textPrimary = theme.value("textPrimary").toString();
    const QString textSecondary = theme.value("textSecondary").toString();
    const QString primary = theme.value("primary").toString();

    const QString qss = QString(R"(
        DeviceSearchCard {
            font-family: '%1';
        }
        DeviceSearchCard QLabel {
            background: transparent;
        }
        #CardFrame {
            background-color: %2;
            border: 1px solid %3;
            border-radius: 12px;
        }
        #DeviceName {
            color: %4;
            font-size: %5px;
            font-weight: 600;
        }
        #DeviceMac {
            color: %6;
            font-size: %7px;
        }
        #ConnectButton {
            background-color: %8;
            color: #FFFFFF;
            border: none;
            border-radius: 8px;
            font-size: %7px;
            font-weight: 600;
            padding: 0 14px;
        }
        #ConnectButton:hover {
            background-color: %8;
            opacity: 0.92;
        }
        #ConnectButton:pressed {
            opacity: 0.82;
        }
        #ConnectButton:disabled {
            background-color: %3;
            color: %6;
        }
    )")
        .arg(fontFamily, surface, border, textPrimary)
        .arg(sizeNormal)
        .arg(textSecondary)
        .arg(sizeSmall)
        .arg(primary);

    this->setStyleSheet(qss);
}

// ============================================================
// 2. DeviceConnectedCard 已连接设备卡片
// ============================================================
DeviceConnectedCard::DeviceConnectedCard(int id, const QString& name, const QString& ip, QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 6, 10, 6);

    auto* cardFrame = new QFrame(this);
    cardFrame->setObjectName("ConnectedCardFrame");
    cardFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    applyCardShadow(cardFrame);

    auto* cardLayout = new QHBoxLayout(cardFrame);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(14);

    auto* infoLayout = new QVBoxLayout();
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(6);

    auto* nameLabel = new QLabel(name, cardFrame);
    nameLabel->setObjectName("DeviceName");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setWordWrap(false);
    nameLabel->setWordWrap(Qt::ElideRight);

    auto* statusLayout = new QHBoxLayout();
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(6);

    auto* statusDot = new QLabel(cardFrame);
    statusDot->setObjectName("StatusDot");
    statusDot->setFixedSize(8, 8);

    auto* statusLabel = new QLabel("已连接 · " + ip, cardFrame);
    statusLabel->setObjectName("DeviceStatusText");
    statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    statusLabel->setWordWrap(false);
    statusLabel->setWordWrap(Qt::ElideRight);

    statusLayout->addWidget(statusDot, 0, Qt::AlignVCenter);
    statusLayout->addWidget(statusLabel, 1);

    infoLayout->addWidget(nameLabel);
    infoLayout->addLayout(statusLayout);

    auto* detailBtn = new QPushButton("详情", cardFrame);
    detailBtn->setObjectName("DetailButton");
    detailBtn->setFixedSize(82, 34);

    auto* disconnectBtn = new QPushButton("断开", cardFrame);
    disconnectBtn->setObjectName("DisconnectButton");
    disconnectBtn->setFixedSize(82, 34);

    cardLayout->addLayout(infoLayout, 1);
    cardLayout->addWidget(detailBtn, 0, Qt::AlignVCenter);
    cardLayout->addWidget(disconnectBtn, 0, Qt::AlignVCenter);

    mainLayout->addWidget(cardFrame);

    connect(detailBtn, &QPushButton::clicked, this, [=]() {
        emit requestDetailPage(id);
        });

    connect(disconnectBtn, &QPushButton::clicked, this, [=]() {
        disconnectBtn->setEnabled(false);
        disconnectBtn->setText("断开中…");

        auto* ui = UITestModule::instance();
        if (!ui) {
            disconnectBtn->setEnabled(true);
            disconnectBtn->setText("断开");
            return;
        }

        connect(ui, &UITestModule::deviceConnectionError, disconnectBtn,
            [=](int deviceId, int /*code*/, const QString& /*message*/) {
                if (deviceId == id) {
                    disconnectBtn->setEnabled(true);
                    disconnectBtn->setText("断开");
                }
            },
            Qt::SingleShotConnection);

        ui->disconnectDevice(id);
        });

    if (auto* session = UiSession::instance()) {
        connect(session, &UiSession::themeChanged, this, &DeviceConnectedCard::updateStyle);
        connect(session, &UiSession::fontChanged, this, &DeviceConnectedCard::updateStyle);
    }
    updateStyle();
}

void DeviceConnectedCard::updateStyle()
{
    auto* session = UiSession::instance();
    if (!session) return;

    const auto theme = session->theme();
    const auto fontMap = session->font();

    const QString fontFamily = fontMap.value("family", "Microsoft YaHei").toString();
    const int sizeNormal = fontMap.value("sizeNormal", 14).toInt();
    const int sizeSmall = fontMap.value("sizeSmall", 12).toInt();

    const QString border = theme.value("border").toString();
    const QString surface = theme.value("surface").toString();
    const QString textPrimary = theme.value("textPrimary").toString();
    const QString textSecondary = theme.value("textSecondary").toString();
    const QString success = theme.value("success").toString();
    const QString danger = theme.value("danger").toString();

    const QString qss = QString(R"(
        DeviceConnectedCard {
            font-family: '%1';
        }
        DeviceConnectedCard QLabel {
            background: transparent;
        }
        #ConnectedCardFrame {
            background-color: %2;
            border: 1px solid %3;
            border-radius: 12px;
        }
        #DeviceName {
            color: %4;
            font-size: %5px;
            font-weight: 600;
        }
        #StatusDot {
            background-color: %6;
            border-radius: 4px;
        }
        #DeviceStatusText {
            color: %7;
            font-size: %8px;
        }
        #DetailButton, #DisconnectButton {
            border: none;
            border-radius: 8px;
            font-size: %8px;
            font-weight: 600;
            padding: 0 14px;
            min-width: 68px;
        }
        #DetailButton {
            background-color: transparent;
            color: %4;
            border: 1px solid %3;
        }
        #DetailButton:hover {
            background-color: rgba(127, 127, 127, 0.08);
        }
        #DisconnectButton {
            background-color: %9;
            color: #FFFFFF;
        }
        #DisconnectButton:hover {
            opacity: 0.92;
        }
        #DisconnectButton:pressed {
            opacity: 0.82;
        }
        #DisconnectButton:disabled {
            background-color: %3;
            color: %7;
        }
    )")
        .arg(fontFamily, surface, border, textPrimary)
        .arg(sizeNormal)
        .arg(success)
        .arg(textSecondary)
        .arg(sizeSmall)
        .arg(danger);

    setStyleSheet(qss);
}

// ============================================================
// 3. DeviceRecordCard 操作历史卡片
// ============================================================
DeviceRecordCard::DeviceRecordCard(const QString& time, const QString& action, bool isSuccess, QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 6, 10, 6);

    auto* cardFrame = new QFrame(this);
    cardFrame->setObjectName("RecordCardFrame");
    cardFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    applyCardShadow(cardFrame);

    auto* cardLayout = new QHBoxLayout(cardFrame);
    cardLayout->setContentsMargins(16, 12, 16, 12);
    cardLayout->setSpacing(14);

    auto* leftLayout = new QVBoxLayout();
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto* timeLabel = new QLabel(time, cardFrame);
    timeLabel->setObjectName("RecordTime");
    timeLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    timeLabel->setWordWrap(false);
    // timeLabel->setWordWrap
    timeLabel->setWordWrap(Qt::ElideRight);

    auto* actionLabel = new QLabel(action, cardFrame);
    actionLabel->setObjectName("RecordAction");
    actionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    actionLabel->setWordWrap(false);
    actionLabel->setWordWrap(Qt::ElideRight);

    leftLayout->addWidget(timeLabel);
    leftLayout->addWidget(actionLabel);

    auto* statusBadge = new QLabel(cardFrame);
    statusBadge->setFixedHeight(26);
    statusBadge->setFixedWidth(64);
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    if (isSuccess) {
        statusBadge->setText("成功");
        statusBadge->setObjectName("BadgeSuccess");
    }
    else {
        statusBadge->setText("失败");
        statusBadge->setObjectName("BadgeFailed");
    }

    cardLayout->addLayout(leftLayout, 1);
    cardLayout->addWidget(statusBadge, 0, Qt::AlignVCenter);

    mainLayout->addWidget(cardFrame);

    if (auto* session = UiSession::instance()) {
        connect(session, &UiSession::themeChanged, this, &DeviceRecordCard::updateStyle);
        connect(session, &UiSession::fontChanged, this, &DeviceRecordCard::updateStyle);
    }
    updateStyle();
}

void DeviceRecordCard::updateStyle()
{
    auto* session = UiSession::instance();
    if (!session) return;

    const auto theme = session->theme();
    const auto fontMap = session->font();

    const QString fontFamily = fontMap.value("family", "Microsoft YaHei").toString();
    const int sizeNormal = fontMap.value("sizeNormal", 14).toInt();
    const int sizeSmall = fontMap.value("sizeSmall", 12).toInt();

    const QString border = theme.value("border").toString();
    const QString surface = theme.value("surface").toString();
    const QString textPrimary = theme.value("textPrimary").toString();
    const QString textSecondary = theme.value("textSecondary").toString();
    const QString success = theme.value("success").toString();
    const QString danger = theme.value("danger").toString();

    const QString qss = QString(R"(
        DeviceRecordCard {
            font-family: '%1';
        }
        DeviceRecordCard QLabel {
            background: transparent;
        }
        #RecordCardFrame {
            background-color: %2;
            border: 1px solid %3;
            border-radius: 12px;
        }
        #RecordTime {
            color: %6;
            font-size: %8px;
        }
        #RecordAction {
            color: %4;
            font-size: %5px;
            font-weight: 600;
        }
        #BadgeSuccess, #BadgeFailed {
            color: #FFFFFF;
            border-radius: 13px;
            font-size: %8px;
            font-weight: 600;
            padding: 0 10px;
        }
        #BadgeSuccess {
            background-color: %7;
        }
        #BadgeFailed {
            background-color: %9;
        }
    )")
        .arg(fontFamily, surface, border, textPrimary)
        .arg(sizeNormal)
        .arg(textSecondary)
        .arg(success)
        .arg(sizeSmall)
        .arg(danger);

    setStyleSheet(qss);
}