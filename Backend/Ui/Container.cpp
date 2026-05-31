#include <QLayout>
#include <QMouseEvent>
#include <QStyle>
#include <QDebug>
#include <QScroller>
#include <QScrollBar>
#include <QEvent>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QUrl>
#include <QIcon>
#include <QResizeEvent>
#include <QPropertyAnimation>

#include "Backend/Ui/Session.hpp"
#include "ContainerPrivate.hpp"
#include "Container.hpp"

template <typename T>
static constexpr T clamp(T val, T min, T max) {
	return (val < min) ? min : ((val > max) ? max : val);
}

Container::Container(QWidget* parent)
	: Extensible(parent) {
	ui_ = UiSession::instance();
	Q_ASSERT_X(ui_, "Container::Container", "UiSession should initialize before container.");

	scrollArea_ = new QScrollArea(this);
	scrollArea_->setObjectName("appScrollArea");
	scrollArea_->setGeometry(this->rect());
	scrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea_->setFrameShape(QFrame::NoFrame);
	scrollArea_->setFocusPolicy(Qt::NoFocus);

	scrollArea_->setWidgetResizable(false);

	scrollContent_ = new QWidget();
	scrollContent_->setObjectName("scrollContent");

	contentLayout_ = new QHBoxLayout(scrollContent_);
	contentLayout_->setContentsMargins(24, 12, 24, 12);
	contentLayout_->setSpacing(0);
	scrollContent_->setLayout(contentLayout_);

	scrollArea_->setWidget(scrollContent_);

	QScroller::grabGesture(scrollArea_->viewport(), QScroller::LeftMouseButtonGesture);

	auto* scroller = QScroller::scroller(scrollArea_->viewport());
	auto props = scroller->scrollerProperties();
	props.setScrollMetric(QScrollerProperties::FrameRate, QScrollerProperties::Fps60);
	props.setScrollMetric(QScrollerProperties::DragStartDistance, 0.005);
	props.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
		QScrollerProperties::OvershootScrollDistanceFactor);
	scroller->setScrollerProperties(props);

	scrollArea_->viewport()->installEventFilter(this);
	connect(scrollArea_->horizontalScrollBar(), &QScrollBar::valueChanged,
		this, &Container::onScrollValueChanged);

	this->navContainer_ = new QWidget(this);
	this->navContainer_->setObjectName("navContainer");
	auto* navLayout = new QHBoxLayout(navContainer_);
	navLayout->setContentsMargins(0, 0, 0, 0);
	navLayout->setSpacing(8);
	this->navContainer_->setLayout(navLayout);

	connect(ui_, &UiSession::themeChanged, this, &Container::applyTheme);

	this->applyTheme();
	this->ui_->main(this);

	this->syncLayout();
}

int Container::currentIndex() const {
	return currentIndex_.value();
}

void Container::setCurrentIndex(int v) {
	const auto& widgets = this->widgets();
	const int count = widgets.size();

	if (v == currentIndex_.value())
		return;
	if (v < 0 || v >= count) {
		qCritical().nospace() << "[Container Error] index:" << v
			<< " Out of range [0," << count << "), operation aborted.";
		return;
	}

	currentIndex_.setValue(v);

	if (scrollArea_) {
		int targetX = contentLayout_->contentsMargins().left();
		const int viewportW = this->width();
		const int minAllowedW = viewportW / 2 + 24;
		const int spacing = contentLayout_->spacing();

		for (int i = 0; i < v; ++i) {
			if (widgets[i]) {
				int pageW = qMax(minAllowedW, qMax(widgets[i]->minimumWidth(), widgets[i]->sizeHint().width()));
				targetX += pageW + spacing;
			}
		}

		int targetWidgetW = 0;
		if (widgets[v]) {
			targetWidgetW = qMax(minAllowedW, qMax(widgets[v]->minimumWidth(), widgets[v]->sizeHint().width()));
		}

		targetX = targetX + (targetWidgetW / 2) - (viewportW / 2);

		isInternalScrolling_ = true;
		QScroller::scroller(scrollArea_->viewport())->scrollTo(QPointF(targetX, 0), 300);
		isInternalScrolling_ = false;
	}

	setCurrentTabbarIndex(v);
}
QBindable<int> Container::bindableCurrentIndex() {
	return &currentIndex_;
}

