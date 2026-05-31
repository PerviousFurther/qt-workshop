#pragma once 

#include "Backend/Device/Driver.hpp"
#include <QMutex>
#include <QHash>
#include <QByteArray>
#include <QString>

class NetworkRespone;

// class offer file operation, configuration's write and network upload operation.
class 
#if defined(RINGAPP_DRIVER_BASIC_EXPORT)
	Q_DECL_EXPORT
#else
	Q_DECL_IMPORT
#endif
BasicDeviceModule : public DeviceModule 
{
	Q_OBJECT;
public:
	using DeviceModule::DeviceModule;

signals:
	void fileSaved(QString identifier);
	void fileRead(QString identifier, QByteArray data);
	void fileError(QString identifier, QString error);

	void uploaded(QString identifier);
	void downloaded(QString identifier, QByteArray data);
	// Network error.
	void errorOccured(QString identifier, QString error);

	void canceled(QString identifier);

public:
	// operation requires the identifier is unique.
	// append record data into configuration, local file and upload it to cloud if have.
	bool writeRecordFile(QString identifier, QString filename, QByteArray rawData);
	// return false if the id is not marked.
	bool readRecordFile(QString identifier);

	bool load(QString& error) override;
	bool unload(QString& error) override;

private slots:
	void onUserChanged();
	void onRequested(NetworkRespone* respone);

private:
	struct TaskInfo {
		QString identifier;
		QString filename;
		QByteArray data;
		NetworkRespone* response = nullptr;

		bool cancelRequested = false;
	};

	void startUpload(const QString& identifier);

	QMutex mutex_;
	QHash<QString, TaskInfo> tasks_;
	QHash<NetworkRespone*, QString> responseToId_;
};