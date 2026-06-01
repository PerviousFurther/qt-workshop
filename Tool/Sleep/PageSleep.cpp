#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QRandomGenerator>
#include <QtMath>
#include <QTimer>
#include <QScroller>
#include <QFont>

#include "Backend/Ui/Session.hpp"
#include "PageSleep.hpp"

namespace {
	constexpr int kWavePointCount = 96;
	constexpr int kWaveTimerMs = 160;
}

auto enableHover(QWidget* w) {
	w->setAttribute(Qt::WA_Hover, true);
	w->setMouseTracking(true);
};

PSGMainPage::PSGMainPage(QWidget* parent)
	: QWidget(parent) {
	setObjectName("PSGMainPage");
	setAttribute(Qt::WA_StyledBackground, true);
	initLayout();
	generateMockData();
	initStyleSheet();

	auto* session = UiSession::instance();
	connect(session, &UiSession::themeChanged, this, &PSGMainPage::initStyleSheet, Qt::UniqueConnection);
	connect(session, &UiSession::themeChanged, this, &PSGMainPage::generateMockData, Qt::UniqueConnection);
	connect(session, &UiSession::fontChanged, this, &PSGMainPage::initStyleSheet, Qt::UniqueConnection);

	mockDataTimer_ = new QTimer(this);
	mockDataTimer_->setTimerType(Qt::CoarseTimer);

	connect(mockDataTimer_, &QTimer::timeout, this, [this]() {
		tickCount_++;

		// 迁移点 1：将 QList<QPoint> 改为 QList<QPointF>
		QList<QPointF> eegPoints;
		QList<QPointF> flowPoints;
		QList<QPointF> ecgPoints;
		QList<QPointF> spo2Points;

		eegPoints.reserve(kWavePointCount);
		flowPoints.reserve(kWavePointCount);
		ecgPoints.reserve(kWavePointCount);
		spo2Points.reserve(kWavePointCount);

		const int baseTick = tickCount_;
		auto* rand = QRandomGenerator::global();

		for (int i = 0; i < kWavePointCount; ++i) {
			const qreal x = i * 6.0;
			const double t = baseTick + i;

			// ==========================================
			// 1. EEG 脑电波模拟 (多频叠加 + 连续扰动)
			// ==========================================
			double eegBio = qSin(t * 0.15) * 8.0
				+ qSin(t * 0.45) * 4.0
				+ qSin(t * 0.85) * 1.5;
			double eegNoise = rand->bounded(-30, 31) * 0.1;
			// 迁移点 2：直接追加 QPointF，移除 static_cast<int>
			eegPoints.append(QPointF(x, 40.0 + eegBio + eegNoise));

			// ==========================================
			// 2. Nasal Airflow 呼吸气流 (非对称快吸慢呼)
			// ==========================================
			double respirationPhase = fmod(t * 0.04, 2.0 * M_PI);
			double flowBio = qSin(respirationPhase);
			if (flowBio > 0) {
				flowBio = qPow(flowBio, 0.7);
			}
			else {
				flowBio = -qPow(qAbs(flowBio), 1.3);
			}
			// 迁移点 3：升级为 QPointF
			flowPoints.append(QPointF(x, 110.0 + (flowBio * 20.0) + rand->bounded(-1, 2)));

			// ==========================================
			// 3. ECG 心电图 (基于生理周期的 P-QRS-T 模拟)
			// ==========================================
			double ecgPeriod = 40.0;
			double ecgPhase = fmod(t, ecgPeriod);
			double ecgBio = 0.0;

			if (ecgPhase > 2 && ecgPhase < 8) {
				ecgBio += 3.0 * qSin((ecgPhase - 2) * M_PI / 6.0);
			}
			if (ecgPhase >= 10 && ecgPhase <= 14) {
				double qrsT = ecgPhase - 12;
				ecgBio += 35.0 * qExp(-qPow(qrsT / 0.8, 2.0));
				ecgBio -= 5.0 * qExp(-qPow((qrsT + 1.2) / 0.5, 2.0));
				ecgBio -= 7.0 * qExp(-qPow((qrsT - 1.2) / 0.5, 2.0));
			}
			if (ecgPhase > 18 && ecgPhase < 28) {
				ecgBio += 6.0 * qSin((ecgPhase - 18) * M_PI / 10.0);
			}
			ecgBio += rand->bounded(-10, 11) * 0.1;
			// 迁移点 4：升级为 QPointF
			ecgPoints.append(QPointF(x, 190.0 - ecgBio));

			// ==========================================
			// 4. SpO2 血氧容积波 (与心率同步的 PPG 脉搏波)
			// ==========================================
			double ppgPhase = ecgPhase;
			double ppgBio = 0.0;

			// 主脉冲波 + 重搏突起波拟合
			ppgBio += 12.0 * qExp(-qPow((ppgPhase - 15) / 4.0, 2.0));
			ppgBio += 4.0 * qExp(-qPow((ppgPhase - 21) / 3.0, 2.0));

			qreal spo2BaseY = 270.0;
			// 迁移点 5：升级为 QPointF
			spo2Points.append(QPointF(x, spo2BaseY - ppgBio + rand->bounded(0, 2)));
		}

		// 从 UiSession 动态提取对应的通道颜色
		auto* sessionNow = UiSession::instance();
		QColor eegColor = sessionNow ? QColor(sessionNow->theme().value("signalEEG").toString()) : QColor("#0EA5E9");
		QColor respColor = sessionNow ? QColor(sessionNow->theme().value("signalResp").toString()) : QColor("#10B981");
		QColor ecgColor = sessionNow ? QColor(sessionNow->theme().value("signalECG").toString()) : QColor("#2563EB");
		QColor spo2Color = sessionNow ? QColor(sessionNow->theme().value("signalSpO2").toString()) : QColor("#8B5CF6");

		if (!eegColor.isValid())  eegColor = QColor("#0EA5E9");
		if (!respColor.isValid()) respColor = QColor("#10B981");
		if (!ecgColor.isValid())  ecgColor = QColor("#2563EB");
		if (!spo2Color.isValid()) spo2Color = QColor("#8B5CF6");

		// 此时传递 QList<QPointF> 完美匹配新接口
		waveformMonitor_->clearCurves();
		waveformMonitor_->addCurve(eegPoints, eegColor, 1.5);
		waveformMonitor_->addCurve(flowPoints, respColor, 1.8);
		waveformMonitor_->addCurve(ecgPoints, ecgColor, 1.5);
		waveformMonitor_->addCurve(spo2Points, spo2Color, 2.0);
		});

	mockDataTimer_->start(kWaveTimerMs);
}