void Container::setCurrentTabbarIndex(int v) {
	for (int i = 0; i < navButtons_.size(); ++i) {
		navButtons_[i]->setProperty("active", i == v);
		navButtons_[i]->style()->unpolish(navButtons_[i]);
		navButtons_[i]->style()->polish(navButtons_[i]);
		navButtons_[i]->update();
	}
}

void Container::onScrollValueChanged(int value) {
	isDrag_ = true;

	const int viewportW = width();
	const auto& widgets = this->widgets();
	const int pageCount = widgets.size();

	if (viewportW > 0 && !isInternalScrolling_ && pageCount > 0) {
		int targetIndex = 0;
		int cumulativeX = contentLayout_ ? contentLayout_->contentsMargins().left() : 24;
		const int minAllowedW = viewportW / 2 + 24;
		const int spacing = contentLayout_ ? contentLayout_->spacing() : 0;

		for (int i = 0; i < pageCount; ++i) {
			if (!widgets[i]) continue;
			int pageW = qMax(minAllowedW, qMax(widgets[i]->minimumWidth(), widgets[i]->sizeHint().width()));

			if (value < cumulativeX + pageW / 2) {
				targetIndex = i;
				break;
			}
			cumulativeX += pageW + spacing;
			targetIndex = i;
		}

		targetIndex = clamp(targetIndex, 0, pageCount - 1);

		if (targetIndex != currentIndex_.value()) {
			currentIndex_.setValue(targetIndex);
			setCurrentTabbarIndex(targetIndex);
		}
	}

	updateTransforms(value);
}

void Container::updateTransforms(int scrollOffset) {
	Q_UNUSED(scrollOffset);
}

bool Container::eventFilter(QObject* watcher, QEvent* e) {
	if (watcher == scrollArea_->viewport()) {
		switch (e->type()) {
		case QEvent::MouseButtonRelease:
			isDrag_ = false;
			break;
		case QEvent::Leave:
			isDrag_ = false;
			break;
		default:
			break;
		}
	}
	else if (this->widgets().contains(static_cast<QWidget*>(watcher))) {
		if (e->type() == QEvent::Resize) {
			if (!this->property("_isSyncingLayout").toBool()) {
				this->setProperty("_isSyncingLayout", true);
				this->syncLayout();
				this->setProperty("_isSyncingLayout", false);
			}
		}
	}
	return QWidget::eventFilter(watcher, e);
}

qsizetype Container::appendWidget(QWidget* widget) {
	QString iconText, title;
	QUrl iconUrl;

	if (!assign(iconText, widget->property("iconText")) &&
		!assign(iconUrl, widget->property("iconUrl"))) {
		qWarning() << "[Container WARNING] Page requires `iconText` or `iconUrl`.";
		return qsizetype(-1);
	}

	if (!assign(title, widget->property("title"))) {
		qWarning() << "[Container WARNING] Page requires `title`.";
		return qsizetype(-1);
	}

	auto* btn = new QPushButton(navContainer_);
	btn->setFixedSize(36, 36);
	btn->setCursor(Qt::PointingHandCursor);
	btn->setObjectName("navButton");
	btn->show();

	if (!iconUrl.isEmpty()) {
		btn->setIcon(QIcon(iconUrl.isLocalFile() ? iconUrl.toLocalFile() : iconUrl.toString()));
	}
	else {
		btn->setText(iconText.isEmpty() ? title.left(1) : iconText);
	}

	connect(btn, &QPushButton::clicked, this, [this, widget]() {
		int idx = this->widgets().indexOf(widget);
		this->setCurrentIndex(idx);
		});

	auto* layout = static_cast<QHBoxLayout*>(navContainer_->layout());
	layout->addWidget(btn);
	navButtons_.append(btn);

	auto c = this->Extensible::appendWidget(widget);

	widget->setParent(scrollContent_);
	widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	contentLayout_->addWidget(widget);

	widget->installEventFilter(this);

	this->syncLayout();
	this->navContainer_->raise();
	this->applyTheme();
	return c;
}

QWidget* Container::removeWidget(qsizetype id) noexcept {
	auto w = Extensible::removeWidget(id);
	if (w) {
		auto index = this->contentLayout_->indexOf(w);
		if (index >= 0) {
			this->contentLayout_->removeWidget(w);
			w->removeEventFilter(this);
			delete navButtons_.takeAt(index);
			delete pageTitles_.takeAt(index);
			this->setCurrentIndex(qMax(0, this->widgets().size() - 1));
			this->syncLayout();
		}
	}
	return w;
}

