#pragma once

// #include <QQmlProperty>
#include <QVariantMap>

#include "Core/Utils.hpp"
#include "Core/Module.hpp"

#include "Field.hpp"

class Extensible;
class QWidget;
class
#if defined(RINGAPP_GUI_UI_EXPORT)
	Q_DECL_EXPORT
#else
	Q_DECL_IMPORT
#endif
UiSession : public Module {
	Q_OBJECT;
public:
	using Module::Module;

	static QString moduleName();
	static UiSession* instance() { return instance_; }

	Q_PROPERTY(QVariantMap theme READ theme WRITE setTheme NOTIFY themeChanged);
	Q_PROPERTY(QVariantMap font READ font NOTIFY fontChanged);
	Q_PROPERTY(QVariantMap app READ app NOTIFY appChanged);
	Q_PROPERTY(QList<QVariantMap> themes READ themes NOTIFY themesChanged);

	// Q_PROPERTY(QString mainEntry READ mainEntry NOTIFY updatedUi);

signals:
	void updating();
	void updated();
	void fontChanged();
	void appChanged();
	void themeChanged();
	void themesChanged();

	// void fontUpdated();

public:
	virtual QVariantMap theme() = 0;
	virtual void setTheme(QVariantMap) = 0;

	virtual QVariantMap font() = 0;
	virtual void setFont(QVariantMap) = 0;

	virtual QVariantMap app() = 0;
	virtual QList<QVariantMap> themes() = 0;

	// Main page container.
	virtual Extensible* main() = 0;
	virtual void main(Extensible* mainpage) = 0;

	// Home page.
	virtual Extensible* home() = 0;	
	// Setting page.
	virtual Extensible* settings() = 0;

	virtual Extensible* searchDevicePage() = 0;
	virtual Extensible* connectedDevice() = 0;
	virtual Extensible* deviceRecords() = 0;

	virtual void push(QWidget* top) = 0;
	virtual QWidget* pop() = 0;

protected:
	static UiSession* instance_;
};