#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include "Driver.hpp"
#include "SessionPrivate.hpp"

QString DeviceSession::moduleName() {
	return lstr(QML_MODULE_NAME);
}

DeviceSessionImpl::DeviceSessionImpl() 
    : DeviceSession(QML_MODULE_NAME, QML_MODULE_MAJOR, QML_MODULE_MINOR) {

    bleFinder_ = new QBluetoothDeviceDiscoveryAgent(this);
    connect(bleFinder_, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
        this, &DeviceSessionImpl::onBleDeviceDiscovered);
    connect(bleFinder_, &QBluetoothDeviceDiscoveryAgent::finished,
        this, &DeviceSessionImpl::onBleDiscoveryFinished);
    connect(bleFinder_, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::errorOccurred),
        this, [this](QBluetoothDeviceDiscoveryAgent::Error error) {
            this->onBleDiscoveryError(static_cast<int>(error));
        });
    instance_ = this;
}

DeviceSessionImpl::~DeviceSessionImpl() {
    QString _; unload(_);
}

bool DeviceSessionImpl::load(QString& error) {
    return true;
}

bool DeviceSessionImpl::unload(QString& error) {
    for (auto [id, mod] : this->deviceToModuleMap_.asKeyValueRange()) 
        mod->disconnectDevice(id);
    this->deviceToModuleMap_.clear();
    for (auto mod : this->modules_)
        mod->release();
    this->modules_.clear();
    return true;
}

void DeviceSessionImpl::release() noexcept {
    delete this;
}

bool DeviceSessionImpl::registerDriverModule(DeviceModule* mod)
{
    if (!mod || modules_.contains(mod)) {
        return false;
    }

    connect(mod, &DeviceModule::deviceConnected, this, &DeviceSessionImpl::onModuleConnected);
    connect(mod, &DeviceModule::deviceConnectionError, this, &DeviceSessionImpl::onModuleConnectionError);
    connect(mod, &DeviceModule::readResponded, this, &DeviceSessionImpl::onModuleReadResponded);
    connect(mod, &DeviceModule::writeResponded, this, &DeviceSessionImpl::onModuleWriteResponded);

    modules_.append(mod);
    return true;
}

