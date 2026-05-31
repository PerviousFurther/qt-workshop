#include <QLayout>
#include <QPushButton>
#include <QDateTime>
// #include <QGraphicsOpacityEffect>
// #include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QProgressBar>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include "Extensible.hpp"
#include "Session.hpp"
#include "Loading.hpp"

LoadingWidget::LoadingWidget(QWidget* parent)
    : QWidget(parent) {
    this->setFixedWidth(parent->width());
    this->setFixedHeight(parent->height());

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->addStretch();

    // 1. 初始化所有核心控件
    panel_ = new QFrame;
    panel_->setObjectName("loadingPanel");
    panel_->setFixedWidth(240);
    panel_->setFixedHeight(240);

    auto* panelLayout = new QVBoxLayout(panel_);
    panelLayout->setContentsMargins(36, 32, 36, 32);
    panelLayout->setSpacing(10);

    icon_ = new QLabel(lstr("Ring"));
    icon_->setAlignment(Qt::AlignCenter);
    icon_->setFixedSize(72, 72);

    title_ = new QLabel(lstr("RingApp"));
    title_->setAlignment(Qt::AlignCenter);

    subtitle_ = new QLabel(lstr("加载中..."));
    subtitle_->setAlignment(Qt::AlignCenter);
    subtitle_->setWordWrap(true);

    progress_ = new QProgressBar;
    progress_->setRange(0, 0);
    progress_->setTextVisible(false);
    progress_->setFixedHeight(6);

    // 2. 完美的布局组装
    panelLayout->addWidget(icon_, 0, Qt::AlignHCenter);
    panelLayout->addWidget(title_);
    panelLayout->addWidget(subtitle_);
    panelLayout->addStretch();
    panelLayout->addWidget(progress_);

    root->addWidget(panel_, 0, Qt::AlignHCenter);
    root->addStretch();

    // 3. 应用初始样式并监听主题改变信号
    this->updateTheme();

    connect(UiSession::instance(), &UiSession::themeChanged, 
        this, &LoadingWidget::updateTheme);
    connect(UiSession::instance(), &UiSession::fontChanged,
        this, &LoadingWidget::updateTheme);
}

void LoadingWidget::updateTheme() {
    auto* ui = UiSession::instance();
    const QVariantMap theme = ui->theme();
    const QVariantMap fontCfg = ui->font();

    auto surface = map(theme, lstr("surface"), lstr("#FFFFFF"));
    auto border = map(theme, lstr("border"), lstr("#D9E2EC"));
    auto grid = map(theme, lstr("grid"), lstr("#E5EEF5"));
    auto textPrimary = map(theme, lstr("textPrimary"), lstr("#1F2937"));
    auto textSecondary = map(theme, lstr("textSecondary"), lstr("#64748B"));
    auto primary = map(theme, lstr("primary"), lstr("#3B82F6"));

    auto fontFamily = map(fontCfg, lstr("family"), lstr("Microsoft YaHei"));
    auto titleSize = map(fontCfg, lstr("titleSize"), 20);
    auto titleWeight = map(fontCfg, lstr("titleWeight"), 700);
    auto bodySize = map(fontCfg, lstr("bodySize"), 14);

    // 更新各组件的 QSS
    panel_->setStyleSheet(lstr(
        "QFrame#loadingPanel {"
        " background: %1;"
        " border: 1px solid %2;"
        " border-radius: 12px;"
        "}"
    ).arg(alpha(surface, 0.95), border));

    icon_->setStyleSheet(lstr(
        "background: %1;"
        "color: %2;"
        "border: 1px solid %3;"
        "border-radius: 12px;"
        "font-family: '%4';"
        "font-size: 22px;"
        "font-weight: 800;"
    ).arg(primary, surface, border, fontFamily));

    title_->setStyleSheet(lstr(
        "font-family: '%1'; font-size: %2px; font-weight: %3; color: %4;"
    ).arg(fontFamily).arg(titleSize).arg(titleWeight).arg(textPrimary));

    subtitle_->setStyleSheet(lstr(
        "font-family: '%1'; font-size: %2px; color: %3;"
    ).arg(fontFamily).arg(bodySize).arg(textSecondary));

    progress_->setStyleSheet(lstr(
        "QProgressBar {"
        " background: %1;"
        " border: none;"
        " border-radius: 3px;"
        "}"
        "QProgressBar::chunk {"
        " background: %2;"
        " border-radius: 3px;"
        "}"
    ).arg(grid, primary));

    // 触发 paintEvent 重新绘制背景渐变
    this->update();
}

void LoadingWidget::fadeIn() {
}

void LoadingWidget::fadeOut() {
}

void LoadingWidget::startLoading(const QVariantMap& params) {
    subtitle_->setText(params.value(lstr("text"), lstr("加载中...")).toString());
    this->fadeIn();
}

void LoadingWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    qint64 ms = QDateTime::currentMSecsSinceEpoch();
    double angleX = ms + 1.5;
    double angleY = ms;
    int centerX = width() / 2 + static_cast<int>(width() * 0.25 * cos(angleX));
    int centerY = height() / 2 + static_cast<int>(height() * 0.25 * sin(angleY));

    int radius = qMax(width(), height());
    QRadialGradient gradient(centerX, centerY, radius);
    auto theme = UiSession::instance()->theme();
    gradient.setColorAt(0.0, theme.value("primary").toString());
    gradient.setColorAt(0.5, theme.value("secondary").toString());
    gradient.setColorAt(1.0, theme.value("signalBody").toString());

    painter.fillRect(rect(), gradient);

    QWidget::paintEvent(event);
}

void LoadingWidget::animateTo(qreal value) {
    Q_UNUSED(value);
}