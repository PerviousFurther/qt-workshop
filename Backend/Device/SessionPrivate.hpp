#pragma once

// #include <QMap Played>
#include <QQueue>
#include <QPair>

#include "Session.hpp"

class QBluetoothDeviceDiscoveryAgent;
class DeviceSessionImpl : public DeviceSession {
    Q_OBJECT;
public:
    explicit DeviceSessionImpl();
    ~DeviceSessionImpl() override;

    bool load(QString& error);
    bool unload(QString& error);
    void release() noexcept override;

    bool registerDriverModule(DeviceModule* mod) override;
    bool unregisterDriverModule(DeviceModule* mod) override;

    void searchDevices() override;
    void stopSearchingDevice() override;

    void connectDevice(int deviceId) override;
    void disconnectDevice(int deviceId) override;

    bool read(int deviceId, int serialCode, QByteArray cmd) override;
    bool write(int deviceId, int serialCode, QByteArray cmd) override;

    QStringList connectableDeviceNames() override;

private slots:
    void onBleDeviceDiscovered(const QBluetoothDeviceInfo& bleInfo);
    void onBleDiscoveryFinished();
    void onBleDiscoveryError(int error);

    void onModuleConnected(int id);
    void onModuleConnectionError(int id, int code, QString msg);
    void onModuleReadResponded(int id, int serial, QByteArray cmd, QByteArray response);
    void onModuleWriteResponded(int id, int serial, QByteArray cmd, QByteArray response);

private:
    QList<DeviceModule*> modules_;

    QMap<int, DeviceModule*> deviceToModuleMap_;
    QMap<int, QString> discoveredDevices_;

    QBluetoothDeviceDiscoveryAgent* bleFinder_ = nullptr;

    int nextGeneratedId_ = 1000;

    bool emitError_ = false;
};