#include <QUuid>
#include <QFileInfo>
#include <QMutexLocker>

#include "Core/Utils.hpp"
#include "Driver.hpp"
#include "Field.hpp"

namespace DriverField {
    using namespace Sleep;
}
namespace Sleep = DriverField::Sleep;

struct DeviceProfile {
    QString bleId;
    QBluetoothUuid serviceUuid;
    QBluetoothUuid notifyUuid;
    QBluetoothUuid writeUuid;
};
struct SleepDevicesModule::DiscoveredDevice {
    QBluetoothDeviceInfo info;
    DeviceProfile profile;
};

// ==========================================
// 辅助解析函数
// ==========================================

QString deviceAddressText(QBluetoothDeviceInfo const& info) {
    const auto address = info.address().toString();
    if (!address.isEmpty()) return address;

    const auto uuid = info.deviceUuid().toString(QUuid::WithoutBraces);
    if (!uuid.isEmpty()) return uuid;

    qCritical() << "[Driver.Sleep ERROR] unidentified driver address.";
    return lstr();
}

bool resolveProfile(QBluetoothDeviceInfo const& info, DeviceProfile& profile) {
    const auto name = info.name().trimmed();

    if (name.compare(DriverField::Device_SmartRing, Qt::CaseInsensitive) == 0) {
        profile = {
            DriverField::Profile_BleAtk,
            QBluetoothUuid(lstr("9ecadc24-0ee5-a9e0-93f3-a3b50100406e")),
            QBluetoothUuid(lstr("9ecadc24-0ee5-a9e0-93f3-a3b50300406e")),
            QBluetoothUuid(lstr("9ecadc24-0ee5-a9e0-93f3-a3b50200406e"))
        };
        return true;
    }

    if (name.compare(DriverField::Device_NordicUart, Qt::CaseInsensitive) == 0) {
        profile = {
            DriverField::Profile_NordicZc,
            QBluetoothUuid(lstr("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")),
            QBluetoothUuid(lstr("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")),
            QBluetoothUuid(lstr("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"))
        };
        return true;
    }

    return false;
}


// ==========================================
// UartDriver
// ==========================================

UartDriver::UartDriver(QBluetoothDeviceInfo const& info, DeviceProfile profile, QObject* parent)
    : Driver(parent)
    , info_(info)
    , bleId_(std::move(profile.bleId))
    , address_(deviceAddressText(info))
    , serviceUuid_(std::move(profile.serviceUuid))
    , notifyUuid_(std::move(profile.notifyUuid))
    , writeUuid_(std::move(profile.writeUuid)) {

    controller_ = QLowEnergyController::createCentral(info, this);
    if (!controller_) {
        enableLoopbackMode();
        return;
    }

    connect(controller_, &QLowEnergyController::connected, this, [this]() {
        pendingServiceDiscovery_ = true;
        controller_->discoverServices();
        });
    connect(controller_, &QLowEnergyController::disconnected, this, [this]() {
        serviceReady_ = false;
        pendingServiceDiscovery_ = false;
        service_ = nullptr;
        });
    connect(controller_, &QLowEnergyController::serviceDiscovered, this, &UartDriver::handleServiceDiscovered);
    connect(controller_, &QLowEnergyController::discoveryFinished, this, &UartDriver::handleServiceDiscoveryFinished);
}

UartDriver::~UartDriver() {
    cleanup();
    emit this->connectionError(0, lstr("设备 %1 已经断开连接").arg(this->getName()));
}

void UartDriver::read(int serialCode, QVariantMap cmd) {
    if (serialCode < 0)
        qCritical("SerialCode less than zero is not allowed, the behavior is undefined.");

    QByteArray bytes = cmd.value(Sleep::Command::Bytes).toByteArray();
    pending_.enqueue({ PendingKind::Read, serialCode, cmd, bytes });
    processQueue();
}

void UartDriver::write(int serialCode, QVariantMap cmd) {
    if (serialCode < 0)
        qCritical("SerialCode less than zero is not allowed, the behavior is undefined.");

    QByteArray bytes = cmd.value(Sleep::Command::Bytes).toByteArray();
    pending_.enqueue({ PendingKind::Write, serialCode, cmd, bytes });
    processQueue();
}

QString UartDriver::id() const {
    return address_;
}

QString UartDriver::getName() const {
    return info_.name();
}

void UartDriver::setName(QString) {
    qCritical("Uart driver cannot set name.");
}

QVariantMap UartDriver::metaInfo() const {
    return QVariantMap{
        { Sleep::Response::Id, address_ },
        { Sleep::Response::Kind, bleId_ }
    };
}

void UartDriver::release() {
    delete this;
}

bool UartDriver::canRead(QLowEnergyCharacteristic const& characteristic) {
    return characteristic.properties().testFlag(QLowEnergyCharacteristic::Read);
}

