#include "Extensible.hpp"

static constexpr auto K_Id = "_id_E_";

static auto ID_GEN = 0u;

auto selectWidget(qsizetype id, auto& widgets) {
	for (auto it = widgets.begin(); it != widgets.end(); it++) {
		auto idx = (*it)->property(K_Id).toInt();
		if (idx == id) return it;
	} return widgets.end();
}

QString alpha(const QColor& color, qreal alpha) {
	const int alphaByte = qBound(0, qRound(alpha * 255.0), 255);
	return lstr("rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(alphaByte);
}

qsizetype Extensible::appendWidget(QWidget* widget) {
	if (widget == nullptr) return -1;
	auto value = ID_GEN++;
	widget->setProperty(K_Id, value);
	this->widgets_.emplaceBack(widget)->setParent(this);
	return value;
}

// return old one.
QWidget* Extensible::widget(qsizetype index, QWidget* widget) {
	auto it = selectWidget(index, this->widgets_);
	auto r = qExchange(*it, widget);
	qsizetype id = -1;
	if (r) {
		r->setParent(nullptr);
		id = getWidgetId(widget);
	}
	if (widget) {
		widget->setProperty(K_Id, id == -1 ? ID_GEN : id);
		widget->setParent(this);
	}
	return r;
}

QWidget* Extensible::removeWidget(qsizetype index) noexcept {
	auto it = selectWidget(index, this->widgets_);
	if (it != this->widgets_.end()) {
		auto c = *it;
		widgets_.erase(it);
		return c;
	} else 
		return nullptr;
}

QWidget* Extensible::removeWidget(QWidget* w) noexcept {
	for (auto it = this->widgets_.begin(); it != this->widgets_.end();) {
		if (w == *it) {
			it = widgets_.erase(it);
			w->setParent(nullptr);
			break;
		} else 
			it++;
	}
	return w;
}

// return widget at index.
QWidget* Extensible::widget(qsizetype index) const noexcept {
	return *selectWidget(index, this->widgets_);
}

QList<QWidget*> Extensible::exchangeWidgets(QList<QWidget*> newone) {
	for (auto widget : widgets_)
		if (widget) widget->setParent(nullptr);
	for (auto widget : newone)
		if (widget) widget->setParent(this);
	return qExchange(widgets_, qMove(newone));
}

qsizetype Extensible::getWidgetId(QWidget* widget) {
	qsizetype value = -1;
	if (widget) assign(value, widget->property(K_Id));
	return value;
}

