#include <QLoggingCategory>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScroller>

#include "Core/Utils.hpp"
#include "Backend/Device/Session.hpp"
#include "Session.hpp"
#include "PageDevicePrivate.hpp"

static QString toVerticalText(QString text) {
	QString visualText;
	for (int i = 0; i < text.length(); ++i) {
		visualText.append(text.at(i));
		if (i != text.length() - 1) visualText.append(lstr("\n"));
	}
	return visualText;
}

VerticalTitleBar::VerticalTitleBar(const QString& text, QWidget* parent) : QWidget(parent) {
	auto* layout = new QVBoxLayout(this);
	layout->setAlignment(Qt::AlignCenter);
	layout->setContentsMargins(0, 0, 0, 0);
	label_ = new QLabel(toVerticalText(text), this);
	label_->setAlignment(Qt::AlignCenter);
	this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	layout->addWidget(label_, 0, Qt::AlignCenter);
}

void VerticalTitleBar::setLabelStyle(const QString& style) {
	if (label_) label_->setStyleSheet(style);
}


// ===============================
// DeviceSearchPanel
// ===============================

DeviceSearchPanel::DeviceSearchPanel(QWidget* parent) : Extensible(parent) {
	this->setAttribute(Qt::WA_StyledBackground);
	this->setObjectName("DeviceSearchPanel");

	auto* masterLayout = new QHBoxLayout(this);
	masterLayout->setContentsMargins(0, 0, 0, 0);
	masterLayout->setSpacing(0);

	titleBar_ = new VerticalTitleBar(lstr("连接设备"), this);
	masterLayout->addWidget(titleBar_, Qt::AlignCenter);
	connect(titleBar_, &VerticalTitleBar::clicked, this, &DeviceSearchPanel::titleClicked);

	bleLayout_ = new QVBoxLayout();
	bleLayout_->setContentsMargins(15, 15, 15, 15);
	masterLayout->addLayout(bleLayout_);

	bleLayout_->addStretch();

	bluetoothBtn_ = new QPushButton(lstr("蓝牙"), this);
	bluetoothBtn_->setFixedSize(70, 70);
	bluetoothBtn_->setCursor(Qt::PointingHandCursor);
	bleLayout_->addWidget(bluetoothBtn_, 0, Qt::AlignCenter);
	bleLayout_->addSpacing(15);

	statusText_ = new QLabel(lstr("点击上方蓝牙图标开始搜索设备...\n请确保设备已开启并处于可连接状态"), this);
	statusText_->setAlignment(Qt::AlignCenter);
	bleLayout_->addWidget(statusText_, 0, Qt::AlignCenter);

	listWidget_ = new QListWidget(this);
	listWidget_->setStyleSheet(lstr("background: transparent; border: none;"));
	listWidget_->setSelectionMode(QListWidget::NoSelection);
	listWidget_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	listWidget_->setSpacing(2);
	bleLayout_->addWidget(listWidget_, 1);
	listWidget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	QScroller::grabGesture(listWidget_, QScroller::LeftMouseButtonGesture);

	bleLayout_->addStretch();

	connect(bluetoothBtn_, &QPushButton::clicked, this, &DeviceSearchPanel::onBluetoothIconClicked);

	auto* session = DeviceSession::instance();
	connect(session, &DeviceSession::deviceDiscoverError, this, &DeviceSearchPanel::onDeviceDiscoverError);
	connect(session, &DeviceSession::deviceConnectionError, this, &DeviceSearchPanel::onDeviceConnectionError);

	auto* uiSession = UiSession::instance();
	connect(uiSession, &UiSession::themeChanged, this, &DeviceSearchPanel::updateThemeAndStyles);
	connect(uiSession, &UiSession::fontChanged, this, &DeviceSearchPanel::updateThemeAndStyles);

	this->updateThemeAndStyles();
}

void DeviceSearchPanel::updateDisplay() {
	bool haveList = !widgets_.isEmpty();

	bluetoothBtn_->setVisible(isExpanded_);

	statusText_->setVisible(isExpanded_ && !haveList);
	if (!haveList) {
		if (statusText_->text().contains(lstr("正在搜索"))) {
			statusText_->setText(lstr("正在搜索附近的设备...\n请确保设备已开启并处于可连接状态"));
		}
		else {
			statusText_->setText(lstr("未找到可用设备。\n点击上方蓝牙图标重新搜索"));
		}
	}

	if (listWidget_) {
		listWidget_->setVisible(isExpanded_ && haveList);
	}

	for (int i = bleLayout_->count() - 1; i >= 0; --i) {
		if (bleLayout_->itemAt(i)->spacerItem()) {
			delete bleLayout_->takeAt(i);
		}
	}

	if (isExpanded_) {
		if (!haveList) {
			bleLayout_->insertStretch(0, 1);
			bleLayout_->addStretch(1);
		}
	}
}

