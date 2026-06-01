#include <ctime>
#include <QScrollArea>
#include <QMenu>
#include <QWidgetAction>
#include <QTimer>
#include <QScroller>
#include <QDataStream>
#include <QDateTime>
#include <QMessageBox>

#include "Core/Utils.hpp"
#include "Driver/UiTest/Driver.hpp"
#include "Backend/Ui/Session.hpp"
#include "Backend/Network/Session.hpp"

#include "CardUiTestDetails.hpp"

// 为自定义的 StageData 结构提供数据流序列化操作符
static QDataStream& operator<<(QDataStream& out, const StageData& data) {
	out << data.stageIndex << data.startDuration << data.persistDuration;
	return out;
}
static QDataStream& operator>>(QDataStream& in, StageData& data) {
	in >> data.stageIndex >> data.startDuration >> data.persistDuration;
	return in;
}

static QColor getColorFrom(const QVariantMap& theme, const QString& key, const QColor& fallback = Qt::white) {
	if (!theme.contains(key)) return fallback;
	QVariant var = theme.value(key);
	if (var.canConvert<QColor>()) return var.value<QColor>();
	if (var.canConvert<QString>()) return QColor(var.toString());
	return fallback;
}


SleepBaseCard::SleepBaseCard(const QString& title, QWidget* parent)
	: QWidget(parent) {
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(12, 12, 12, 12);
	mainLayout->setSpacing(8);

	auto* headerLayout = new QHBoxLayout();

	enableBtn_ = new QPushButton(this);
	enableBtn_->setObjectName("enableBtn");
	enableBtn_->setCheckable(true);
	enableBtn_->setChecked(true);
	enableBtn_->setCursor(Qt::PointingHandCursor);
	enableBtn_->setFixedSize(16, 16);

	titleLabel_ = new QLabel(title, this);

	toggleBtn_ = new QPushButton(this);
	toggleBtn_->setCheckable(true);
	toggleBtn_->setCursor(Qt::PointingHandCursor);
	toggleBtn_->setFlat(true);
	toggleBtn_->hide();
	headerLayout->addWidget(enableBtn_);
	headerLayout->addWidget(titleLabel_);
	headerLayout->addStretch();
	headerLayout->addWidget(toggleBtn_);

	stackedWidget_ = new QStackedWidget(this);

	auto* placeholderLabel = new QLabel(lstr("暂无测试数据，请激活状态开始测量"), this);
	placeholderLabel->setObjectName("placeholderLabel");
	placeholderLabel->setAlignment(Qt::AlignCenter);
	stackedWidget_->addWidget(placeholderLabel);

	mainLayout->addLayout(headerLayout);
	mainLayout->addWidget(stackedWidget_, 1);

	connect(toggleBtn_, &QPushButton::clicked, this, &SleepBaseCard::onToggleViewClicked);

	if (UiSession::instance()) {
		connect(UiSession::instance(), &UiSession::themeChanged, this, &SleepBaseCard::onThemeChanged);
		connect(UiSession::instance(), &UiSession::fontChanged, this, &SleepBaseCard::onThemeChanged);
	}
}

bool SleepBaseCard::checked() {
	return this->enableBtn_->isChecked();
}

int SleepBaseCard::addView(QWidget* widget) {
	return stackedWidget_->addWidget(widget);
}

void SleepBaseCard::onToggleViewClicked() {
	int count = stackedWidget_->count();
	if (count < 3) return;
	int currentIdx = stackedWidget_->currentIndex();
	int nextIdx = (currentIdx == 1) ? 2 : 1;
	stackedWidget_->setCurrentIndex(nextIdx);
}

void SleepBaseCard::setViewState(bool measuring, bool hasData) {
	if (measuring) {
		toggleBtn_->hide();
		stackedWidget_->setCurrentIndex(1);
	}
	else {
		if (!hasData) {
			toggleBtn_->hide();
			stackedWidget_->setCurrentIndex(0);
		}
		else {
			toggleBtn_->show();
			stackedWidget_->setCurrentIndex(2);
		}
	}
}

