#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QFontMetrics>
#include <QSpacerItem>

#include "CardDriver.hpp"
#include "Driver.hpp"
#include "Backend/Ui/Session.hpp"

static void applyCardShadow(QFrame* frame)
{
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
DeviceRecordCard::DeviceRecordCard(int id, const QString& name, const QString& filePath, QWidget* parent)
    : QWidget(parent)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 6, 10, 6);

    auto* cardFrame = new QFrame(this);
    cardFrame->setObjectName("RecordCardFrame");
    cardFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    applyCardShadow(cardFrame);

    // 2. 整体换成 QHBoxLayout，方便右侧塞入按钮
    auto* cardLayout = new QHBoxLayout(cardFrame);
    cardLayout->setContentsMargins(16, 12, 16, 12);
    cardLayout->setSpacing(12);

    // 左侧：名字和路径直接上下堆叠
    auto* textLayout = new QVBoxLayout();
    textLayout->setSpacing(4);

    // 文件/设备名字
    auto* nameLabel = new QLabel(name, cardFrame);
    nameLabel->setObjectName("RecordName");
    nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    nameLabel->setWordWrap(false);

    // 文件路径
    auto* pathLabel = new QLabel(filePath, cardFrame);
    pathLabel->setObjectName("RecordPath");
    pathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    pathLabel->setWordWrap(false);

    textLayout->addWidget(nameLabel);
    textLayout->addWidget(pathLabel);

    cardLayout->addLayout(textLayout);

    auto* detailButton = new QPushButton(lstr("查看详情"), cardFrame);
    detailButton->setObjectName("DetailButton");
    detailButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    cardLayout->addWidget(detailButton);

    mainLayout->addWidget(cardFrame);

    // 4. 信号槽绑定：点击按钮时，引发自定义信号
    connect(detailButton, &QPushButton::clicked, this, [this, id, name]() {
        emit this->requestRecordPage(id, name);
    });

    if (auto* session = UiSession::instance()) {
        connect(session, &UiSession::themeChanged, this, &DeviceRecordCard::updateStyle);
        connect(session, &UiSession::fontChanged, this, &DeviceRecordCard::updateStyle);
    }
    this->updateStyle();
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

    auto primary = theme.value(UiField::Primary).toString();
    const QString border = theme.value("border").toString();
    const QString surface = theme.value("surface").toString();
    const QString textPrimary = theme.value("textPrimary").toString();
    const QString textSecondary = theme.value("textSecondary").toString();

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
        #RecordName {
            color: %4;
            font-size: %5px;
            font-weight: 600;
        }
        #RecordPath {
            color: %6;
            font-size: %7px;
        }
        QPushButton {
            background-color: %8;
            color: #FFFFFF;
            border: none;
            border-radius: 6px;
            padding: 6px 14px;
            font-size: %7px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: %9;
        }
        QPushButton:pressed {
            background-color: %8;
        }
    )")
        .arg(fontFamily, surface, border, textPrimary)
        .arg(sizeNormal)
        .arg(textSecondary)
        .arg(sizeSmall)
        .arg(primary);

    setStyleSheet(qss);
}