qsizetype DeviceSearchPanel::appendWidget(QWidget* w) {
	qsizetype res = Extensible::appendWidget(w);
	if (w && listWidget_) {
		w->setParent(this);
		auto* item = new QListWidgetItem(listWidget_);
		int itemHeight = w->sizeHint().height() > 0 ? w->sizeHint().height() : 60;
		item->setSizeHint(QSize(0, itemHeight));
		listWidget_->addItem(item);
		listWidget_->setItemWidget(item, w);
	}
	updateDisplay();
	return res;
}



QWidget* DeviceSearchPanel::widget(qsizetype id, QWidget* w) {
	QWidget* res = Extensible::widget(id, w);
	if (res && listWidget_ && w) {
		for (int i = 0; i < listWidget_->count(); ++i) {
			QListWidgetItem* item = listWidget_->item(i);
			if (listWidget_->itemWidget(item) == res) {
				w->setParent(this);

				int itemHeight = w->sizeHint().height() > 0 ? w->sizeHint().height() : 60;
				item->setSizeHint(QSize(0, itemHeight));

				listWidget_->setItemWidget(item, w);
				break;
			}
		}
	}

	updateDisplay();
	return res;

}

QWidget* DeviceSearchPanel::removeWidget(qsizetype id) noexcept {
	QWidget* res = Extensible::removeWidget(id);
	if (res && listWidget_) {
		for (int i = 0; i < listWidget_->count(); ++i) {
			QListWidgetItem* item = listWidget_->item(i);
			if (listWidget_->itemWidget(item) == res) {
				delete listWidget_->takeItem(i);
				break;
			}
		}
	}

	updateDisplay();
	return res;
}

void DeviceSearchPanel::setExpanded(bool expanded) {
	isExpanded_ = expanded;
	titleBar_->setVisible(!expanded);
	updateDisplay();
}

void DeviceSearchPanel::triggerSearch() {
	onBluetoothIconClicked();
}

void DeviceSearchPanel::onBluetoothIconClicked() {
	statusText_->setText(lstr("正在搜索附近的设备...\n请确保设备已开启并处于可连接状态"));
	DeviceSession::instance()->searchDevices();
}

void DeviceSearchPanel::onDeviceConnectionError(int id, int errorCode, const QString& msg) {
	if (errorCode == DeviceField::Error_RemoteMissing) {
		qCritical() << lstr("[Device Ui Message] receive remote missing error [%1]: %2").arg(errorCode).arg(msg);
		statusText_->setVisible(true);
		statusText_->setText(lstr("无法连接到目标设备，请检查设备后重试。"));
		QMessageBox::critical(this, lstr("设备连接错误"), lstr("无法连接到目标设备。"));
	}
}

void DeviceSearchPanel::refreshAvailableDevices() {
	updateDisplay();
}

void DeviceSearchPanel::onDeviceDiscoverError(int errorCode, const QString& msg) {
	statusText_->setText(lstr("搜索失败: %1").arg(msg));
	QMessageBox::warning(this, lstr("搜索失败，错误码: %1").arg(errorCode), msg);
}