void PSGMainPage::initStyleSheet() {
	auto* session = UiSession::instance();
	QVariantMap theme = session ? session->theme() : QVariantMap();
	QVariantMap font = session ? session->font() : QVariantMap();

	// 1. 基础面板与网格色彩
	QString bg = theme.value("background", "#0F172A").toString();
	QString surface = theme.value("surface", "#1E293B").toString();
	QString border = theme.value("border", "#334155").toString();
	QString grid = theme.value("grid", "#1E293B").toString();

	// 2. 完美的级联文本色系提取 (Primary -> Secondary -> Hint)
	QString textPrimary = theme.value("textPrimary", "#F8FAFC").toString();
	QString textSecondary = theme.value("textSecondary", "#94A3B8").toString();
	QString textHint = theme.value("textHint", "#475569").toString();

	// 3. 核心品牌状态色
	QString primary = theme.value("primary", "#38BDF8").toString();
	QString danger = theme.value("danger", "#F87171").toString();
	QString accent = theme.value("accent", "#A855F7").toString();

	// 4. 字体规格提取
	QString fontFamily = font.value("family", "Microsoft YaHei").toString();
	int sizeNormal = font.value("sizeNormal", 14).toInt();
	int sizeTitle = font.value("sizeTitle", 20).toInt();

	// 向自定义绘制的绘图组件显式传递 QFont 确保字体完全解耦
	QFont targetFont(fontFamily, sizeNormal);
	if (waveformMonitor_) { waveformMonitor_->setFont(targetFont);   waveformMonitor_->update(); }
	if (stagePieChart_) { stagePieChart_->setFont(targetFont);     stagePieChart_->update(); }
	if (hypnogramChart_) { hypnogramChart_->setFont(targetFont);    hypnogramChart_->update(); }
	if (positionBarChart_) { positionBarChart_->setFont(targetFont);  positionBarChart_->update(); }
	if (respiratoryEvents_) { respiratoryEvents_->setFont(targetFont); respiratoryEvents_->update(); }
	if (apneaBarChart_) { apneaBarChart_->setFont(targetFont);     apneaBarChart_->update(); }

	this->setStyleSheet(QString(R"(
		QWidget#PSGMainPage {
			background-color: %1;
			color: %2;
			font-family: "%3";
		}

		QScrollArea#MainScrollArea {
			background-color: transparent;
			border: none;
		}
		QScrollArea#MainScrollArea QWidget {
			background-color: transparent;
		}

		/* 运用 grid 与 border 渲染滚动条 */
		QScrollArea#MainScrollArea QScrollBar:vertical {
			border: none;
			background: %4;
			width: 6px;
			margin: 0px;
			border-radius: 3px;
		}
		QScrollArea#MainScrollArea QScrollBar::handle:vertical {
			background: %5;
			min-height: 40px;
			border-radius: 3px;
		}
		QScrollArea#MainScrollArea QScrollBar::handle:vertical:hover {
			background: %6;
		}
		QScrollArea#MainScrollArea QScrollBar::add-line:vertical,
		QScrollArea#MainScrollArea QScrollBar::sub-line:vertical {
			height: 0px;
		}

		QLabel#PageTitle {
			font-size: %7px;
			font-weight: 700;
			color: %2;
			letter-spacing: 1px;
		}

		QLabel#PageSubTitle {
			font-size: 12px;
			color: %10; /* 注入 textSecondary 次要说明文字 */
		}

		QPushButton#ExitButton {
			background-color: transparent;
			color: %2;
			border: 1px solid %5;
			border-radius: 6px;
			padding: 6px 20px;
			font-size: 13px;
			font-weight: 500;
		}
		QPushButton#ExitButton:hover {
			background-color: %8;
			color: #FFFFFF;
			border: 1px solid %8;
		}

		/* 卡片背景与悬停边框完美融合 */
		QFrame#CardFrame {
			background-color: %9;
			border: 1px solid %5;
			border-radius: 12px;
		}
		QFrame#CardFrame:hover {
			border: 1px solid %6;
		}

		QLabel#CardTitle {
			font-size: %11px;
			font-weight: 600;
			color: %2;
			border-left: 4px solid %6;
			padding-left: 12px;
			background: transparent;
		}

		QLabel#CardDesc {
			font-size: 11px;
			color: %12; /* 注入 textHint 暗示文本颜色作为底层注解 */
			background: transparent;
		}
	)")
		.arg(bg)             // %1
		.arg(textPrimary)    // %2
		.arg(fontFamily)     // %3
		.arg(grid)           // %4
		.arg(border)         // %5
		.arg(primary)        // %6
		.arg(sizeTitle)      // %7
		.arg(danger)         // %8
		.arg(surface)        // %9
		.arg(textSecondary)  // %10
		.arg(sizeNormal)     // %11
		.arg(textHint)       // %12
	);
}