void SleepBaseCard::onThemeChanged() {
	if (!UiSession::instance()) return;
	QVariantMap theme = UiSession::instance()->theme();
	QVariantMap font = UiSession::instance()->font();

	QColor surface = getColorFrom(theme, UiField::Surface, QColor(30, 30, 30));
	QColor border = getColorFrom(theme, UiField::Border, QColor(50, 50, 50));
	QColor textPrimary = getColorFrom(theme, UiField::TextPrimary, Qt::white);
	QColor textSecondary = getColorFrom(theme, UiField::TextSecondary, Qt::gray);
	QColor primary = getColorFrom(theme, UiField::Primary, QColor(0, 120, 215));

	QString fontFamily = font.value(UiField::Family, "Segoe UI").toString();
	int bodySize = font.value(UiField::BodySize, 14).toInt();
	int subtitleSize = font.value(UiField::SubtitleSize, 16).toInt();
	int smallSize = font.value(UiField::SizeSmall, 12).toInt();

	this->setStyleSheet(lstr(
		"SleepBaseCard { "
		"  background-color: %1; "
		"  border: 1px solid %2; "
		"  border-radius: 12px; "
		"  font-family: '%3'; "
		"}"
	).arg(surface.name(), border.name(), fontFamily));

	titleLabel_->setStyleSheet(lstr(
		"QLabel { color: %1; font-weight: bold; font-size: %2px; font-family: '%3'; }"
	).arg(textPrimary.name()).arg(subtitleSize).arg(fontFamily));

	if (auto* placeholder = findChild<QLabel*>("placeholderLabel")) {
		placeholder->setStyleSheet(lstr(
			"QLabel { color: %1; font-size: %2px; background: transparent; font-family: '%3'; }"
		).arg(textSecondary.name()).arg(bodySize).arg(fontFamily));
	}

	enableBtn_->setStyleSheet(lstr(
		"QPushButton#enableBtn { "
		"  border: 2px solid %1; "
		"  border-radius: 8px; "
		"  background-color: transparent; "
		"} "
		"QPushButton#enableBtn:hover { "
		"  border-color: %2; "
		"} "
		"QPushButton#enableBtn:checked { "
		"  background-color: %2; "
		"  border-color: %2; "
		"}"
	).arg(textSecondary.name(), primary.name()));

	toggleBtn_->setText(lstr("切换视图"));
	toggleBtn_->setStyleSheet(lstr(
		"QPushButton { "
		"  color: %1; "
		"  background-color: transparent; "
		"  border: 1px solid %2; "
		"  border-radius: 4px; "
		"  padding: 4px 8px; "
		"  font-size: %3px; "
		"  font-family: '%4'; "
		"}"
		"QPushButton:hover { "
		"  background-color: %5; "
		"  color: %6; "
		"}"
	).arg(textSecondary.name(), border.name()).arg(smallSize).arg(fontFamily, primary.name(), textPrimary.name()));

	updateThemeStyles(theme, font);
}





SleepStagingCard::SleepStagingCard(QWidget* parent) : SleepBaseCard(lstr("睡眠分期"), parent) {
	this->addView(stagingWidget_ = new StagingWidget(this));
	this->addView(pieChartWidget_ = new PieChartWidget(this));
	this->onThemeChanged();
}

void SleepStagingCard::setData(const QList<StageData>& stageData, const QList<qreal>& piePercentages) {
	stagingWidget_->setData(stageData);
	if (!piePercentages.isEmpty()) {
		pieChartWidget_->setData(piePercentages);
	}
}

void SleepStagingCard::updateThemeStyles(const QVariantMap& theme, const QVariantMap&) {
	QColor grid = getColorFrom(theme, UiField::Grid, QColor(60, 60, 60));
	QColor textSec = getColorFrom(theme, UiField::TextSecondary, Qt::gray);
	QColor eegColor = getColorFrom(theme, UiField::SignalEEG, QColor(0, 200, 100));
	QColor primary = getColorFrom(theme, UiField::Primary, QColor(40, 120, 230));
	QColor accent = getColorFrom(theme, UiField::Accent, QColor(200, 50, 200));

	stagingWidget_->setAxisColor(grid);
	stagingWidget_->setXLabelColor(textSec);
	stagingWidget_->setYLabelColor(textSec);
	stagingWidget_->setBarColors({ eegColor, primary, accent });
	pieChartWidget_->update();
}





RespiratoryEventsCard::RespiratoryEventsCard(QWidget* parent)
	: SleepBaseCard(lstr("呼吸事件"), parent) {
	this->addView(eventWidget_ = new EventWidget(this));
	this->addView(barChartWidget_ = new BarChartWidget(this));

	eventWidget_->setYLabels({ lstr("呼吸暂停"), lstr("阻塞性呼吸暂停"), lstr("中枢性呼吸暂停") });
	eventWidget_->setLabelsEnabled(true, true);
	eventWidget_->setAxesEnabled(true, true);
	eventWidget_->setBarWidth(20);
	eventWidget_->setMaxDuration(40.0);

	this->onThemeChanged();
}

void RespiratoryEventsCard::setData(const QList<QList<StageData>>& eventData,
	const QList<QList<qreal>>& barValues,
	const QList<QList<QString>>& barLabels) {
	eventWidget_->setValues(eventData);
	barChartWidget_->setData(barValues, barLabels);
}

void RespiratoryEventsCard::updateThemeStyles(const QVariantMap& theme, const QVariantMap&) {
	QColor respColor = getColorFrom(theme, UiField::SignalResp, QColor(0, 180, 255));
	QColor warning = getColorFrom(theme, UiField::Warning, QColor(255, 180, 0));
	QColor danger = getColorFrom(theme, UiField::Danger, QColor(255, 60, 60));

	QList<QColor> eventColors = { respColor, warning, danger };
	barChartWidget_->setSeriesColors(eventColors);
	barChartWidget_->setEnableBackground(false);
}

