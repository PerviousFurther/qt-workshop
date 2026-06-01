#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QVariant>
#include <QLabel>

#include "Field.hpp"
#include "DetailPage.hpp"
#include "Backend/Ui/Session.hpp"

UiTestDetailPage::UiTestDetailPage(int id, QString title, QWidget* parent)
	: Extensible(parent)
	, deviceId_(id)
{
	// 1. 将主布局改为垂直布局，让顶部栏和横向拓展区上下堆叠
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(10);
	// this->setLayout(mainLayout);

	// 2. 创建顶部栏布局（水平布局）
	QHBoxLayout* topLayout = new QHBoxLayout();

	backButton_ = new QPushButton("<", this);
	backButton_->setObjectName("backButton");

	// sendButton_ = new QPushButton("开始测量", this);
	// sendButton_->setObjectName("sendButton");

	titleLabel_ = new QLabel(title, this);
	titleLabel_->setObjectName("title");
	titleLabel_->setAlignment(Qt::AlignCenter);

	topLayout->addWidget(backButton_);
	topLayout->addStretch();
	topLayout->addWidget(titleLabel_);
	// topLayout->addStretch();
	// topLayout->addWidget(sendButton_);

	// 将顶部栏加到主布局最上方
	mainLayout->addLayout(topLayout);

	// 3. 初始化滚动区域（即你的“横向拓展区”）
	scrollArea_ = new QScrollArea(this);
	scrollArea_->setWidgetResizable(true);

	scrollContent_ = new QWidget(scrollArea_);
	scrollLayout_ = new QVBoxLayout(); // 内部组件仍可保持垂直排列，且会自动横向拉伸占满
	scrollLayout_->setAlignment(Qt::AlignTop);
	scrollContent_->setLayout(scrollLayout_);

	scrollArea_->setWidget(scrollContent_);

	mainLayout->addWidget(scrollArea_, 1);

	// 4. 信号槽连接
	// connect(sendButton_, &QPushButton::clicked, this, &UiTestDetailPage::collectAndSendBytes);
	connect(backButton_, &QPushButton::clicked, this, &UiTestDetailPage::requestBack);

	UiSession* session = UiSession::instance();
	if (session) {
		connect(session, &UiSession::themeChanged, this, &UiTestDetailPage::updateStyles);
		connect(session, &UiSession::fontChanged, this, &UiTestDetailPage::updateStyles);

		this->updateStyles();
	}
}

void UiTestDetailPage::updateStyles() {
	UiSession* session = UiSession::instance();
	// if (!session) return;

	QVariantMap themeMap = session->theme();
	QVariantMap fontMap = session->font();

	QString background = themeMap.value(UiField::Background).toString();
	QString surface = themeMap.value(UiField::Surface).toString();
	QString border = themeMap.value(UiField::Border).toString();
	QString grid = themeMap.value(UiField::Grid).toString();
	QString primary = themeMap.value(UiField::Primary).toString();
	QString danger = themeMap.value(UiField::Danger).toString();
	QString textPrimary = themeMap.value(UiField::TextPrimary).toString();
	QString textSecondary = themeMap.value(UiField::TextSecondary).toString();
	QString textHint = themeMap.value(UiField::TextHint).toString();

	QString fontFamily = fontMap.value(UiField::Family, "Microsoft YaHei").toString();
	int sizeNormal = fontMap.value(UiField::SizeNormal, 14).toInt();
	int sizeSmall = fontMap.value(UiField::SizeSmall, 12).toInt();
	int sizeTitle = fontMap.value(UiField::SizeTitle, 20).toInt();

	QString qss = lstr(
		"UiTestDetailPage { "
		"   background-color: %1; "
		"   font-family: '%2'; "
		"   font-size: %3px; "
		"   color: %4; "
		"}"
		"QScrollArea { "
		"   border: none; "
		"   background: transparent; "
		"}"
		"QPushButton { "
		"   background-color: %5; "
		"   color: #FFFFFF; "
		"   border: 1px solid %6; "
		"   border-radius: 6px; "
		"   padding: 8px 16px; "
		"   font-size: %3px; "
		"   font-weight: bold; "
		"}"
		"QPushButton:hover { "
		"   background-color: %7; "
		"   border-color: %5; "
		"}"
		"QPushButton:disabled { "
		"   background-color: %8; "
		"   color: %9; "
		"   border-color: %8; "
		"}"
		"QPushButton#backButton { "
		"   background-color: transparent; "
		"   color: %4; "
		"   border: 1px solid %6; "
		"   padding: 8px 12px; "
		"}"
		"QPushButton#backButton:hover { "
		"   background-color: %7; "
		"}"
		"QLabel#title { "
		"   color: %4; "
		"   font-size: %10px; "
		"   font-weight: bold; "
		"}"
	)
		.arg(background)     // %1
		.arg(fontFamily)     // %2
		.arg(sizeNormal)     // %3
		.arg(textPrimary)    // %4
		.arg(primary)        // %5
		.arg(border)         // %6
		.arg(grid)           // %7
		.arg(border)         // %8
		.arg(textHint)       // %9
		.arg(sizeTitle);
	// scrollArea_->setFrameShape(QFrame::NoFrame);
	// scrollArea_->setStyleSheet(lstr("QScrollArea { background: %2; border-color: %3; }")
	// 	.arg(background).arg(surface).arg(border));
	scrollContent_->setStyleSheet(lstr("background: %1; border-color: %2; ").arg(surface).arg(border));

	this->setStyleSheet(qss);
}

qsizetype UiTestDetailPage::appendWidget(QWidget* widget) {
	if (!widget) return -1;
	qsizetype index = Extensible::appendWidget(widget);
	if (index != -1) {
		scrollLayout_->addWidget(widget);
		widget->setParent(scrollContent_);
	}
	return index;
}

QWidget* UiTestDetailPage::removeWidget(qsizetype id) noexcept {
	QWidget* widget = Extensible::removeWidget(id);
	if (widget) {
		scrollLayout_->removeWidget(widget);
		widget->setParent(nullptr);
	}
	return widget;
}

void UiTestDetailPage::collectAndSendBytes() {
	// QByteArray combinedBytes;
	// QList<QWidget*> allWidgets = widgets();
	// for (QWidget* w : allWidgets) {
	// 	if (!w) continue;
	// 
	// 	bool isEnabled = w->property(DeviceField::UiTest::Enabled).toBool();
	// 	if (isEnabled) {
	// 		QVariant bytesVar = w->property(DeviceField::UiTest::Bytes);
	// 		if (bytesVar.isValid()) {
	// 			combinedBytes.append(bytesVar.toByteArray());
	// 		}
	// 	}
	// }
	// 
	// if (!combinedBytes.isEmpty()) {
	// 	sendButton_->setEnabled(false);
	// 	emit this->requestMeasure(deviceId_, combinedBytes);
	// }

	emit this->requestMeasure(this->deviceId_);
}

void UiTestDetailPage::onMeasurementResultReceived(bool success) {
	// sendButton_->setEnabled(true);
	emit this->measurementFinished(success);
}