void DeviceSearchPanel::updateThemeAndStyles() {
	auto theme = UiSession::instance()->theme();
	auto font = UiSession::instance()->font();

	QString bgColor = theme.value(lstr("background"), lstr("#F8F9FA")).toString();
	QString surfaceColor = theme.value(lstr("surface"), lstr("#FFFFFF")).toString();
	QString borderColor = theme.value(lstr("border"), lstr("#D9E2EC")).toString();
	QString primaryColor = theme.value(lstr("primary"), lstr("#3B82F6")).toString();
	QString textSecondary = theme.value(lstr("textSecondary"), lstr("#64748B")).toString();

	QString fontFamily = font.value(lstr("family"), lstr("Microsoft YaHei")).toString();
	int sizeNormal = font.value(lstr("sizeNormal"), 14).toInt();
	int sizeSmall = font.value(lstr("sizeSmall"), 12).toInt();
	int sizeSubtitle = font.value(lstr("sizeLarge"), 16).toInt();

	this->setStyleSheet(lstr("#DeviceSearchPanel { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border-radius: 16px; border: 1px solid %3; }")
		.arg(bgColor, surfaceColor, borderColor));

	titleBar_->setStyleSheet(lstr("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border: none;").arg(bgColor, surfaceColor));
	titleBar_->setLabelStyle(lstr("font-family: '%1'; font-weight: bold; font-size: %2px; color: %3; line-height: 1.2;")
		.arg(fontFamily).arg(sizeSubtitle).arg(textSecondary));

	listWidget_->setStyleSheet("background: transparent; border: none;");

	bluetoothBtn_->setStyleSheet(lstr("font-family: '%1'; background-color: %2; border-radius: 35px; color: white; font-weight: bold; font-size: %3px;")
		.arg(fontFamily).arg(primaryColor).arg(sizeNormal));
	statusText_->setStyleSheet(lstr("font-family: '%1'; color: %2; font-size: %3px; border: none; background: transparent;")
		.arg(fontFamily).arg(textSecondary).arg(sizeSmall));

	this->refreshAvailableDevices();
}

// ====================================
// ConnectedDevicesPanel —— 已连接面板
// ====================================

ConnectedDevicesPanel::ConnectedDevicesPanel(QWidget* parent) : Extensible(parent) {
	this->setAttribute(Qt::WA_StyledBackground);
	this->setObjectName("ConnectedDevicesPanel");

	auto* masterLayout = new QHBoxLayout(this);
	masterLayout->setContentsMargins(0, 0, 0, 0);
	masterLayout->setSpacing(0);

	titleBar_ = new VerticalTitleBar(lstr("已连接设备"), this);
	masterLayout->addWidget(titleBar_, Qt::AlignCenter);
	connect(titleBar_, &VerticalTitleBar::clicked, this, &ConnectedDevicesPanel::titleClicked);

	auto* connLayout = new QVBoxLayout();
	connLayout->setContentsMargins(15, 15, 15, 15);
	masterLayout->addLayout(connLayout);

	// 提示标签放于主布局中（或由 listWidget 显示控制）
	hintLabel_ = new QLabel(lstr("暂无已连接的设备"), this);
	hintLabel_->setAlignment(Qt::AlignCenter);
	connLayout->addWidget(hintLabel_, 1, Qt::AlignCenter);

	// ====== 改用 QListWidget 实现 ======
	listWidget_ = new QListWidget(this);
	listWidget_->setSelectionMode(QListWidget::NoSelection);
	listWidget_->setStyleSheet("border: none; background: transparent;");
	listWidget_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 隐藏原生滚动条，触控更美观
	listWidget_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	listWidget_->setSpacing(6); // 子项间距
	connLayout->addWidget(listWidget_, 1);

	listWidget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	QScroller::grabGesture(listWidget_, QScroller::LeftMouseButtonGesture);

	// 关键点 3：精细调优滚动参数，达到“黄油般顺滑”的触控手感
	//QScroller* scroller = QScroller::scroller(listWidget_);
	//QScrollerProperties properties = scroller->scrollerProperties();

	// properties.setScrollMetric(QScrollerProperties::DragStartDistance, 3.0);
	// properties.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.2);
	// properties.setScrollMetric(QScrollerProperties::OvershootScrollTime, 0.4);

	// scroller->setScrollerProperties(properties);

	addDeviceBtn_ = new QPushButton(lstr("+ 添加设备"), this);
	addDeviceBtn_->setFixedHeight(35);
	connLayout->addWidget(addDeviceBtn_, 0, Qt::AlignBottom);

	connect(addDeviceBtn_, &QPushButton::clicked, this, &ConnectedDevicesPanel::addDeviceRequested);

	auto* uiSession = UiSession::instance();
	connect(uiSession, &UiSession::themeChanged, this, &ConnectedDevicesPanel::updateThemeAndStyles);
	connect(uiSession, &UiSession::fontChanged, this, &ConnectedDevicesPanel::updateThemeAndStyles);

	this->updateThemeAndStyles();
	this->updateDisplay();
}

