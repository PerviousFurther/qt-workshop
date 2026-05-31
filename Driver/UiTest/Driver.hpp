#pragma once

#include "Core/Utils.hpp"
#include "Driver/Basic/Driver.hpp"

#include "Field.hpp"

// #include "Driver/History/Driver.hpp"

class Extensible;
class 
#if defined (RINGAPP_DRIVER_UI_TEST_EXPORT)
	Q_DECL_EXPORT
#else
	Q_DECL_IMPORT
#endif
UITestModule : public BasicDeviceModule {
	Q_OBJECT;
public:
	UITestModule();

signals:
	void openingDetailPage(int id);
	void requestMeasure(int id);

public:
	static auto instance() { return instance_; }

	bool load(QString& error) override;
	bool unload(QString& error) override;

	bool activate(int id, QBluetoothDeviceInfo const&) override;
	bool deactivate(int id) override;

	bool connectDevice(int id) override;
	bool disconnectDevice(int id) override;

	bool read(int id, int serialCode, QByteArray arr) override;
	bool write(int id, int serialCode, QByteArray arr) override;

	void release() noexcept override;
	QStringList devices() override;
	QVariantMap infoOf(int id) override;

	Extensible* detailPage();

public slots:
	void openDetailPage(int id);
	void onTimerTimeout();

private:
	inline static UITestModule* instance_ = nullptr;

	int deviceId_ = -1;
	// QWidget *searched = nullptr, *connected = nullptr, *records = nullptr;
	Extensible* detailPage_ = nullptr;
	qsizetype searchId_ = -1, connectedId_ = -1, historyId_ = -1;

	QTimer* simTimer_ = nullptr;
	QMap<int, QByteArray> channelBuffers_;
	quint64 sampleIndex_ = 0;
	quint8 enabledChannels_ = 0;
};