void PSGMainPage::initLayout() {
	auto* mainVerticalLayout = new QVBoxLayout(this);
	mainVerticalLayout->setContentsMargins(28, 24, 28, 24);
	mainVerticalLayout->setSpacing(24);

	auto* headerLayout = new QHBoxLayout();
	headerLayout->setContentsMargins(4, 0, 4, 4);

	auto* titleLayout = new QVBoxLayout();
	auto* pageTitle = new QLabel("多导睡眠监测高级工作站", this);
	pageTitle->setObjectName("PageTitle");
	auto* pageSubTitle = new QLabel("PSG Real-time Diagnostic Control Center (Standard Mode)", this);
	pageSubTitle->setObjectName("PageSubTitle");
	titleLayout->addWidget(pageTitle);
	titleLayout->addWidget(pageSubTitle);

	auto* exitButton = new QPushButton("退出工作站", this);
	exitButton->setObjectName("ExitButton");
	exitButton->setCursor(Qt::PointingHandCursor);

	connect(exitButton, &QPushButton::clicked, this, [this]() {
		if (UiSession::instance()) { 
			while (UiSession::instance()->pop() != this);
			delete this;
		}
	});

	headerLayout->addLayout(titleLayout);
	headerLayout->addStretch();
	headerLayout->addWidget(exitButton);

	mainVerticalLayout->addLayout(headerLayout);

	auto* scrollArea = new QScrollArea(this);
	scrollArea->setObjectName("MainScrollArea");
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	QScroller::scroller(scrollArea)->grabGesture(scrollArea, QScroller::LeftMouseButtonGesture);
	scrollArea->setFrameShape(QFrame::NoFrame);

	auto* scrollContainer = new QWidget(scrollArea);
	scrollContainer->setObjectName("ScrollContainer");

	auto* contentLayout = new QGridLayout(scrollContainer);
	contentLayout->setContentsMargins(0, 0, 16, 0);
	contentLayout->setSpacing(24);
	contentLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);


	// --- 1. 实时波形卡片 (EEG / Resp / ECG / SpO2) ---
	auto* frameWaveform = new QFrame(scrollContainer);
	frameWaveform->setObjectName("CardFrame");
	frameWaveform->setMinimumHeight(400);
	enableHover(frameWaveform);

	auto* vLayoutW = new QVBoxLayout(frameWaveform);
	vLayoutW->setContentsMargins(20, 16, 20, 16);
	vLayoutW->setSpacing(12);

	auto* titleW = new QLabel("实时多导生理信号集群监测", frameWaveform);
	titleW->setObjectName("CardTitle");
	auto* descW = new QLabel("包含脑电(EEG)、呼吸流(Flow)、心电(ECG)和血氧(SpO2)的同步多维波形追踪", frameWaveform);
	descW->setObjectName("CardDesc");

	waveformMonitor_ = new HistoryWaveformWidget(frameWaveform);
	waveformMonitor_->setMinimumHeight(300);
	waveformMonitor_->setAllowZoom(false);
	waveformMonitor_->setAllowDrag(false);

	vLayoutW->addWidget(titleW);
	vLayoutW->addWidget(descW);
	vLayoutW->addWidget(waveformMonitor_);

	// --- 2. 饼图卡片 ---
	auto* framePie = new QFrame(scrollContainer);
	framePie->setObjectName("CardFrame");
	framePie->setMinimumHeight(400);
	enableHover(framePie);

	auto* vLayoutP = new QVBoxLayout(framePie);
	vLayoutP->setContentsMargins(20, 16, 20, 16);
	vLayoutP->setSpacing(12);

	auto* titleP = new QLabel("睡眠结构占比 (%)", framePie);
	titleP->setObjectName("CardTitle");
	auto* descP = new QLabel("全晚各睡眠阶段所占比例的宏观结构 analysis", framePie);
	descP->setObjectName("CardDesc");

	stagePieChart_ = new PieChartWidget(framePie);
	stagePieChart_->setMinimumHeight(300);

	vLayoutP->addWidget(titleP);
	vLayoutP->addWidget(descP);
	vLayoutP->addWidget(stagePieChart_);

	contentLayout->addWidget(frameWaveform, 0, 0);
	contentLayout->addWidget(framePie, 0, 1);

	// --- 3. 睡眠分期趋势卡片 ---
	auto* frameStage = new QFrame(scrollContainer);
	frameStage->setObjectName("CardFrame");
	frameStage->setMinimumHeight(320);
	enableHover(frameStage);

	auto* vLayoutS = new QVBoxLayout(frameStage);
	vLayoutS->setContentsMargins(20, 16, 20, 16);
	vLayoutS->setSpacing(12);

	auto* titleS = new QLabel("整晚睡眠分期趋势 (Hypnogram)", frameStage);
	titleS->setObjectName("CardTitle");

	hypnogramChart_ = new StagingWidget(frameStage);
	hypnogramChart_->setMinimumHeight(220);

	vLayoutS->addWidget(titleS);
	vLayoutS->addWidget(hypnogramChart_);

	// --- 4. 体位卡片 ---
	auto* framePos = new QFrame(scrollContainer);
	framePos->setObjectName("CardFrame");
	framePos->setMinimumHeight(320);
	enableHover(framePos);

	auto* vLayoutPos = new QVBoxLayout(framePos);
	vLayoutPos->setContentsMargins(20, 16, 20, 16);
	vLayoutPos->setSpacing(12);

	auto* titlePos = new QLabel("睡眠体位分布时长 (分钟)", framePos);
	titlePos->setObjectName("CardTitle");

	positionBarChart_ = new BarChartWidget(framePos);
	positionBarChart_->setMinimumHeight(220);

	vLayoutPos->addWidget(titlePos);
	vLayoutPos->addWidget(positionBarChart_);

	contentLayout->addWidget(frameStage, 1, 0);
	contentLayout->addWidget(framePos, 1, 1);

	// --- 5. 呼吸暂停时序事件卡片 ---
	auto* frameEvent = new QFrame(scrollContainer);
	frameEvent->setObjectName("CardFrame");
	frameEvent->setMinimumHeight(280);
	enableHover(frameEvent);

	auto* vLayoutE = new QVBoxLayout(frameEvent);
	vLayoutE->setContentsMargins(20, 16, 20, 16);
	vLayoutE->setSpacing(12);

	auto* titleE = new QLabel("呼吸暂停与低通气时序事件追踪", frameEvent);
	titleE->setObjectName("CardTitle");

	respiratoryEvents_ = new EventWidget(frameEvent);
	respiratoryEvents_->setMinimumHeight(180);

	vLayoutE->addWidget(titleE);
	vLayoutE->addWidget(respiratoryEvents_);

	// --- 6. 事件计数卡片 ---
	auto* frameApnea = new QFrame(scrollContainer);
	frameApnea->setObjectName("CardFrame");
	frameApnea->setMinimumHeight(280);
	enableHover(frameApnea);

	auto* vLayoutAp = new QVBoxLayout(frameApnea);
	vLayoutAp->setContentsMargins(20, 16, 20, 16);
	vLayoutAp->setSpacing(12);

	auto* titleAp = new QLabel("呼吸紊乱事件计数 (次数)", frameApnea);
	titleAp->setObjectName("CardTitle");

	apneaBarChart_ = new BarChartWidget(frameApnea);
	apneaBarChart_->setMinimumHeight(180);

	vLayoutAp->addWidget(titleAp);
	vLayoutAp->addWidget(apneaBarChart_);

	contentLayout->addWidget(frameEvent, 2, 0);
	contentLayout->addWidget(frameApnea, 2, 1);

	contentLayout->setColumnStretch(0, 7);
	contentLayout->setColumnStretch(1, 3);

	scrollArea->setWidget(scrollContainer);
	mainVerticalLayout->addWidget(scrollArea);
}