SpO2TrendCard::SpO2TrendCard(QWidget* parent) : SleepBaseCard(lstr("血氧趋势"), parent) {
	this->addView(waveformWidget_ = new HistoryWaveformWidget(this));
	this->addView(pieChartWidget_ = new PieChartWidget(this));
	this->onThemeChanged();
}



void SpO2TrendCard::setData(const QList<QPointF>& trendPoints, const QList<qreal>& distributionPercentages) {
	auto ui = UiSession::instance()->theme();
	waveformWidget_->clearCurves();
	waveformWidget_->addCurve(trendPoints, ui.value(UiField::SignalSpO2).toString(), 2.0);
	if (!distributionPercentages.isEmpty()) {
		pieChartWidget_->setData(distributionPercentages);
	}
}



void SpO2TrendCard::updateThemeStyles(const QVariantMap& theme, const QVariantMap&) {
	QColor spo2Color = getColorFrom(theme, UiField::SignalSpO2, QColor(0, 230, 230));
	QColor danger = getColorFrom(theme, UiField::Danger, QColor(240, 50, 50));
	QColor warning = getColorFrom(theme, UiField::Warning, QColor(240, 150, 10));

	if (waveformWidget_->curveCount() > 0) {
		waveformWidget_->setCurveColor(0, spo2Color);
	}
	pieChartWidget_->setEnableBackground(false);
	pieChartWidget_->setData(pieChartWidget_->percentages(), { danger, spo2Color, warning });
}

BodyPositionCard::BodyPositionCard(QWidget* parent) : SleepBaseCard(lstr("体位状态"), parent) {
	this->addView(stagingWidget_ = new StagingWidget(this));
	this->addView(pieChartWidget_ = new PieChartWidget(this));
	this->onThemeChanged();
}

void BodyPositionCard::setData(const QList<StageData>& positionData, const QList<qreal>& piePercentages) {
	stagingWidget_->setData(positionData);
	if (!piePercentages.isEmpty()) {
		pieChartWidget_->setData(piePercentages);
	}
}

void BodyPositionCard::updateThemeStyles(const QVariantMap& theme, const QVariantMap&) {
	QColor grid = getColorFrom(theme, UiField::Grid, QColor(60, 60, 60));
	QColor textSec = getColorFrom(theme, UiField::TextSecondary, Qt::gray);
	QColor c1 = getColorFrom(theme, UiField::Primary, QColor(30, 144, 255));
	QColor c2 = getColorFrom(theme, UiField::Secondary, QColor(138, 43, 226));
	QColor c3 = getColorFrom(theme, UiField::Success, QColor(46, 139, 87));
	QColor c4 = getColorFrom(theme, UiField::Accent, QColor(218, 112, 214));
	QList<QColor> positionColors = { c1, c2, c3, c4 };

	stagingWidget_->setAxisColor(grid);
	stagingWidget_->setXLabelColor(textSec);
	stagingWidget_->setYLabelColor(textSec);
	stagingWidget_->setBarColors(positionColors);
	stagingWidget_->setYLabel({ lstr("左侧"), lstr("俯卧"), lstr("右侧"), lstr("仰卧") });
	pieChartWidget_->setEnableBackground(false);
}