QLowEnergyService::WriteMode UartDriver::writeMode() const {
    return writeCharacteristic_.properties().testFlag(QLowEnergyCharacteristic::WriteNoResponse)
        ? QLowEnergyService::WriteWithoutResponse
        : QLowEnergyService::WriteWithResponse;
}

void UartDriver::enableLoopbackMode() {
    loopbackMode_ = true;
    serviceReady_ = true;
}

void UartDriver::cleanup() {
    if (service_) {
        disconnect(service_, nullptr, this, nullptr);
        service_->deleteLater();
        service_ = nullptr;
    }
    if (controller_) {
        disconnect(controller_, nullptr, this, nullptr);
        if (controller_->state() != QLowEnergyController::UnconnectedState) {
            controller_->disconnectFromDevice();
        }
        controller_ = nullptr;
    }
}

void UartDriver::processQueue() {
    if (current_.kind != PendingKind::Invalid || pending_.isEmpty()) return;

    if (!serviceReady_) {
        ensureConnected();
        return;
    }

    current_ = pending_.dequeue();
    if (current_.kind == PendingKind::Write) {
        startWrite(current_);
        return;
    }

    startRead(current_);
}

void UartDriver::ensureConnected() {
    if (loopbackMode_) {
        serviceReady_ = true;
        processQueue();
        return;
    }
    if (!controller_) {
        enableLoopbackMode();
        processQueue();
        return;
    }

    switch (controller_->state()) {
    case QLowEnergyController::ConnectedState:
        if (!pendingServiceDiscovery_) {
            pendingServiceDiscovery_ = true;
            controller_->discoverServices();
        }
        break;
    case QLowEnergyController::ConnectingState:
    case QLowEnergyController::DiscoveringState:
        break;
    default:
        controller_->connectToDevice();
        break;
    }
}

void UartDriver::handleServiceDiscovered(QBluetoothUuid const& uuid) {
    if (!controller_ || service_ || uuid != serviceUuid_) return;

    service_ = controller_->createServiceObject(serviceUuid_, this);
    if (!service_) return;

    connect(service_, &QLowEnergyService::stateChanged, this, &UartDriver::handleServiceStateChanged);
    connect(service_, &QLowEnergyService::characteristicChanged, this, &UartDriver::handleCharacteristicChanged);
    connect(service_, &QLowEnergyService::characteristicRead, this, &UartDriver::handleCharacteristicRead);
    connect(service_, &QLowEnergyService::characteristicWritten, this, &UartDriver::handleCharacteristicWritten);
    connect(service_, &QLowEnergyService::descriptorWritten, this, &UartDriver::handleDescriptorWritten);

    if (service_->state() == QLowEnergyService::DiscoveryRequired) {
        service_->discoverDetails();
    }
}

void UartDriver::handleServiceDiscoveryFinished() {
    pendingServiceDiscovery_ = false;
    if (!service_) {
        enableLoopbackMode();
    }
    processQueue();
}

void UartDriver::handleServiceStateChanged(QLowEnergyService::ServiceState state) {
    if (!service_ || state != QLowEnergyService::RemoteServiceDiscovered) return;

    readCharacteristic_ = service_->characteristic(notifyUuid_);
    writeCharacteristic_ = service_->characteristic(writeUuid_);
    notificationDescriptor_ = readCharacteristic_.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

    if (!readCharacteristic_.isValid() && !writeCharacteristic_.isValid()) {
        enableLoopbackMode();
    }
    serviceReady_ = true;
    processQueue();
}

void UartDriver::startWrite(PendingOperation const& operation) {
    if (loopbackMode_ || !service_ || !writeCharacteristic_.isValid()) {
        lastValue_ = operation.payload;
        finishWrite(operation.payload);
        return;
    }

    const auto mode = writeMode();
    service_->writeCharacteristic(writeCharacteristic_, operation.payload, mode);
    if (mode == QLowEnergyService::WriteWithoutResponse) {
        lastValue_ = operation.payload;
        finishWrite(operation.payload);
    }
}

void UartDriver::startRead(PendingOperation const& operation) {
    if (loopbackMode_) {
        finishRead(operation.payload.isEmpty() ? lastValue_ : operation.payload);
        return;
    }
    if (!service_) {
        finishRead(lastValue_);
        return;
    }

    if (!notificationEnabled_ && notificationDescriptor_.isValid()) {
        waitingForNotificationSetup_ = true;
        service_->writeDescriptor(notificationDescriptor_, QLowEnergyCharacteristic::CCCDEnableNotification);
        return;
    }

    dispatchRead(operation);
}

void UartDriver::dispatchRead(PendingOperation const& operation) {
    if (!operation.payload.isEmpty() && service_ && writeCharacteristic_.isValid()) {
        waitingForTriggeredRead_ = true;
        const auto mode = writeMode();
        service_->writeCharacteristic(writeCharacteristic_, operation.payload, mode);
        if (mode == QLowEnergyService::WriteWithoutResponse) {
            handlePostReadTrigger();
        }
        return;
    }

    if (service_ && readCharacteristic_.isValid() && canRead(readCharacteristic_)) {
        service_->readCharacteristic(readCharacteristic_);
        return;
    }

    finishRead(lastValue_);
}

