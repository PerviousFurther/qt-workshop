#pragma once

#include "Core/Utils.hpp"
#include "Core/Module.hpp"

#include "Field.hpp"

class QBluetoothDeviceInfo;
class
#if defined(RINGAPP_BACKEND_DEVICE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
DeviceModule: public Module{
    Q_OBJECT;
public:
    using Module::Module;

signals:
    void deviceConnected(int deviceId);
    void deviceConnectionError(int deviceId, int code, QString msg);

    void writeResponded(int deviceId, int serial, QByteArray command, QByteArray response);
    void readResponded(int deviceId, int serial, QByteArray command, QByteArray response);

public:
    // initialize resource.
    virtual bool activate(int deviceId, QBluetoothDeviceInfo const& bleInfo) = 0;
    // release resources.
    virtual bool deactivate(int deviceId) = 0;

    // make connection.
    virtual bool connectDevice(int deviceId) = 0;
    // release connection.
    virtual bool disconnectDevice(int deviceId) = 0;

    virtual bool read(int id, int serial, QByteArray info) = 0;
    virtual bool write(int id, int serial, QByteArray command) = 0;

    // device id that the driverModule can create.
    // For ble, it is ble identifier name.
    virtual QStringList devices() = 0;
    // activate and get infomation.
    virtual QVariantMap infoOf(int id) = 0;
};