SleepDashboardWidget::SleepDashboardWidget(QWidget* parent) : QWidget(parent) {
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(12, 12, 12, 12);
	mainLayout->setSpacing(14);

	auto* controlBar = new QWidget(this);
	controlBar->setObjectName("controlBar");

	auto* controlLayout = new QHBoxLayout(controlBar);
	controlLayout->setContentsMargins(16, 10, 16, 10);
	controlLayout->setSpacing(16);

	auto* enabledLabel = new QLabel(lstr("激活状态:"), this);
	switchEnabledBtn_ = new QPushButton(this);
	switchEnabledBtn_->setCheckable(true);
	switchEnabledBtn_->setCursor(Qt::PointingHandCursor);
	switchEnabledBtn_->setFixedSize(64, 24);
	switchEnabledBtn_->setText(lstr("OFF"));

	auto* settingsBtn = new QPushButton(lstr("设置"), this);
	settingsBtn->setObjectName("settingsBtn");
	settingsBtn->setCursor(Qt::PointingHandCursor);

	auto* settingsMenu = new QMenu(this);
	settingsMenu->setObjectName("settingsMenu");

	auto* menuContainer = new QWidget(settingsMenu);
	auto* menuLayout = new QHBoxLayout(menuContainer);
	menuLayout->setContentsMargins(10, 6, 10, 6);
	menuLayout->setSpacing(8);

	auto* bytesLabel = new QLabel(lstr("测试数据大小 (Bytes):"), menuContainer);
	bytesSpinBox_ = new QSpinBox(menuContainer);
	bytesSpinBox_->setRange(0, 1024 * 1024);
	bytesSpinBox_->setValue(0);

	menuLayout->addWidget(bytesLabel);
	menuLayout->addWidget(bytesSpinBox_);

	auto* widgetAction = new QWidgetAction(settingsMenu);
	widgetAction->setDefaultWidget(menuContainer);
	settingsMenu->addAction(widgetAction);
	settingsBtn->setMenu(settingsMenu);

	controlLayout->addWidget(enabledLabel);
	controlLayout->addWidget(switchEnabledBtn_);
	controlLayout->addSpacing(8);
	controlLayout->addWidget(settingsBtn);
	controlLayout->addStretch();

	mainLayout->addWidget(controlBar);

	auto* scrollArea = new QScrollArea(this);
	scrollArea->setObjectName("cardScrollArea");
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea->setFrameShape(QFrame::NoFrame);

	auto* scrollContainer = new QWidget(scrollArea);
	scrollContainer->setObjectName("scrollContainer");

	QScroller::grabGesture(scrollArea, QScroller::LeftMouseButtonGesture);
	QScrollerProperties properties = QScroller::scroller(scrollArea)->scrollerProperties();
	// properties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, QScrollerProperties::OvershootAlwaysOn);
	// properties.setScrollMetric(QScrollerProperties::Inver, true);
	// QScroller::scroller(scrollArea)->setScrollerProperties(properties);

	auto* hLayout = new QHBoxLayout(scrollContainer);
	hLayout->setContentsMargins(0, 0, 0, 0);
	hLayout->setSpacing(16);

	stagingCard_ = new SleepStagingCard(scrollContainer);
	respiratoryCard_ = new RespiratoryEventsCard(scrollContainer);
	spo2Card_ = new SpO2TrendCard(scrollContainer);
	bodyPositionCard_ = new BodyPositionCard(scrollContainer);

	stagingCard_->setMinimumWidth(380);
	respiratoryCard_->setMinimumWidth(380);
	spo2Card_->setMinimumWidth(380);
	bodyPositionCard_->setMinimumWidth(380);

	hLayout->addWidget(stagingCard_, 1);
	hLayout->addWidget(respiratoryCard_, 1);
	hLayout->addWidget(spo2Card_, 1);
	hLayout->addWidget(bodyPositionCard_, 1);

	scrollArea->setWidget(scrollContainer);
	mainLayout->addWidget(scrollArea, 1);

	connect(switchEnabledBtn_, &QPushButton::toggled, this, &SleepDashboardWidget::onEnabledToggled);
	connect(bytesSpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, &SleepDashboardWidget::onBytesChanged);

	pollTimer_ = new QTimer(this);
	connect(pollTimer_, &QTimer::timeout, this, &SleepDashboardWidget::pollDeviceData);

	if (auto* driver = UITestModule::instance()) {
		connect(driver, &UITestModule::readResponded, this, &SleepDashboardWidget::onReadResponded);
		connect(driver, &UITestModule::errorOccured, this, [this](QString id, QString message) {
			qCritical() << lstr("[UiTest ToolSleep ERROR] Device Error [ID: %1]: %2").arg(id, message);

			QMessageBox::critical(this,
				lstr("设备错误"),
				lstr("测量过程中发生设备异常！\n\n设备ID: %1\n错误信息: %2").arg(id, message)
			);

			if (switchEnabledBtn_ && switchEnabledBtn_->isChecked()) {
				switchEnabledBtn_->setChecked(false);
			}
		});
	}

	if (UiSession::instance()) {
		connect(UiSession::instance(), &UiSession::themeChanged, this, &SleepDashboardWidget::onThemeChanged);
		connect(UiSession::instance(), &UiSession::fontChanged, this, &SleepDashboardWidget::onThemeChanged);
	}

	this->onThemeChanged();

	stagingCard_->setViewState(false, false);
	respiratoryCard_->setViewState(false, false);
	spo2Card_->setViewState(false, false);
	bodyPositionCard_->setViewState(false, false);

}