bool DeviceSessionImpl::unregisterDriverModule(DeviceModule* mod) {
    if (!mod || !modules_.contains(mod)) {
        qCritical() << "[Device ERROR] unregister invalid module.";
        return false;
    }

    for (auto it = deviceToModuleMap_.begin(); it != deviceToModuleMap_.end(); ) {
        if (it.value() == mod) {
            emit this->deviceConnectionError(it.key(), DeviceField::Error_ModuleUnloaded, lstr("驱动已经卸载。"));
            discoveredDevices_.remove(it.key());
            mod->disconnectDevice(it.key());
            it = deviceToModuleMap_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = modules_.begin(); it != modules_.end(); it++)
        if (*it == mod) {
            modules_.erase(it);
            break;
        }

    return true;
}

void DeviceSessionImpl::searchDevices() {
    if (bleFinder_->isActive()) bleFinder_->stop();
    discoveredDevices_.clear();
    bleFinder_->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

    // open hole for ui test.
    constexpr auto UITEST_Hole_Id = -1;
    for (auto mod : this->modules_) {
        if (mod->activate(UITEST_Hole_Id, QBluetoothDeviceInfo())) {
            qInfo() << "[Device Message] Open hole success from:" << mod->moduleName();
            this->deviceToModuleMap_[UITEST_Hole_Id] = mod;
            break;
        }
    }
}

void DeviceSessionImpl::stopSearchingDevice(){
    if (bleFinder_->isActive()) bleFinder_->stop();
    emit this->deviceDiscoverError(DeviceField::Error_StopManually, lstr("已停止搜索"));
}

void DeviceSessionImpl::connectDevice(int deviceId) {
    auto* mod = deviceToModuleMap_.value(deviceId);
    if (mod) {
        emit deviceConnecting(deviceId);
        mod->connectDevice(deviceId);
    } else {
        qCritical() << "[Device Error] device Id '" << deviceId << "' is not found.";
    }
}

void DeviceSessionImpl::disconnectDevice(int deviceId) {
    auto* mod = deviceToModuleMap_.value(deviceId);
    if (mod) {
        mod->disconnectDevice(deviceId);
    }
}

bool DeviceSessionImpl::read(int deviceId, int serialCode, QByteArray cmd) {
    auto* mod = deviceToModuleMap_.value(deviceId);
    if (!mod) {
        qCritical() << "[Device Error] device id is not contain:" << deviceId << "read operation is aborted.";
        return false;
    } else {
        return mod->read(deviceId, serialCode, cmd);
    }
}

bool DeviceSessionImpl::write(int deviceId, int serialCode, QByteArray cmd) {
    auto* mod = deviceToModuleMap_.value(deviceId);
    if (!mod) {
        qCritical() << "[Device Error] device id is not contain:" << deviceId << "write operation is aborted.";
        return false;
    } else {
        return mod->write(deviceId, serialCode, cmd);
    }
}

QStringList DeviceSessionImpl::connectableDeviceNames() {
    if (this->bleFinder_->isActive()) // active might not thread safe.
        return {};
    else
        return discoveredDevices_.values();
}

// ==================== Slots ====================

void DeviceSessionImpl::onBleDeviceDiscovered(const QBluetoothDeviceInfo& bleInfo) {
    for (int i = modules_.size() - 1; i >= 0; --i) {
        auto* mod = modules_.value(i);
        int assignedId = mod->activate(nextGeneratedId_, bleInfo);
        if (assignedId != -1) {
            if (assignedId == nextGeneratedId_) {
                nextGeneratedId_++;
            }

            deviceToModuleMap_[assignedId] = mod;

            QVariantMap info = mod->infoOf(assignedId);
            QString devName = info.value(DeviceField::Info_Name).toString();
            if (devName.isEmpty()) 
                devName = bleInfo.name();

            discoveredDevices_[assignedId] = devName;

            emit deviceDiscovered(assignedId, info);
            break;
        }
    }
}

void DeviceSessionImpl::onBleDiscoveryFinished() {
    emit finishDeviceDiscovered(); // maybe more thing, whatever.
}

void DeviceSessionImpl::onBleDiscoveryError(int error) {
    switch (error) {
    case QBluetoothDeviceDiscoveryAgent::Error::PoweredOffError:
        emit deviceDiscoverError(400 + error, lstr("蓝牙已关闭，请打开蓝牙以搜索设备"));
        break;
    default:
        emit deviceDiscoverError(400 + error, lstr("未知错误，请稍后再试"));
        break;
    }
}

void DeviceSessionImpl::onModuleConnected(int id) {
    emit deviceConnected(id);
    // emit this->deviceToModuleMap_.value(id)->deviceConnected(id);
}

void DeviceSessionImpl::onModuleConnectionError(int id, int code, QString msg) {
    emit deviceConnectionError(id, code, msg);
    // emit this->deviceToModuleMap_.value(id)->deviceConnectionError(id, code, msg);
}

void DeviceSessionImpl::onModuleReadResponded(int id, int serial, QByteArray cmd, QByteArray response) {
    emit readResponded(id, serial, cmd, response);
    // emit this->deviceToModuleMap_.value(id)->readResponded(id, serial, cmd, response);
}

void DeviceSessionImpl::onModuleWriteResponded(int id, int serial, QByteArray cmd, QByteArray response) {
    emit writeResponded(id, serial, cmd, response);
    // emit this->deviceToModuleMap_.value(id)->writeResponded(id, serial, cmd, response);
}

// customized MODULE entry 

MODULE_ENTRY(Q_DECL_EXPORT)() {
    return new DeviceSessionImpl();
}
