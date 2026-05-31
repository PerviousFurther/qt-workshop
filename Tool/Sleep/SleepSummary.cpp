#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QMouseEvent>
#include <QColor>
#include <QtMath>
#include <cmath>

#include "Tool/Basic/CardPieChart.hpp"
#include "Tool/Basic/CardBarChart.hpp"
#include "Tool/Basic/CardWaveform.hpp"

#include "Backend/Ui/Session.hpp"
#include "SleepSummary.hpp"

PSGSummaryCard::PSGSummaryCard(QWidget* parent) : QWidget(parent) {
    this->initUI();

    auto* session = UiSession::instance();
    if (session) {
        connect(session, &UiSession::themeChanged, this, &PSGSummaryCard::syncWithSession);
        connect(session, &UiSession::fontChanged, this, &PSGSummaryCard::syncWithSession);
    }

    this->setupMockData();
    this->updateDashboardData("7.5 小时", 14.5, 88); // 初始展示有数据状态
    this->syncWithSession();
    this->setCursor(Qt::PointingHandCursor);
}

void PSGSummaryCard::initUI() {
    auto* globalLayout = new QVBoxLayout(this);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->setSpacing(14);

    // 主背景容器
    containerFrame_ = new QFrame(this);
    containerFrame_->setObjectName("MainDashboardContainer");
    auto* mainLayout = new QVBoxLayout(containerFrame_);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(20);

    // ==========================================
    // 1. 顶部区域：标题 与 三大核心指标横向排列
    // ==========================================
    auto* topBarLayout = new QHBoxLayout();

    // 左侧标题
    auto* titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(6);
    auto* titleLabel = new QLabel("睡眠监测报告", this);
    titleLabel->setObjectName("CardTitle");
    auto* statusLabel = new QLabel("已生成", this);
    statusLabel->setObjectName("StatusTag");
    statusLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(statusLabel);
    topBarLayout->addLayout(titleLayout);

    topBarLayout->addStretch(1);

    // 右侧三大核心指标
    auto* metricsLayout = new QHBoxLayout();
    metricsLayout->setSpacing(48);

    // 指标 1：总睡眠时长
    auto* m1Layout = new QVBoxLayout();
    m1Layout->setSpacing(4);
    auto* m1Header = new QLabel("🕒 总睡眠时长", this);
    m1Header->setObjectName("MetricLabel");
    valueDuration_ = new QLabel(this);
    valueDuration_->setObjectName("MetricValue");
    tipDuration_ = new QLabel("建议 7-9 小时", this);
    tipDuration_->setObjectName("MetricTip");
    m1Layout->addWidget(m1Header);
    m1Layout->addWidget(valueDuration_);
    m1Layout->addWidget(tipDuration_);

    // 指标 2：AHI 指数
    auto* m2Layout = new QVBoxLayout();
    m2Layout->setSpacing(4);
    auto* m2Header = new QLabel("📈 AHI 指数", this);
    m2Header->setObjectName("MetricLabel");
    valueAHI_ = new QLabel(this);
    valueAHI_->setObjectName("MetricValue");
    tipAHI_ = new QLabel("正常范围: 0-5", this);
    tipAHI_->setObjectName("MetricTip");
    m2Layout->addWidget(m2Header);
    m2Layout->addWidget(valueAHI_);
    m2Layout->addWidget(tipAHI_);

    // 指标 3：睡眠效率
    auto* m3Layout = new QVBoxLayout();
    m3Layout->setSpacing(4);
    auto* m3Header = new QLabel("📊 睡眠效率", this);
    m3Header->setObjectName("MetricLabel");
    valueEfficiency_ = new QLabel(this);
    valueEfficiency_->setObjectName("MetricValue");
    tipEfficiency_ = new QLabel("优于 80% 的用户", this);
    tipEfficiency_->setObjectName("MetricTip");
    m3Layout->addWidget(m3Header);
    m3Layout->addWidget(valueEfficiency_);
    m3Layout->addWidget(tipEfficiency_);

    metricsLayout->addLayout(m1Layout);
    metricsLayout->addLayout(m2Layout);
    metricsLayout->addLayout(m3Layout);
    topBarLayout->addLayout(metricsLayout);
    mainLayout->addLayout(topBarLayout);

    // ==========================================
    // 2. 中间区域：分栏子卡片（图表区域）
    // ==========================================
    auto* chartsRowLayout = new QHBoxLayout();
    chartsRowLayout->setSpacing(20);

    // --- 子卡片 1：睡眠结构分析 ---
    auto* pieCard = new QFrame(this);
    pieCard->setObjectName("SubChartCard");
    subCards_.append(pieCard);
    auto* pieCardLayout = new QVBoxLayout(pieCard);
    pieCardLayout->setContentsMargins(16, 16, 16, 16);

    auto* pieTitle = new QLabel("🍕 睡眠结构分析", this);
    pieTitle->setObjectName("SubCardTitle");
    pieChart_ = new PieChartWidget(this);
    pieChart_->setHoleRadius(0.6);
    pieChart_->setMinimumSize(160, 140);

    auto* pieLegendLayout = new QGridLayout();
    pieLegendLayout->setVerticalSpacing(6);
    pieLegendLayout->setHorizontalSpacing(10);
    QStringList labels = { "● 深睡眠", "● 浅睡眠", "● 快速眼动", "● 清醒" };
    QStringList values = { "3.0h  40%", "2.5h  33%", "1.5h  20%", "0.5h  7%" };
    QStringList colors = { "#9254DE", "#52C41A", "#BFBFBF", "#F5222D" };
    for (int i = 0; i < labels.size(); ++i) {
        auto* lLabel = new QLabel(labels[i], this);
        lLabel->setStyleSheet(lstr("color: %1; font-weight: bold;").arg(colors[i]));
        auto* vLabel = new QLabel(values[i], this);
        vLabel->setObjectName("LegendValueLabel");
        pieLegendLayout->addWidget(lLabel, i, 0);
        pieLegendLayout->addWidget(vLabel, i, 1, Qt::AlignRight);
    }

    statusPieCard_ = new QLabel("✔  睡眠结构合理", this);
    statusPieCard_->setObjectName("SubCardStatusTag");
    statusPieCard_->setAlignment(Qt::AlignCenter);

    pieCardLayout->addWidget(pieTitle);
    pieCardLayout->addWidget(pieChart_, 1);
    pieCardLayout->addLayout(pieLegendLayout);
    pieCardLayout->addSpacing(10);
    pieCardLayout->addWidget(statusPieCard_);

    // --- 子卡片 2：每小时 AHI 趋势 ---
    auto* barCard = new QFrame(this);
    barCard->setObjectName("SubChartCard");
    subCards_.append(barCard);
    auto* barCardLayout = new QVBoxLayout(barCard);
    barCardLayout->setContentsMargins(16, 16, 16, 16);

    auto* barTitle = new QLabel("📊 每小时 AHI 趋势", this);
    barTitle->setObjectName("SubCardTitle");
    barChart_ = new BarChartWidget(this);
    barChart_->setYBounds(0, 6);
    barChart_->setGaps(14, 0);
    barChart_->setMinimumSize(160, 140);

    auto* barLegend = new QLabel("● 1-2 正常  ● 2-5 轻度  ● 5-15 中度", this);
    barLegend->setObjectName("LegendValueLabel");
    barLegend->setAlignment(Qt::AlignCenter);

    statusBarCard_ = new QLabel("✔  整体趋势平稳", this);
    statusBarCard_->setObjectName("SubCardStatusTag");
    statusBarCard_->setAlignment(Qt::AlignCenter);

    barCardLayout->addWidget(barTitle);
    barCardLayout->addWidget(barChart_, 1);
    barCardLayout->addWidget(barLegend);
    barCardLayout->addSpacing(14);
    barCardLayout->addWidget(statusBarCard_);

    // --- 子卡片 3：脑电/呼吸波形预览 ---
    auto* waveCard = new QFrame(this);
    waveCard->setObjectName("SubChartCard");
    subCards_.append(waveCard);
    auto* waveCardLayout = new QVBoxLayout(waveCard);
    waveCardLayout->setContentsMargins(16, 16, 16, 16);

    auto* waveTitle = new QLabel("〰 脑电/呼吸波形预览", this);
    waveTitle->setObjectName("SubCardTitle");
    waveformWidget_ = new HistoryWaveformWidget(this);
    waveformWidget_->setMinimumSize(160, 140);
    waveformWidget_->setAllowDrag(false);
    waveformWidget_->setAllowZoom(false);

    statusWaveformCard_ = new QLabel("✔  波形信号质量良好", this);
    statusWaveformCard_->setObjectName("SubCardStatusTag");
    statusWaveformCard_->setAlignment(Qt::AlignCenter);

    waveCardLayout->addWidget(waveTitle);
    waveCardLayout->addWidget(waveformWidget_, 1);
    waveCardLayout->addSpacing(26);
    waveCardLayout->addWidget(statusWaveformCard_);

    chartsRowLayout->addWidget(pieCard, 1);
    chartsRowLayout->addWidget(barCard, 1);
    chartsRowLayout->addWidget(waveCard, 1);
    mainLayout->addLayout(chartsRowLayout, 1);

    globalLayout->addWidget(containerFrame_);

    // ==========================================
    // 2.5 空数据状态
    // ==========================================
    auto* emptyStateFrame = new QFrame(this);
    emptyStateFrame->setObjectName("EmptyStateContainer");
    emptyStateFrame->setVisible(false);
    emptyStateFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto* emptyLayout = new QVBoxLayout(emptyStateFrame);
    emptyLayout->setContentsMargins(24, 20, 24, 20);
    emptyLayout->setSpacing(10);
    emptyLayout->setAlignment(Qt::AlignCenter);

    auto* emptyIcon = new QLabel("📭", emptyStateFrame);
    emptyIcon->setObjectName("EmptyStateIcon");
    emptyIcon->setAlignment(Qt::AlignCenter);

    auto* emptyTitle = new QLabel("暂无睡眠数据", emptyStateFrame);
    emptyTitle->setObjectName("EmptyStateTitle");
    emptyTitle->setAlignment(Qt::AlignCenter);

    auto* emptyDesc = new QLabel("当前没有可展示的报告内容。", emptyStateFrame);
    emptyDesc->setObjectName("EmptyStateDesc");
    emptyDesc->setAlignment(Qt::AlignCenter);
    emptyDesc->setWordWrap(true);

    auto* emptyHint = new QLabel("请先同步设备数据，或在生成报告后刷新页面。", emptyStateFrame);
    emptyHint->setObjectName("EmptyStateHint");
    emptyHint->setAlignment(Qt::AlignCenter);
    emptyHint->setWordWrap(true);

    emptyLayout->addStretch(1);
    emptyLayout->addWidget(emptyIcon);
    emptyLayout->addWidget(emptyTitle);
    emptyLayout->addWidget(emptyDesc);
    emptyLayout->addWidget(emptyHint);
    emptyLayout->addStretch(1);

    globalLayout->addWidget(emptyStateFrame);

    // ==========================================
    // 3. 底部全宽提示条
    // ==========================================
    bottomBarFrame_ = new QFrame(this);
    bottomBarFrame_->setObjectName("BottomTipBar");
    auto* bottomLayout = new QHBoxLayout(bottomBarFrame_);
    bottomLayout->setContentsMargins(20, 10, 20, 10);

    auto* tipIcon = new QLabel("📄", this);
    auto* tipText = new QLabel("点击卡片任意区域进入报告详情页查看完整生理分期", this);
    tipText->setObjectName("BottomTipText");
    auto* arrowLabel = new QLabel("▶", this);
    arrowLabel->setObjectName("BottomTipArrow");

    bottomLayout->addWidget(tipIcon);
    bottomLayout->addWidget(tipText);
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(arrowLabel);

    globalLayout->addWidget(bottomBarFrame_);
}