void SleepDashboardWidget::onThemeChanged() {
	if (!UiSession::instance()) return;

	QVariantMap theme = UiSession::instance()->theme();
	QVariantMap font = UiSession::instance()->font();

	QColor bg = getColorFrom(theme, UiField::Background, QColor(15, 15, 15));
	QColor surface = getColorFrom(theme, UiField::Surface, QColor(30, 30, 30));
	QColor border = getColorFrom(theme, UiField::Border, QColor(50, 50, 50));
	QColor textPrimary = getColorFrom(theme, UiField::TextPrimary, Qt::white);
	QColor textSecondary = getColorFrom(theme, UiField::TextSecondary, Qt::gray);
	QColor primary = getColorFrom(theme, UiField::Primary, QColor(0, 120, 215));
	QColor accent = getColorFrom(theme, UiField::Accent, QColor(200, 50, 200));

	QString fontFamily = font.value(UiField::Family, "Segoe UI").toString();
	int normalSize = font.value(UiField::SizeNormal, 14).toInt();
	int smallSize = font.value(UiField::SizeSmall, 12).toInt();
	int tinySize = font.value(UiField::SizeTiny, 10).toInt();
	int weightMedium = font.value(UiField::SubtitleWeight, 500).toInt();

	this->setStyleSheet(lstr("SleepDashboardWidget { background-color: %1; font-family: '%2'; }").arg(bg.name(), fontFamily));

	if (QScrollArea* scrollArea = findChild<QScrollArea*>("cardScrollArea")) {
		scrollArea->setStyleSheet(lstr(
			"QScrollArea { background: transparent; border: none; }"
			"QWidget#scrollContainer { background: transparent; }"
			"QScrollBar:horizontal { height: 6px; background: transparent; margin: 0px; }"
			"QScrollBar::handle:horizontal { background: %1; border-radius: 3px; min-width: 40px; }"
			"QScrollBar::handle:horizontal:hover { background: %2; }"
			"QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; background: transparent; }"
		).arg(border.name(), primary.name()));
	}

	if (QWidget* controlBar = findChild<QWidget*>("controlBar")) {
		controlBar->setStyleSheet(lstr(
			"QWidget { background-color: %1; border: 1px solid %2; border-radius: 8px; }"
			"QLabel { color: %3; font-size: %4px; font-weight: %5; border: none; font-family: '%6'; }"
		).arg(surface.name(), border.name(), textPrimary.name()).arg(smallSize).arg(weightMedium).arg(fontFamily));
	}

	if (switchEnabledBtn_) {
		switchEnabledBtn_->setStyleSheet(lstr(
			"QPushButton { "
			"  background-color: %1; color: %2; border: 1px solid %1; border-radius: 12px; "
			"  font-size: %3px; font-weight: bold; text-align: center; padding: 0px; font-family: '%4'; "
			"} "
			"QPushButton:hover { "
			"  border-color: %5; color: %6; "
			"} "
			"QPushButton:checked { "
			"  background-color: %5; color: %6; border-color: %5; "
			"}"
		).arg(border.name(), textSecondary.name()).arg(tinySize).arg(fontFamily, primary.name(), textPrimary.name()));
	}

	if (QPushButton* settingsBtn = findChild<QPushButton*>("settingsBtn")) {
		settingsBtn->setStyleSheet(lstr(
			"QPushButton { "
			"  background-color: transparent; color: %1; border: 1px solid %2; border-radius: 4px; "
			"  padding: 4px 18px 4px 10px; font-size: %3px; font-family: '%4'; "
			"} "
			"QPushButton:hover { background-color: %5; color: %6; } "
			"QPushButton::menu-indicator { subcontrol-origin: padding; subcontrol-position: center right; right: 4px; }"
		).arg(textPrimary.name(), border.name()).arg(smallSize).arg(fontFamily, primary.name(), textPrimary.name()));

		if (QMenu* menu = settingsBtn->menu()) {
			menu->setStyleSheet(lstr(
				"QMenu { background-color: %1; border: 1px solid %2; border-radius: 6px; padding: 2px; }"
				"QLabel { color: %3; font-size: %4px; border: none; background: transparent; font-family: '%5'; }"
			).arg(surface.name(), border.name(), textPrimary.name()).arg(smallSize).arg(fontFamily));
		}
	}

	if (bytesSpinBox_) {
		bytesSpinBox_->setStyleSheet(lstr(
			"QSpinBox { "
			"  background-color: %1; "
			"  color: %2; "
			"  border: 1px solid %3; "
			"  border-radius: 6px; "
			"  padding: 4px 24px 4px 8px; "
			"  min-width: 90px; "
			"  font-size: %4px; "
			"  font-family: '%5'; "
			"} "
			"QSpinBox:hover { "
			"  border: 1px solid %6; "
			"} "
			"QSpinBox:focus { "
			"  border: 1px solid %6; "
			"  background-color: %7; "
			"} "
			"QSpinBox::up-button { "
			"  subcontrol-origin: border; "
			"  subcontrol-position: top right; "
			"  width: 18px; "
			"  height: 12px; "
			"  border-left: 1px solid %3; "
			"  border-bottom: 1px solid %3; "
			"  background: %7; "
			"  border-top-right-radius: 5px; "
			"} "
			"QSpinBox::up-button:hover { background: %3; } "
			"QSpinBox::up-button:pressed { background: %6; } "
			"QSpinBox::up-arrow { "
			"  border-left: 3px solid transparent; "
			"  border-right: 3px solid transparent; "
			"  border-bottom: 4px solid %8; "
			"  width: 0; height: 0; "
			"} "
			"QSpinBox::down-button { "
			"  subcontrol-origin: border; "
			"  subcontrol-position: bottom right; "
			"  width: 18px; "
			"  height: 11px; "
			"  border-left: 1px solid %3; "
			"  background: %7; "
			"  border-bottom-right-radius: 5px; "
			"} "
			"QSpinBox::down-button:hover { background: %3; } "
			"QSpinBox::down-button:pressed { background: %6; } "
			"QSpinBox::down-arrow { "
			"  border-left: 3px solid transparent; "
			"  border-right: 3px solid transparent; "
			"  border-top: 4px solid %8; "
			"  width: 0; height: 0; "
			"}"
		).arg(bg.name(), textPrimary.name(), border.name()).arg(smallSize).arg(fontFamily, primary.name(), surface.name(), accent.name()));
	}
}

