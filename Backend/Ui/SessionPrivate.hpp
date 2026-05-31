#pragma once

#include <QWidget>
#include <QListWidget>
#include <QFontComboBox>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QSignalBlocker>

#include "Session.hpp"

class MainWindow;
class Impl : public UiSession {
	Q_OBJECT;

public:
	Impl();
	~Impl();

	bool load(QString&) override;
	bool unload(QString&) override;
	void release() noexcept override;

	QVariantMap theme() override { return theme_; }
	void setTheme(QVariantMap which) override;

	void setFont(QVariantMap value) override;
	QVariantMap font() override { return font_; }
	QVariantMap app() override { return app_; }
	QList<QVariantMap> themes() override { return themes_; }

	Extensible* main() override;
	void main(Extensible*) override;

	// qsizetype append(QWidget*) override;
	// QWidget* remove(qsizetype) override;

	Extensible* home() override;
	Extensible* settings() override;
	Extensible* searchDevicePage() override;
	Extensible* connectedDevice() override;
	Extensible* deviceRecords() override;

	void push(QWidget*) override;
	QWidget* pop() override;

private:
	QList<QVariantMap> themes_;
	QVariantMap theme_;
	QVariantMap font_;
	QVariantMap app_;
	QList<QWidget*> pages_;
	Extensible* main_ = nullptr;
	MainWindow* mainWin_ = nullptr;
};

class ThemeFontCard : public QWidget {
	Q_OBJECT

public:
	explicit ThemeFontCard(QWidget* parent = nullptr);
	~ThemeFontCard() override;

private slots:
	void syncWithSession();
	void onThemeSelected(QListWidgetItem* item);
	void applyFontChange();

private:
	QListWidget* themeList_ = nullptr;
	QFontComboBox* fontCombo_ = nullptr;
	QSlider* sizeSlider_ = nullptr;
	QLabel* sizeTextLabel_ = nullptr;
};