#pragma once

#include "Core/Utils.hpp"
#include "Core/Module.hpp"

#include "Field.hpp"

class DeviceModule;
class QBluetoothDeviceInfo;
class
#if defined(RINGAPP_BACKEND_DEVICE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
DeviceSession : public Module {
    Q_OBJECT;
public:
    static QString moduleName();

    using Module::Module;

signals:
    void finishDeviceDiscovered(); 

    void deviceDiscovered(int deviceId, QVariantMap info);
    void deviceDiscoverError(int errorCode, QString msg);

    // On design, following operation should listen at one of DeviceSession or concrete DeviceModule.
    // else the signal will received twice, causing unexcepted behavior.

    void deviceConnecting(int deviceId);
    void deviceConnected(int deviceId);
    void deviceConnectionError(int deviceId, int errorCode, QString msg);
    
    void readResponded(int deviceId, int serialCode, QByteArray command, QByteArray result);
    void writeResponded(int deviceId, int serialCode, QByteArray command, QByteArray result);

public:
    static auto instance() noexcept { return instance_; }

    // Not thread safe.
    // Noticed that the DriverModule might have multiple module support.
    // return false if the module is registered.
    // notice that the module register at later will take place of fronter.
    virtual bool registerDriverModule(DeviceModule* mod) = 0;
    // Not thread safe.
    virtual bool unregisterDriverModule(DeviceModule* mod) = 0;

    virtual void searchDevices() = 0;
    virtual void stopSearchingDevice() = 0;
    virtual void connectDevice(int deviceId) = 0;
    virtual void disconnectDevice(int deviceId) = 0;
    
    // It should transfer to device method.

    virtual bool read(int deviceId, int serial, QByteArray data) = 0;
    virtual bool write(int deviceId, int serial, QByteArray data) = 0;
   
    // should call after finishDeviceDiscovered.
    virtual QStringList connectableDeviceNames() = 0;

protected:
    inline static DeviceSession* instance_ = nullptr;
};