void SleepDashboardWidget::onBytesChanged(int value) {
}

void SleepDashboardWidget::onEnabledToggled(bool checked) {
	switchEnabledBtn_->setText(checked ? lstr("ON") : lstr("OFF"));
	this->setProperty(DeviceField::UiTest::Enabled, checked);

	auto* driver = UITestModule::instance();
	if (!driver) return;

	if (checked) {
		stagingHistory_.clear();
		spo2TrendPoints_.clear();
		positionHistory_.clear();
		apneaHistory_.clear();
		obstructiveHistory_.clear();
		centralHistory_.clear();
		normalCount_ = 0;
		hypopneaCount_ = 0;
		apneaCount_ = 0;
		currentRespTime_ = 0.0;
		timeIndex_ = 0;

		stagingCard_->setViewState(true, false);
		respiratoryCard_->setViewState(true, false);
		spo2Card_->setViewState(true, false);
		bodyPositionCard_->setViewState(true, false);

		lastEnabledChannels_ = 0;
		if (bodyPositionCard_->checked()) lastEnabledChannels_ |= (1 << 0);
		if (spo2Card_->checked())         lastEnabledChannels_ |= (1 << 1);
		if (stagingCard_->checked())      lastEnabledChannels_ |= (1 << 2);
		if (respiratoryCard_->checked())  lastEnabledChannels_ |= (1 << 3) | (1 << 4);

		QByteArray cmd;
		cmd.append(static_cast<char>(15));
		cmd.append(lastEnabledChannels_);
		cmd.append(static_cast<char>(0)); // for safety.
		driver->write(deviceId_, DeviceField::UiTest::RequestSample, cmd);

		pollTimer_->start(250);
	} else {
		pollTimer_->stop();

		QByteArray cmd;
		cmd.append(static_cast<char>(0));
		cmd.append(static_cast<char>(0));
		cmd.append(static_cast<char>(0));
		driver->write(deviceId_, DeviceField::UiTest::RequestSample, cmd);

		bool hasData = !stagingHistory_.isEmpty() || !spo2TrendPoints_.isEmpty() || !positionHistory_.isEmpty() || !apneaHistory_.isEmpty();
		if (hasData) {
			// === 1. 睡眠分期图表结算统计 ===
			qreal c0 = 0, c1 = 0, c2 = 0;
			for (const auto& item : stagingHistory_) {
				if (item.stageIndex == 0) c0++;
				else if (item.stageIndex == 1) c1++;
				else if (item.stageIndex == 2) c2++;
			}
			qreal totalStaging = stagingHistory_.size();
			QList<qreal> stagingPercentages = { 0.0, 0.0, 0.0 };
			if (totalStaging > 0) {
				stagingPercentages = { (c0 / totalStaging) * 100.0, (c1 / totalStaging) * 100.0, (c2 / totalStaging) * 100.0 };
			}
			stagingCard_->setData(stagingHistory_, stagingPercentages);

			// === 2. 停止后统一进行血氧分布图表结算统计 ===
			qreal normalSpO2 = 0, mildSpO2 = 0, severeSpO2 = 0;
			for (const auto& pt : spo2TrendPoints_) {
				if (pt.y() >= 95.0) normalSpO2++;
				else if (pt.y() >= 90.0) mildSpO2++;
				else severeSpO2++;
			}
			qreal totalSpO2 = spo2TrendPoints_.size();
			QList<qreal> spo2Percentages = { 0.0, 0.0, 0.0 };
			if (totalSpO2 > 0) {
				spo2Percentages = { (severeSpO2 / totalSpO2) * 100.0, (normalSpO2 / totalSpO2) * 100.0, (mildSpO2 / totalSpO2) * 100.0 };
			}
			spo2Card_->setData(spo2TrendPoints_, spo2Percentages);

			// === 3. 停止后统一进行体位分布图表结算统计 ===
			qreal p0 = 0, p1 = 0, p2 = 0, p3 = 0;
			for (const auto& item : positionHistory_) {
				if (item.stageIndex == 0) p0++;
				else if (item.stageIndex == 1) p1++;
				else if (item.stageIndex == 2) p2++;
				else if (item.stageIndex == 3) p3++;
			}
			qreal totalPosition = positionHistory_.size();
			QList<qreal> positionPercentages = { 0.0, 0.0, 0.0, 0.0 };
			if (totalPosition > 0) {
				positionPercentages = { (p0 / totalPosition) * 100.0, (p1 / totalPosition) * 100.0, (p2 / totalPosition) * 100.0, (p3 / totalPosition) * 100.0 };
			}
			bodyPositionCard_->setData(positionHistory_, positionPercentages);

			// === 4. 停止后统一进行呼吸事件全局结算汇总 ===
			qreal windowSize = 40.0;
			qreal minTime = qMax(0.0, currentRespTime_ - windowSize);
			auto getAdjustedViewportHistory = [minTime](const QList<StageData>& srcHistory) {
				QList<StageData> destHistory;
				for (const auto& block : srcHistory) {
					if (block.startDuration + block.persistDuration > minTime) {
						StageData adj = block;
						adj.startDuration = qMax(0.0, block.startDuration - minTime);
						if (block.startDuration < minTime) {
							adj.persistDuration = (block.startDuration + block.persistDuration) - minTime;
						}
						destHistory.append(adj);
					}
				}
				return destHistory;
			};

			QList<QList<StageData>> eventDataList;
			eventDataList.append(getAdjustedViewportHistory(apneaHistory_));
			eventDataList.append(getAdjustedViewportHistory(obstructiveHistory_));
			eventDataList.append(getAdjustedViewportHistory(centralHistory_));

			QList<QList<qreal>> barValues = { { qreal(normalCount_), qreal(hypopneaCount_), qreal(apneaCount_) } };
			QList<QList<QString>> barLabels = { { lstr("正常呼吸"), lstr("低通气"), lstr("呼吸暂停") } };
			respiratoryCard_->setData(eventDataList, barValues, barLabels);

			// 本地持久化归档
			auto time = QDateTime::currentDateTime();
			auto userId = NetworkSession::instance()->userId();
			auto idfile = userId + time.toString("yyyy.MM.dd:hh.mm");
			QByteArray sessionBuffer;
			QDataStream outStream(&sessionBuffer, QIODevice::WriteOnly);

			outStream << stagingHistory_;
			outStream << spo2TrendPoints_;
			outStream << positionHistory_;
			outStream << apneaHistory_;
			outStream << obstructiveHistory_;
			outStream << centralHistory_;
			outStream << normalCount_ << hypopneaCount_ << apneaCount_;
			outStream << currentRespTime_ << timeIndex_;

			QByteArray compressedPayload = qCompress(sessionBuffer);
			driver->writeRecordFile(idfile, idfile + ".z", compressedPayload);
		}

		stagingCard_->setViewState(false, hasData);
		respiratoryCard_->setViewState(false, hasData);
		spo2Card_->setViewState(false, hasData);
		bodyPositionCard_->setViewState(false, hasData);
	}
}

