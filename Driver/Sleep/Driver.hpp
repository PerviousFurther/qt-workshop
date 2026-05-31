#pragma once

#include "Core/Utils.hpp"
#include "Backend/Device/Driver.hpp"
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtBluetooth/QLowEnergyController>
#include <QtBluetooth/QLowEnergyService>
#include <QQueue>
#include <QMutex>

struct DeviceProfile;
class
#if defined(RINGAPP_DRIVER_SLEEP_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
UartDriver : public Driver {
    Q_OBJECT
public:
    UartDriver(QBluetoothDeviceInfo const& info, DeviceProfile profile, QObject* parent);
    ~UartDriver();

    void read(int serialCode, QVariantMap cmd) override;
    void write(int serialCode, QVariantMap cmd) override;
    bool readable() override { return serviceReady_; }
    bool writeable() override { return serviceReady_; }

    QString id() const override;
    QString getName() const override;
    void setName(QString newName) override;
    QVariantMap metaInfo() const override;
    void release() override;
    QString kind() noexcept override;

private:
    enum class PendingKind { Invalid, Read, Write };
    struct PendingOperation {
        PendingKind kind = PendingKind::Invalid;
        int serialCode = 0;
        QVariantMap cmd;
        QByteArray payload;
    };

    void processQueue();
    void ensureConnected();
    void startWrite(PendingOperation const& operation);
    void startRead(PendingOperation const& operation);
    void dispatchRead(PendingOperation const& operation);
    void handlePostReadTrigger();
    void finishRead(QByteArray const& value);
    void finishWrite(QByteArray const& value);
    void enableLoopbackMode();
    void cleanup();
    bool canRead(QLowEnergyCharacteristic const& characteristic);
    QLowEnergyService::WriteMode writeMode() const;

private slots:
    void handleServiceDiscovered(QBluetoothUuid const& uuid);
    void handleServiceDiscoveryFinished();
    void handleServiceStateChanged(QLowEnergyService::ServiceState state);
    void handleCharacteristicChanged(QLowEnergyCharacteristic const& characteristic, QByteArray const& value);
    void handleCharacteristicRead(QLowEnergyCharacteristic const& characteristic, QByteArray const& value);
    void handleCharacteristicWritten(QLowEnergyCharacteristic const& characteristic, QByteArray const& value);
    void handleDescriptorWritten(QLowEnergyDescriptor const& descriptor, QByteArray const& value);

private:
    QBluetoothDeviceInfo info_;
    QString bleId_;
    QString address_;
    QBluetoothUuid serviceUuid_;
    QBluetoothUuid notifyUuid_;
    QBluetoothUuid writeUuid_;

    QLowEnergyController* controller_ = nullptr;
    QLowEnergyService* service_ = nullptr;
    QLowEnergyCharacteristic readCharacteristic_;
    QLowEnergyCharacteristic writeCharacteristic_;
    QLowEnergyDescriptor notificationDescriptor_;

    QQueue<PendingOperation> pending_;
    PendingOperation current_; // 取代 std::optional<PendingOperation>
    QByteArray lastValue_;

    bool serviceReady_ = false;
    bool pendingServiceDiscovery_ = false;
    bool notificationEnabled_ = false;
    bool waitingForNotificationSetup_ = false;
    bool waitingForTriggeredRead_ = false;
    bool loopbackMode_ = false;
};

class SleepDevicesModule : public DriverModule {
    Q_OBJECT;
public:
    SleepDevicesModule();
    ~SleepDevicesModule();

    bool load(QString& error) override;
    bool unload(QString& error) override;
    void release() noexcept override;
    Driver* create(QString name) override;
    bool active(QString& name, int counter, QBluetoothDeviceInfo const& info) override;
    QStringList devices() override;
    QString kind(QString name) override;

private:
    mutable QMutex lock_;
    struct DiscoveredDevice;
    QHash<QString, DiscoveredDevice> discoveredDevices_;
};