void PSGSummaryCard::setupMockData() {
    // 饼图基础数据
    pieChart_->setData(
        { 40, 33, 20, 7 },
        { QColor("#9254DE"), QColor("#52C41A"), QColor("#BFBFBF"), QColor("#F5222D") }
    );

    // 柱状图基础数据
    barChart_->setData({ {1.0}, {2.0}, {3.0}, {4.0}, {5.0}, {1.3} });
    barChart_->setSeriesColors({
        QColor("#9254DE"), QColor("#FFA940"), QColor("#73D13D"),
        QColor("#52C41A"), QColor("#1890FF"), QColor("#9254DE")
    });

    // 波形生成模拟迁移
    QList<QPointF> wavePoints;
    wavePoints.reserve(250);

    for (int x = 0; x < 250; ++x) {
        double rad1 = x * 0.1;
        double rad2 = x * 0.25;
        double y = 50.0 + 20.0 * std::sin(rad1) + 8.0 * std::cos(rad2);
        wavePoints.append(QPointF(static_cast<qreal>(x), y));
    }

    waveformWidget_->clearCurves();
    waveformWidget_->addCurve(wavePoints, QColor("#52C41A"), 1.8);
}

void PSGSummaryCard::syncWithSession() {
    auto* session = UiSession::instance();
    if (!session) return;

    auto theme = session->theme();
    auto font = session->font();

    QString fontFamily = font.value("family", "Microsoft YaHei").toString();
    int titleSize = font.value("titleSize", 20).toInt();
    int subtitleSize = font.value("subtitleSize", 16).toInt();
    int bodySize = font.value("bodySize", 14).toInt();
    int captionSize = font.value("captionSize", 10).toInt();

    QString surface = theme.value("surface").toString();
    QString border = theme.value("border").toString();
    QString grid = theme.value("grid").toString();
    QString primary = theme.value("primary").toString();
    QString success = theme.value("success").toString();
    QString danger = theme.value("danger").toString();
    QString textPrimary = theme.value("textPrimary").toString();
    QString textSecondary = theme.value("textSecondary").toString();

    int bigMetricSize = titleSize + 4;

    QColor successColor(success);
    QString statusTagBg = lstr("rgba(%1, %2, %3, 0.1)")
        .arg(successColor.red())
        .arg(successColor.green())
        .arg(successColor.blue());
    auto statusTagBorder = lstr("rgba(%1, %2, %3, 0.3)")
        .arg(successColor.red())
        .arg(successColor.green())
        .arg(successColor.blue());

    auto styleSheet = lstr(R"(
        QFrame#MainDashboardContainer {
            background-color: [SURFACE];
            border: 1px solid [BORDER];
            border-radius: 12px;
        }
        QLabel#CardTitle {
            font-family: '[FONT_FAMILY]';
            font-size: [SUBTITLE_SIZE]px;
            font-weight: bold;
            color: [TEXT_PRIMARY];
        }
        QLabel#StatusTag {
            font-family: '[FONT_FAMILY]';
            font-size: [CAPTION_SIZE]px;
            color: [SUCCESS];
            background-color: [STATUS_TAG_BG];
            border: 1px solid [STATUS_TAG_BORDER];
            border-radius: 4px;
            padding: 2px 6px;
        }
        QLabel#MetricLabel {
            font-family: '[FONT_FAMILY]';
            font-size: [BODY_SIZE]px;
            color: [TEXT_SECONDARY];
        }
        QLabel#MetricValue {
            font-family: '[FONT_FAMILY]';
            font-size: [BIG_METRIC_SIZE]px;
            font-weight: bold;
            color: [TEXT_PRIMARY];
        }
        QLabel#MetricTip {
            font-family: '[FONT_FAMILY]';
            font-size: [CAPTION_SIZE]px;
            color: [TEXT_SECONDARY];
        }
        QFrame#SubChartCard {
            background-color: rgba(255, 255, 255, 0.02);
            border: 1px solid [BORDER];
            border-radius: 8px;
            padding: 10px;
        }
        QFrame#SubChartCard:hover {
            border: 1px solid [PRIMARY];
            background-color: rgba(255, 255, 255, 0.04);
        }
        QLabel#SubCardTitle {
            font-family: '[FONT_FAMILY]';
            font-size: [BODY_SIZE]px;
            font-weight: bold;
            color: [TEXT_PRIMARY];
        }
        QLabel#LegendValueLabel {
            font-family: '[FONT_FAMILY]';
            font-size: [CAPTION_SIZE]px;
            color: [TEXT_SECONDARY];
        }
        QLabel#SubCardStatusTag {
            font-family: '[FONT_FAMILY]';
            font-size: [BODY_SIZE]px;
            color: [SUCCESS];
            background-color: [STATUS_TAG_BG];
            border-radius: 6px;
            padding: 6px;
        }
        QFrame#BottomTipBar {
            background-color: rgba(255, 255, 255, 0.03);
            border: 1px solid [BORDER];
            border-radius: 8px;
        }
        QFrame#MainDashboardContainer:hover + QFrame#BottomTipBar, QFrame#BottomTipBar:hover {
            border: 1px solid [PRIMARY];
            background-color: rgba(255, 255, 255, 0.05);
        }
        QLabel#BottomTipText {
            font-family: '[FONT_FAMILY]';
            font-size: [BODY_SIZE]px;
            color: [TEXT_SECONDARY];
        }
        QLabel#BottomTipArrow {
            color: [PRIMARY];
            font-weight: bold;
        }

        QFrame#EmptyStateContainer {
            background-color: rgba(255, 255, 255, 0.02);
            border: 1px dashed [BORDER];
            border-radius: 12px;
        }
        QFrame#EmptyStateContainer:hover {
            border: 1px solid [PRIMARY];
            background-color: rgba(255, 255, 255, 0.04);
        }
        QLabel#EmptyStateIcon {
            font-size: 48px;
        }
        QLabel#EmptyStateTitle {
            font-family: '[FONT_FAMILY]';
            font-size: [SUBTITLE_SIZE]px;
            font-weight: bold;
            color: [TEXT_PRIMARY];
        }
        QLabel#EmptyStateDesc {
            font-family: '[FONT_FAMILY]';
            font-size: [BODY_SIZE]px;
            color: [TEXT_SECONDARY];
        }
        QLabel#EmptyStateHint {
            font-family: '[FONT_FAMILY]';
            font-size: [CAPTION_SIZE]px;
            color: [PRIMARY];
        }
    )");
    styleSheet.replace("[SURFACE]", surface)
        .replace("[BORDER]", border)
        .replace("[PRIMARY]", primary)
        .replace("[FONT_FAMILY]", fontFamily)
        .replace("[SUBTITLE_SIZE]", QString::number(subtitleSize))
        .replace("[TEXT_PRIMARY]", textPrimary)
        .replace("[CAPTION_SIZE]", QString::number(captionSize))
        .replace("[SUCCESS]", success)
        .replace("[STATUS_TAG_BG]", statusTagBg)
        .replace("[STATUS_TAG_BORDER]", statusTagBorder)
        .replace("[BODY_SIZE]", QString::number(bodySize))
        .replace("[TEXT_SECONDARY]", textSecondary)
        .replace("[BIG_METRIC_SIZE]", QString::number(bigMetricSize));

    this->setStyleSheet(styleSheet);

    // 校准当前 AHI 文本的色调
    double currentAhi = valueAHI_->text().toDouble();
    if (currentAhi >= 15.0) {
        valueAHI_->setStyleSheet(lstr("color: %1;").arg(danger));
    }
    else {
        valueAHI_->setStyleSheet(lstr("color: %1;").arg(primary));
    }

    // 如果当前是空态，保持空态样式可见；如果是数据态，确保主卡显示
    auto* emptyFrame = this->findChild<QFrame*>("EmptyStateContainer");
    if (emptyFrame && !emptyFrame->isVisible()) {
        containerFrame_->setVisible(true);
    }
}