void SleepDashboardWidget::pollDeviceData() {
	auto* driver = UITestModule::instance();
	if (!driver) return;

	quint8 currentMask = 0;
	if (bodyPositionCard_->checked()) currentMask |= (1 << 0);
	if (spo2Card_->checked())         currentMask |= (1 << 1);
	if (stagingCard_->checked())      currentMask |= (1 << 2);
	if (respiratoryCard_->checked())  currentMask |= (1 << 3) | (1 << 4);

	if (currentMask != lastEnabledChannels_) {
		lastEnabledChannels_ = currentMask;
		QByteArray cmd;
		cmd.append(static_cast<char>(15)); // 保持 15Hz 采样率
		cmd.append(static_cast<char>(lastEnabledChannels_));
		cmd.append(static_cast<char>(0));
		driver->write(deviceId_, DeviceField::UiTest::RequestSample, cmd);
	}

	if (bodyPositionCard_->checked()) {
		driver->read(deviceId_, 0x01, {});
	}
	if (spo2Card_->checked()) {
		driver->read(deviceId_, 0x02, {});
	}
	if (stagingCard_->checked()) {
		driver->read(deviceId_, 0x03, {});
	}
	if (respiratoryCard_->checked()) {
		driver->read(deviceId_, 0x04, {});
		driver->read(deviceId_, 0x05, {});
	}
}

void SleepDashboardWidget::onReadResponded(int id, int serialCode, QByteArray arr, QByteArray responseData) {
	if (id != deviceId_) return;

	if (responseData.isEmpty()) {
		int16_t dummyValue = 0;
		switch (serialCode) {
		case 0x01: dummyValue = 350 + (std::rand() % 300); break;
		case 0x02: dummyValue = 120 + (std::rand() % 150); break;
		case 0x03: dummyValue = 90 + (std::rand() % 60);    break;
		case 0x04:
		case 0x05: dummyValue = 900 + (std::rand() % 100); break;
		default:   dummyValue = 0; break;
		}
		responseData.append(reinterpret_cast<const char*>(&dummyValue), sizeof(int16_t));
	}

	const int16_t* samples = reinterpret_cast<const int16_t*>(responseData.constData());
	int sampleCount = responseData.size() / sizeof(int16_t);
	if (sampleCount <= 0) return;

	switch (serialCode) {
	case 0x01:
		processBodyPositionData(samples, sampleCount);
		break;
	case 0x02:
		processSpO2Data(samples, sampleCount);
		break;
	case 0x03:
		processSleepStagingData(samples, sampleCount);
		break;
	case 0x04:
	case 0x05:
		processRespiratoryData(samples, sampleCount, serialCode);
		break;
	default:
		break;
	}
}