qsizetype ConnectedDevicesPanel::appendWidget(QWidget* widget) {
	qsizetype res = Extensible::appendWidget(widget);
	if (widget && listWidget_) {
		widget->setParent(this);
		auto* item = new QListWidgetItem(listWidget_);

		int itemHeight = widget->sizeHint().height() > 0 ? widget->sizeHint().height() : 60;
		item->setSizeHint(QSize(listWidget_->width(), itemHeight));

		listWidget_->addItem(item);
		listWidget_->setItemWidget(item, widget);
	}
	updateDisplay();
	return res;
}

QWidget* ConnectedDevicesPanel::widget(qsizetype id, QWidget* widget) {
	QWidget* res = Extensible::widget(id, widget);
	if (res && listWidget_ && widget) {
		for (int i = 0; i < listWidget_->count(); ++i) {
			QListWidgetItem* item = listWidget_->item(i);
			if (listWidget_->itemWidget(item) == res) {
				widget->setParent(this);
				int itemHeight = widget->sizeHint().height() > 0 ? widget->sizeHint().height() : 60;
				item->setSizeHint(QSize(listWidget_->width(), itemHeight));
				listWidget_->setItemWidget(item, widget);
				break;
			}
		}
	}
	updateDisplay();
	return res;
}

QWidget* ConnectedDevicesPanel::removeWidget(qsizetype id) noexcept {
	QWidget* res = Extensible::removeWidget(id);
	if (res && listWidget_) {
		for (int i = 0; i < listWidget_->count(); ++i) {
			QListWidgetItem* item = listWidget_->item(i);
			if (listWidget_->itemWidget(item) == res) {
				delete listWidget_->takeItem(i);
				break;
			}
		}
	}
	updateDisplay();
	return res;
}

void ConnectedDevicesPanel::updateDisplay() {
	bool haveList = !widgets_.isEmpty();

	if (hintLabel_) {
		hintLabel_->setVisible(isExpanded_ && !haveList);
	}
	if (listWidget_) {
		listWidget_->setVisible(isExpanded_ && haveList);
	}
	if (addDeviceBtn_) {
		addDeviceBtn_->setVisible(isExpanded_);
	}
}

void ConnectedDevicesPanel::setExpanded(bool expanded) {
	isExpanded_ = expanded;
	titleBar_->setVisible(!expanded);
	updateDisplay();
}

void ConnectedDevicesPanel::updateThemeAndStyles() {
	auto theme = UiSession::instance()->theme();
	auto font = UiSession::instance()->font();

	QString bgColor = theme.value(lstr("background"), lstr("#F8F9FA")).toString();
	QString surfaceColor = theme.value(lstr("surface"), lstr("#FFFFFF")).toString();
	QString borderColor = theme.value(lstr("border"), lstr("#D9E2EC")).toString();
	QString successColor = theme.value(lstr("success"), lstr("#22C55E")).toString();
	QString textSecondary = theme.value(lstr("textSecondary"), lstr("#64748B")).toString();
	QString textHint = theme.value(lstr("textHint"), lstr("#999999")).toString();

	QString fontFamily = font.value(lstr("family"), lstr("Microsoft YaHei")).toString();
	int sizeNormal = font.value(lstr("sizeNormal"), 14).toInt();
	int sizeSubtitle = font.value(lstr("sizeLarge"), 16).toInt();

	this->setStyleSheet(lstr("#ConnectedDevicesPanel { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border-radius: 16px; border: 1px solid %3; }")
		.arg(bgColor, surfaceColor, borderColor));

	titleBar_->setStyleSheet(lstr("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border: none;").arg(bgColor, surfaceColor));
	titleBar_->setLabelStyle(lstr("font-family: '%1'; font-weight: bold; font-size: %2px; color: %3; line-height: 1.2;")
		.arg(fontFamily).arg(sizeSubtitle).arg(textSecondary));

	listWidget_->setStyleSheet(lstr("background: transparent; border: none;"));
	addDeviceBtn_->setStyleSheet(lstr("font-family: '%1'; font-weight: bold; font-size: %2px; background-color: %3; border: 1px dashed %4; color: %5; border-radius: 8px;")
		.arg(fontFamily).arg(sizeNormal).arg(surfaceColor, borderColor, successColor));
	hintLabel_->setStyleSheet(lstr("font-family: '%1'; font-size: %2px; color: %3; border: none; background: transparent;")
		.arg(fontFamily).arg(sizeNormal).arg(textHint));
}

// ====================================
// RecordsPanel —— 历史记录面板
// ====================================