void PSGSummaryCard::updateDashboardData(const QString& duration, double ahi, int efficiency) {
    auto* emptyFrame = this->findChild<QFrame*>("EmptyStateContainer");
    auto* tipText = this->findChild<QLabel*>("BottomTipText");

    const bool hasValidData = !duration.trimmed().isEmpty()
        && std::isfinite(ahi)
        && ahi >= 0.0
        && efficiency >= 0;

    if (!hasValidData) {
        // 空数据态
        containerFrame_->hide();
        if (emptyFrame) {
            emptyFrame->show();
        }

        if (tipText) {
            tipText->setText("当前暂无可展示的睡眠数据，请先同步设备或生成报告");
        }

        valueDuration_->setText("--");
        valueAHI_->setText("--");
        valueAHI_->setStyleSheet("color: #8C8C8C; font-weight: bold;");
        valueEfficiency_->setText("--");

        this->setCursor(Qt::ArrowCursor);
        return;
    }

    // 有数据态
    if (emptyFrame) {
        emptyFrame->hide();
    }
    containerFrame_->show();

    if (tipText) {
        tipText->setText("点击卡片任意区域进入报告详情页查看完整生理分期");
    }

    valueDuration_->setText(duration);
    valueAHI_->setText(QString::number(ahi, 'f', 1));
    valueEfficiency_->setText(lstr("%1%").arg(efficiency));

    auto* session = UiSession::instance();
    QString dangerColor = session ? session->theme().value("danger").toString() : "#FF5252";
    QString primaryColor = session ? session->theme().value("primary").toString() : "#00D2FF";

    if (ahi >= 15.0) {
        valueAHI_->setStyleSheet(lstr("color: %1; font-weight: bold;").arg(dangerColor));
    }
    else {
        valueAHI_->setStyleSheet(lstr("color: %1; font-weight: bold;").arg(primaryColor));
    }

    this->setCursor(Qt::PointingHandCursor);
}

void PSGSummaryCard::mouseReleaseEvent(QMouseEvent* event) {
    auto* emptyFrame = this->findChild<QFrame*>("EmptyStateContainer");
    const bool isEmptyState = emptyFrame && emptyFrame->isVisible();

    if (event->button() == Qt::LeftButton) {
        if (!isEmptyState) {
            emit clicked();
        }
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}