void SleepDashboardWidget::processSpO2Data(const int16_t* samples, int count) {
	int16_t lastSample = samples[count - 1];

	qreal spo2 = 88.0 + ((lastSample - 100) / 200.0) * 12.0;
	if (spo2 < 85.0) spo2 = 85.0;
	if (spo2 > 100.0) spo2 = 100.0;

	timeIndex_++;
	spo2TrendPoints_.append(QPointF(timeIndex_, spo2));
	if (spo2TrendPoints_.size() > 50) spo2TrendPoints_.removeFirst();

	spo2Card_->setData(spo2TrendPoints_, { 0.0, 0.0, 0.0 });
}

static void appendEventBlock(QList<StageData>& history, bool isActive, qreal absoluteTime, int stageIdx) {
	if (isActive) {
		if (!history.isEmpty() && (history.last().startDuration + history.last().persistDuration >= absoluteTime - 0.1)) {
			history.last().persistDuration += 1.0;
		}
		else {
			StageData data;
			data.stageIndex = stageIdx;
			data.startDuration = absoluteTime;
			data.persistDuration = 1.0;
			history.append(data);
		}
	}
	while (!history.isEmpty() && (history.first().startDuration + history.first().persistDuration < absoluteTime - 40.0)) {
		history.removeFirst();
	}
}

void SleepDashboardWidget::processRespiratoryData(const int16_t* samples, int count, int code) {
	if (count <= 0) return;
	int16_t lastSample = samples[count - 1];

	bool isApnea = false;
	bool isObstructive = false;
	bool isCentral = false;

	if (code == 0x04 || code == 0x05) {
		currentRespTime_ += 1.0;

		if (lastSample < 980) {
			isApnea = true;
			apneaCount_++;
		}
		if (lastSample < 950) {
			isObstructive = true;
			hypopneaCount_++;
		}
		if (lastSample < 920) {
			isCentral = true;
		}
		if (!isApnea && !isObstructive && !isCentral) {
			normalCount_++;
		}
	}

	if (code == 0x04 || code == 0x05) {
		appendEventBlock(apneaHistory_, isApnea, currentRespTime_, 0);
		appendEventBlock(obstructiveHistory_, isObstructive, currentRespTime_, 1);
		appendEventBlock(centralHistory_, isCentral, currentRespTime_, 2);
	}

	qreal windowSize = 40.0;
	qreal minTime = currentRespTime_ - windowSize;
	if (minTime < 0) minTime = 0;

	auto getAdjustedViewportHistory = [minTime](const QList<StageData>& srcHistory) {
		QList<StageData> destHistory;
		for (const auto& block : srcHistory) {
			if (block.startDuration + block.persistDuration > minTime) {
				StageData adj = block;
				adj.startDuration = qMax(0.0, block.startDuration - minTime);
				if (block.startDuration < minTime) {
					adj.persistDuration = (block.startDuration + block.persistDuration) - minTime;
				}
				destHistory.append(adj);
			}
		}
		return destHistory;
	};

	QList<QList<StageData>> eventDataList;
	eventDataList.append(getAdjustedViewportHistory(apneaHistory_));
	eventDataList.append(getAdjustedViewportHistory(obstructiveHistory_));
	eventDataList.append(getAdjustedViewportHistory(centralHistory_));

	respiratoryCard_->setData(eventDataList, {}, {});
}

void SleepDashboardWidget::processSleepStagingData(const int16_t* samples, int count) {
	if (count <= 0) return;
	int16_t lastSample = samples[count - 1];

	int stageValue = 0;
	if (lastSample > 140) stageValue = 1;
	else if (lastSample < 110) stageValue = 2;

	StageData data;
	data.stageIndex = stageValue;

	stagingHistory_.append(data);
	if (stagingHistory_.size() > 40) stagingHistory_.removeFirst();

	qreal currentStart = 0.0;
	for (int i = 0; i < stagingHistory_.size(); ++i) {
		stagingHistory_[i].startDuration = currentStart;
		stagingHistory_[i].persistDuration = 1.0;
		currentStart += 1.0;
	}

	stagingCard_->setData(stagingHistory_, { 0.0, 0.0, 0.0 });
}

void SleepDashboardWidget::processBodyPositionData(const int16_t* samples, int count) {
	if (count <= 0) return;
	int16_t lastSample = samples[count - 1];

	int positionValue = 0;
	if (lastSample < 412) positionValue = 0;
	else if (lastSample < 512) positionValue = 1;
	else if (lastSample < 612) positionValue = 2;
	else positionValue = 3;

	StageData data;
	data.stageIndex = positionValue;

	positionHistory_.append(data);
	if (positionHistory_.size() > 40) positionHistory_.removeFirst();

	qreal currentStart = 0.0;
	for (int i = 0; i < positionHistory_.size(); ++i) {
		positionHistory_[i].startDuration = currentStart;
		positionHistory_[i].persistDuration = 1.0;
		currentStart += 1.0;
	}

	bodyPositionCard_->setData(positionHistory_, { 0.0, 0.0, 0.0, 0.0 });
}