void UartDriver::handlePostReadTrigger() {
    waitingForTriggeredRead_ = false;
    if (service_ && readCharacteristic_.isValid() && canRead(readCharacteristic_)) {
        service_->readCharacteristic(readCharacteristic_);
        return;
    }
    finishRead(lastValue_);
}

void UartDriver::handleCharacteristicChanged(QLowEnergyCharacteristic const& characteristic, QByteArray const& value) {
    if (characteristic != readCharacteristic_) return;

    lastValue_ = value;
    if (current_.kind == PendingKind::Read) {
        finishRead(value);
    }
}

void UartDriver::handleCharacteristicRead(QLowEnergyCharacteristic const& characteristic, QByteArray const& value) {
    if (characteristic != readCharacteristic_) return;

    lastValue_ = value;
    if (current_.kind == PendingKind::Read) {
        finishRead(value);
    }
}

void UartDriver::handleCharacteristicWritten(QLowEnergyCharacteristic const& characteristic, QByteArray const& value) {
    if (current_.kind == PendingKind::Invalid || characteristic != writeCharacteristic_) return;

    if (current_.kind == PendingKind::Write) {
        lastValue_ = value.isEmpty() ? current_.payload : value;
        finishWrite(lastValue_);
        return;
    }

    if (waitingForTriggeredRead_) {
        handlePostReadTrigger();
    }
}

void UartDriver::handleDescriptorWritten(QLowEnergyDescriptor const& descriptor, QByteArray const& value) {
    if (descriptor != notificationDescriptor_) return;

    notificationEnabled_ = value == QLowEnergyCharacteristic::CCCDEnableNotification;
    if (!waitingForNotificationSetup_ || current_.kind == PendingKind::Invalid) return;

    waitingForNotificationSetup_ = false;
    dispatchRead(current_);
}

void UartDriver::finishRead(QByteArray const& value) {
    if (current_.kind != PendingKind::Invalid) {
        QVariantMap responseMaps;
        responseMaps[DriverField::Response::Code] = 0;
        responseMaps[Sleep::Response::Bytes] = QVariant(value);
        emit this->readResponded(current_.serialCode, responseMaps);
    }

    current_ = PendingOperation();
    waitingForNotificationSetup_ = false;
    waitingForTriggeredRead_ = false;
    processQueue();
}

void UartDriver::finishWrite(QByteArray const& value) {
    if (current_.kind != PendingKind::Invalid) {
        QVariantMap responseMaps;
        responseMaps[DriverField::Response::Code] = 0;
        responseMaps[Sleep::Response::Bytes] = QVariant(value);
        emit this->writeResponded(current_.serialCode, responseMaps);
    }

    current_ = PendingOperation();
    waitingForNotificationSetup_ = false;
    waitingForTriggeredRead_ = false;
    processQueue();
}

QString UartDriver::kind() noexcept {
    return DriverField::Kind_BLE;
}


// ==========================================
// SleepDevicesModule 模块层具体实现
// ==========================================

SleepDevicesModule::SleepDevicesModule() 
    : DriverModule(lstr(QML_MODULE_NAME), QML_MODULE_MAJOR, QML_MODULE_MINOR) {
}

SleepDevicesModule::~SleepDevicesModule() {
}

bool SleepDevicesModule::load(QString&) {
    return true;
}

bool SleepDevicesModule::unload(QString& error) {
    return true;
}

void SleepDevicesModule::release() noexcept {
    delete this;
}

bool SleepDevicesModule::active(QString& name, int counter, QBluetoothDeviceInfo const& info) {
    DeviceProfile profile;
    if (!resolveProfile(info, profile)) {
        return false;
    }

    QMutexLocker locker(&this->lock_);
    name = deviceAddressText(info);
    this->discoveredDevices_[name] = { info, profile };
    return true;
}

Driver* SleepDevicesModule::create(QString name) {
    QMutexLocker locker(&this->lock_);
    if (this->discoveredDevices_.contains(name)) {
        auto const& dev = this->discoveredDevices_[name];
        return new UartDriver(dev.info, dev.profile, this);
    }
    qCritical() << lstr("无法创建未注册的目标设备: %1").arg(name);
    return nullptr;
}

QStringList SleepDevicesModule::devices() {
    QMutexLocker locker(&this->lock_);
    return this->discoveredDevices_.keys();
}

QString SleepDevicesModule::kind(QString name) {
    return DriverField::Kind_BLE;
}

// ==========================================
// 采用底层的通用宏进行模块导出
// ==========================================

MODULE_ENTRY(Q_DECL_EXPORT)() {
    return new SleepDevicesModule();
}