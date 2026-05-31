#include "Driver/History/Driver.hpp"

namespace DriverField::Sleep {
    // 设备类型与固定文本
    // const QString Kind_BLE = lstr("BLE");
    // const QString Kind_Medical = lstr("medical");
    // const QString MockConnectId = lstr("MOCK-CONNECT-0001");

    // 目标蓝牙设备广播名称
    const QString Device_SmartRing = lstr("Smart-Ring");
    const QString Device_NordicUart = lstr("Nordic_UART");

    // 蓝牙芯片/Profile标识符
    const QString Profile_BleAtk = lstr("BLE-ATK02");
    const QString Profile_NordicZc = lstr("Nordic-Zc");

    // 指令相关字段
    namespace Command {
        const QString Bytes = lstr("bytes");
    }

    // 结果/响应相关字段
    namespace Response {
        const QString Id = lstr("id");
        const QString Kind = lstr("kind");
        const QString Code = lstr("code");
        const QString Bytes = lstr("bytes");
    }
}