void PSGMainPage::generateMockData() {
	auto* session = UiSession::instance();
	auto* rand = QRandomGenerator::global();

	auto area = this->findChild<QScrollArea*>(lstr("MainScrollArea"), Qt::FindChildOption::FindDirectChildrenOnly);
	
	// 1. 深度联动：获取状态核心五元色，用于渲染 5 类睡眠结构分期
	QString surface = session->theme().value(UiField::Surface).toString();
	area->setStyleSheet(lstr("background-color: %1").arg(surface));

	QColor cWake = session ? QColor(session->theme().value("danger").toString()) : QColor("#EF4444");
	QColor cREM = session ? QColor(session->theme().value("primary").toString()) : QColor("#3B82F6");
	QColor cN1 = session ? QColor(session->theme().value("warning").toString()) : QColor("#F59E0B");
	QColor cN2 = session ? QColor(session->theme().value("success").toString()) : QColor("#10B981");
	QColor cN3 = session ? QColor(session->theme().value("accent").toString()) : QColor("#8B5CF6");

	const QList<QColor> functionalPalette = { cWake, cREM, cN1, cN2, cN3 };

	QList<StageData> stageTimeline;
	stageTimeline.reserve(15);
	qreal currentTime = 0;
	for (int i = 0; i < 15; ++i) {
		StageData data;
		data.stageIndex = rand->bounded(0, 5);
		data.startDuration = currentTime;
		data.persistDuration = rand->bounded(20, 60);
		currentTime += data.persistDuration;
		stageTimeline.append(data);
	}

	hypnogramChart_->setData(stageTimeline);
	hypnogramChart_->setYLabel({ "Wake", "REM", "N1", "N2", "N3" });
	hypnogramChart_->setBarColors(functionalPalette);
	hypnogramChart_->setBarClickable(false);

	// 2. 深度联动：获取专属的医疗报警色（SignalAlarm）与次要状态色（Secondary）渲染呼吸事件
	QColor alarmColor = session ? QColor(session->theme().value("signalAlarm").toString()) : QColor("#DC2626");
	QColor eventSecondary = session ? QColor(session->theme().value("secondary").toString()) : QColor("#2DD4BF");
	if (!alarmColor.isValid()) alarmColor = QColor("#DC2626");
	if (!eventSecondary.isValid()) eventSecondary = QColor("#2DD4BF");

	QList<QList<StageData>> multiChannelEvents;

	// 阻塞性暂停
	QList<StageData> obstructiveEvents;
	for (int i = 0; i < 5; ++i) {
		qreal randomDuration = rand->bounded(8, 35);
		obstructiveEvents.append({ 0, static_cast<qreal>(i * 80 + 30), randomDuration });
	}

	// 中枢性暂停
	QList<StageData> centralEvents;
	for (int i = 0; i < 3; ++i) {
		qreal randomDuration = rand->bounded(5, 25);
		centralEvents.append({ 1, static_cast<qreal>(i * 120 + 50), randomDuration });
	}

	multiChannelEvents.append(obstructiveEvents);
	multiChannelEvents.append(centralEvents);

	respiratoryEvents_->setYLabels({ "阻塞性暂停", "中枢性暂停" });
	respiratoryEvents_->setValues(multiChannelEvents);
	respiratoryEvents_->setMaxDuration(currentTime);

	// 睡眠结构占比饼图绑定动态色彩
	QList<qreal> stagePercentages = { 10.0, 20.0, 15.0, 40.0, 15.0 };
	stagePieChart_->setData(stagePercentages, functionalPalette);
	stagePieChart_->setHoleRadius(0.72);
	stagePieChart_->setEnableSelected(false);

	// 3. 深度联动：使用系统专门定义的体位颜色键值（SignalBody）
	QColor bodyColor = session ? QColor(session->theme().value("signalBody").toString()) : QColor("#F59E0B");
	if (!bodyColor.isValid()) bodyColor = QColor("#F59E0B");

	QList<QList<qreal>> positionValues = { {180.0, 120.0, 90.0, 30.0} };
	QList<QList<QString>> positionLabels = { {"仰卧", "左侧卧", "右侧卧", "俯卧"} };
	positionBarChart_->setAllowSelect(true);
	positionBarChart_->setYBounds(0, 240);
	positionBarChart_->setSeriesColors({ bodyColor, bodyColor, bodyColor, bodyColor });
	positionBarChart_->setData(positionValues, positionLabels);

	// 4. 深度联动：呼吸紊乱事件计数（OAI, CAI, MAI）分别关联 危险/警告/次要 状态色彩
	QColor warningColor = session ? QColor(session->theme().value("warning").toString()) : QColor("#FBBF24");
	if (!warningColor.isValid()) warningColor = QColor("#FBBF24");

	QList<QList<qreal>> apneaCounts = { {24.0, 12.0, 6.0} };
	QList<QList<QString>> apneaLabels = { {"OAI", "CAI", "MAI"} };
	apneaBarChart_->setYBounds(0, 40);
	apneaBarChart_->setSeriesColors({ alarmColor, warningColor, eventSecondary });
	apneaBarChart_->setData(apneaCounts, apneaLabels);
}