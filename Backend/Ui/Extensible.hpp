#pragma once

#include <QWidget>
#include <QColor>

#include "Core/Utils.hpp"

// Replace alpha of some color.
QString alpha(const QColor& color, qreal alpha);

class 
#if defined(RINGAPP_GUI_UI_EXPORT)
	Q_DECL_EXPORT
#else
	Q_DECL_IMPORT
#endif
Extensible : public QWidget {
	Q_OBJECT;
public:
	using QWidget::QWidget;

	// return widget at index.
	QWidget* widget(qsizetype id) const noexcept;
	// Get all widgets.
	QList<QWidget*> widgets() const noexcept { return this->widgets_; }

public slots:
	// return index of new one, take ownership.
	virtual qsizetype appendWidget(QWidget* widget);
	// return old one, detach ownership.
	virtual QWidget* widget(qsizetype id, QWidget* widget);
	// remove it and return, detach ownership.
	virtual QWidget* removeWidget(qsizetype id) noexcept;
	virtual QWidget* removeWidget(QWidget* widget) noexcept;
	// only allow call when the widget is append by same dll point.
	void deleteWidget(qsizetype id) noexcept { delete removeWidget(id); }
	// only allow call when the widget is append by same dll point.
	void deleteWidget(QWidget* widget) noexcept { deleteWidget(getWidgetId(widget)); }

	// Exchange widgets, swtich ownership.
	QList<QWidget*> exchangeWidgets(QList<QWidget*> newone);
	// Switch the widgets and delete old ones.
	// However it is not cross dll safe.
	// Remember, if all the widget is not create from the same dll, the operation will kill the heap.
	// void setWidgets(QList<QWidget*> newone) {
	// 	for (auto widget : widgets_)
	// 		delete widget;
	// 	for (auto widget : newone)
	// 		if (widget) widget->setParent(this);
	// 	this->widgets_ = newone;
	// }

protected:
	QList<QWidget*> widgets_;

private:
	qsizetype getWidgetId(QWidget* widget);
};