RecordsPanel::RecordsPanel(QWidget* parent) : Extensible(parent) {
	this->setAttribute(Qt::WA_StyledBackground);
	this->setObjectName("RecordsPanel");

	auto* masterLayout = new QHBoxLayout(this);
	masterLayout->setContentsMargins(0, 0, 0, 0);
	masterLayout->setSpacing(0);

	titleBar_ = new VerticalTitleBar(lstr("历史记录"), this);
	masterLayout->addWidget(titleBar_, Qt::AlignCenter);
	connect(titleBar_, &VerticalTitleBar::clicked, this, &RecordsPanel::titleClicked);

	auto* histLayout = new QVBoxLayout();
	histLayout->setContentsMargins(15, 15, 15, 15);
	masterLayout->addLayout(histLayout);

	listWidget_ = new QListWidget(this);
	listWidget_->setSelectionMode(QListWidget::NoSelection);
	listWidget_->setStyleSheet(lstr("outline: none; background: transparent; border: none; }"));
	listWidget_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	listWidget_->setSpacing(4);
	histLayout->addWidget(listWidget_);

	hintLabel_ = new QLabel(lstr("暂无历史记录"), this);
	hintLabel_->setAlignment(Qt::AlignCenter);
	histLayout->addWidget(hintLabel_, 1, Qt::AlignCenter);

	auto* uiSession = UiSession::instance();
	connect(uiSession, &UiSession::themeChanged, this, &RecordsPanel::updateThemeAndStyles);
	connect(uiSession, &UiSession::fontChanged, this, &RecordsPanel::updateThemeAndStyles);

	this->updateThemeAndStyles();
	this->updateDisplay();
}

qsizetype RecordsPanel::appendWidget(QWidget* widget) {
	qsizetype res = Extensible::appendWidget(widget);
	if (widget && listWidget_) {
		widget->setParent(this);
		auto* item = new QListWidgetItem(listWidget_);
		item->setSizeHint(QSize(this->width(), 150));
		listWidget_->addItem(item);
		listWidget_->setItemWidget(item, widget);
	}
	updateDisplay();
	return res;
}

QWidget* RecordsPanel::widget(qsizetype id, QWidget* widget) {
	QWidget* res = Extensible::widget(id, widget);
	if (widget && listWidget_) {
		widget->setParent(this);
		auto* item = new QListWidgetItem(listWidget_);
		item->setSizeHint(QSize(this->width(), 150));
		listWidget_->addItem(item);
		listWidget_->setItemWidget(item, widget);
	}
	updateDisplay();
	return res;
}

QWidget* RecordsPanel::removeWidget(qsizetype id) noexcept {
	QWidget* res = Extensible::removeWidget(id);
	if (res && listWidget_) {
		for (int i = 0; i < listWidget_->count(); ++i) {
			QListWidgetItem* item = listWidget_->item(i);
			if (listWidget_->itemWidget(item) == res) {
				delete listWidget_->takeItem(i);
				break;
			}
		}
	}
	updateDisplay();
	return res;
}

void RecordsPanel::setExpanded(bool expanded) {
	isExpanded_ = expanded;
	titleBar_->setVisible(!expanded);
	updateDisplay();
}

void RecordsPanel::updateDisplay() {
	if (!listWidget_ || !hintLabel_) return;

	bool haveList = !widgets_.isEmpty() || (listWidget_->count() > 0);

	hintLabel_->setVisible(isExpanded_ && !haveList);
	listWidget_->setVisible(isExpanded_ && haveList);
}

void RecordsPanel::updateThemeAndStyles() {
	auto theme = UiSession::instance()->theme();
	auto font = UiSession::instance()->font();

	QString bgColor = theme.value(lstr("background"), lstr("#F8F9FA")).toString();
	QString surfaceColor = theme.value(lstr("surface"), lstr("#FFFFFF")).toString();
	QString borderColor = theme.value(lstr("border"), lstr("#D9E2EC")).toString();
	QString textSecondary = theme.value(lstr("textSecondary"), lstr("#64748B")).toString();
	QString textHint = theme.value(lstr("textHint"), lstr("#999999")).toString();

	QString fontFamily = font.value(lstr("family"), lstr("Microsoft YaHei")).toString();
	int sizeNormal = font.value(lstr("sizeNormal"), 14).toInt();
	int sizeSubtitle = font.value(lstr("sizeLarge"), 16).toInt();

	this->setStyleSheet(lstr("#RecordsPanel { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border-radius: 16px; border: 1px solid %3; }")
		.arg(bgColor, surfaceColor, borderColor));

	titleBar_->setStyleSheet(lstr("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2); border: none;").arg(bgColor, surfaceColor));

	titleBar_->setLabelStyle(lstr("font-family: '%1'; font-weight: bold; font-size: %2px; color: %3; line-height: 1.2;")
		.arg(fontFamily).arg(sizeSubtitle).arg(textSecondary));

	hintLabel_->setStyleSheet(lstr("font-family: '%1'; font-size: %2px; color: %3; border: none; background: transparent;")
		.arg(fontFamily).arg(sizeNormal).arg(textHint));
}