void Container::syncLayout(bool setScroll) {
	const int viewportW = this->width();
	const int viewportH = this->height();
	const int topMargin = 12;
	const int bottomMargin = 12;
	const int leftMargin = 24;
	const int rightMargin = 24;
	const int contentH = qMax(0, viewportH - topMargin - bottomMargin);

	contentLayout_->setContentsMargins(leftMargin, topMargin, rightMargin, bottomMargin);

	navContainer_->setGeometry(16, 16, navButtons_.size() * 44, 40);
	navContainer_->raise();

	const auto& widgets = this->widgets();
	const int minAllowedW = viewportW / 2 + 24;
	const int spacing = contentLayout_->spacing();

	int totalContentW = leftMargin + rightMargin;
	int visibleWidgetCount = 0;

	for (auto* p : widgets) {
		if (!p) continue;

		int pageW = qMax(minAllowedW, qMax(p->minimumWidth(), p->width()));
		QSize targetSize(pageW, contentH);
		if (p->size() != targetSize) {
			p->setFixedSize(targetSize);
		}

		totalContentW += pageW;
		visibleWidgetCount++;
	}

	if (visibleWidgetCount > 1) {
		totalContentW += (visibleWidgetCount - 1) * spacing;
	}

	QSize targetContentSize(qMax(1, totalContentW), viewportH);
	if (scrollContent_->size() != targetContentSize) {
		scrollContent_->setFixedSize(targetContentSize);
	}

	scrollArea_->setGeometry(0, 0, viewportW, viewportH);

	contentLayout_->invalidate();
	contentLayout_->activate();

	// isInternalScrolling_ = true;
	// if (setScroll) {
	// 	if (!widgets.isEmpty()) {
	// 		int targetX = leftMargin;
	// 		int curIdx = clamp(currentIndex_.value(), 0, (int)widgets.size() - 1);
	// 		for (int i = 0; i < curIdx; ++i) {
	// 			if (widgets[i]) {
	// 				int pageW = qMax(minAllowedW, qMax(widgets[i]->minimumWidth(), widgets[i]->sizeHint().width()));
	// 				targetX += pageW + spacing;
	// 			}
	// 		}
	// 		scrollArea_->horizontalScrollBar()->setValue(targetX);
	// 	}
	// 	else {
	// 		scrollArea_->horizontalScrollBar()->setValue(0);
	// 	}
	// }
	// isInternalScrolling_ = false;
}

void Container::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	this->syncLayout();
}

void Container::fadeIn() {}
void Container::fadeOut() {}

void Container::applyTheme() {
	if (!ui_) return;

	const QVariantMap theme = ui_->theme();
	const QVariantMap fontCfg = ui_->font();

	const auto bg = map(theme, lstr("background"), lstr("#F7FAFC"));
	const auto surface = map(theme, lstr("surface"), lstr("#FFFFFF"));
	const auto border = map(theme, lstr("border"), lstr("#D9E2EC"));
	const auto grid = map(theme, lstr("grid"), lstr("#E5EEF5"));
	const auto textPrimary = map(theme, lstr("textPrimary"), lstr("#1F2937"));
	const auto primary = map(theme, lstr("primary"), lstr("#3B82F6"));

	const auto family = map(fontCfg, lstr("family"), lstr("Microsoft YaHei"));
	const auto titleSize = map(fontCfg, lstr("titleSize"), 22);
	const auto titleWeight = map(fontCfg, lstr("titleWeight"), 700);

	this->setStyleSheet(lstr(
		"Container { background-color: %1; }"
	).arg(bg));

	scrollArea_->setStyleSheet(lstr(
		"QScrollArea#appScrollArea { background-color: %1; border: none; }"
		"QWidget#scrollContent { background: transparent; }"
	).arg(bg));

	for (QLabel* label : pageTitles_) {
		label->setStyleSheet(lstr(
			"color: %1; font-family: '%2'; font-size: %3px; font-weight: %4; background: transparent;"
		).arg(textPrimary, family).arg(titleSize).arg(titleWeight));
	}

	for (QPushButton* btn : navButtons_) {
		btn->setStyleSheet(lstr(
			"QPushButton { background-color: %1; border: 1px solid %2; border-radius: 10px; color: %3; }"
			"QPushButton:hover { background-color: %4; border-color: %5; }"
			"QPushButton[active='true'] { background-color: %6; border-color: %5; }"
		).arg(surface, border, textPrimary, grid, primary, alpha(primary, 0.12)));
	}
}