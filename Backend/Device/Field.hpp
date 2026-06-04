#pragma once

// Some common usage things.
namespace DeviceField {
    inline const auto Info_Id = lstr("i");
    inline const auto Info_Name = lstr("n");
    inline const auto Info_Address = lstr("a");

    inline const auto Kind_BLE = lstr("ble");
    inline const auto Kind_Serial = lstr("serial");
    inline const auto Kind = lstr("kind");

    inline const auto Error_Disconnected = 1;
    inline const auto Error_ModuleUnloaded = Error_Disconnected | 0x2;
    inline const auto Error_DeviceManagerUnloaded = Error_Disconnected | 0x4;
    inline const auto Error_RemoteClosed = Error_Disconnected | 0x8;
    inline const auto Error_RemoteMissing = Error_Disconnected | 0x10;
    inline const auto Error_WriteFailure = 0x100;
    inline const auto Error_ReadFailure = 0x200;
    inline const auto Error_StopManually = Error_Disconnected | 0x1000;

    inline const auto Name = lstr("^n");

    // for read or write result.
    namespace Response {
        inline const auto Code = lstr("^c");
        inline const auto Message = lstr("^m");
    }
}