// ===================================
// PageDevice —— 主页面整体包装
// ===================================

PageDevice::PageDevice(QWidget* parent) : QWidget(parent) {
	this->setAttribute(Qt::WA_StyledBackground);
	this->setProperty("iconText", lstr("📱"));
	this->setProperty("title", lstr("设备"));

	this->setMinimumWidth(800);

	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	mainLayout_ = new QHBoxLayout(this);
	mainLayout_->setContentsMargins(20, 20, 20, 20);
	mainLayout_->setSpacing(12);

	searchPanel_ = new DeviceSearchPanel(this);
	connectedPanel_ = new ConnectedDevicesPanel(this);
	recordPanel_ = new RecordsPanel(this);

	mainLayout_->addWidget(searchPanel_);
	mainLayout_->addWidget(connectedPanel_);
	mainLayout_->addWidget(recordPanel_);

	connect(searchPanel_, &DeviceSearchPanel::titleClicked, this, &PageDevice::handleLeft);
	connect(connectedPanel_, &ConnectedDevicesPanel::titleClicked, this, &PageDevice::handleCenter);
	connect(recordPanel_, &RecordsPanel::titleClicked, this, &PageDevice::handleRight);
	
	connect(connectedPanel_, &ConnectedDevicesPanel::addDeviceRequested, this, [this]() {
		this->handleLeft();
		this->searchPanel_->triggerSearch();
		});

	auto* session = DeviceSession::instance();
	connect(session, &DeviceSession::deviceConnected, this, &PageDevice::onDeviceConnected);

	auto* uiSession = UiSession::instance();
	connect(uiSession, &UiSession::themeChanged, this, &PageDevice::updateTopTheme);

	updateTopTheme();
	transitionToState(DisplayState::CenteredState);
}

Extensible* PageDevice::search() { return this->searchPanel_; }
Extensible* PageDevice::record() { return this->recordPanel_; }
Extensible* PageDevice::connected() { return this->connectedPanel_; }

void PageDevice::onDeviceConnected(int driver) {
	transitionToState(DisplayState::CenteredState);
}

void PageDevice::handleLeft() { transitionToState(DisplayState::LeftState); }
void PageDevice::handleCenter() { transitionToState(DisplayState::CenteredState); }
void PageDevice::handleRight() { transitionToState(DisplayState::RightState); }

void PageDevice::transitionToState(int newState) {
	currentState = newState;

	int bleStretch = 0;
	int connStretch = 0;
	int histStretch = 0;

	switch (currentState) {
	case DisplayState::LeftState:
		bleStretch = 1;
		searchPanel_->setExpanded(true);
		connectedPanel_->setExpanded(false);
		recordPanel_->setExpanded(false);
		break;
	case DisplayState::CenteredState:
		connStretch = 1;
		searchPanel_->setExpanded(false);
		connectedPanel_->setExpanded(true);
		recordPanel_->setExpanded(false);
		break;
	case DisplayState::RightState:
		histStretch = 1;
		searchPanel_->setExpanded(false);
		connectedPanel_->setExpanded(false);
		recordPanel_->setExpanded(true);
		break;
	default:
		qCritical() << "[PageDevice ERROR] Unknown state:" << newState;
		return;
	}

	mainLayout_->setStretchFactor(searchPanel_, bleStretch);
	mainLayout_->setStretchFactor(connectedPanel_, connStretch);
	mainLayout_->setStretchFactor(recordPanel_, histStretch);

	mainLayout_->invalidate();
	mainLayout_->activate();

	this->adjustSize();
}

void PageDevice::updateTopTheme() {
	auto theme = UiSession::instance()->theme();
	QString bgColor = theme.value(lstr("background"), lstr("#F8F9FA")).toString();
	this->setStyleSheet(lstr("background-color: %1;").arg(bgColor));
}