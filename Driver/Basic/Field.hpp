#pragma once

#include <QString>
#include <QRegularExpression>

#include "Core/Utils.hpp"


namespace ResponseCode {
    inline constexpr int Success = 200;
    inline constexpr int Forbidden = 403;
    inline constexpr int InternalError = 500;
}

namespace DriverField {
    inline const auto Id = lstr("id");
    inline const auto Name = lstr("name");
    inline const auto Kind_History = lstr("history");

    namespace History {
        inline const QString ModuleName = QML_MODULE_NAME;

        namespace Command {
            inline const auto Keys = lstr("keys");
            inline const auto SaveFilename = lstr("saveFilename");
            inline const auto Meta = lstr("meta");
            inline const auto Meta_Bytes = lstr("metaBytes");
            inline const auto Meta_ByteFilename = lstr("metaByteFilename");
            inline const auto Kind = lstr("kind");

            inline constexpr int Read_Data = 0x01;
            inline constexpr int Read_Records = 0x02;
        }
    }
}

namespace Field {
    inline const auto BinDirectory = lstr("binDir");
    inline const auto CacheDirectory = lstr("cacheDir");

    namespace History {
        namespace Serial {
            inline constexpr int GetData = 1;
            inline constexpr int GetRecords = 2;
            inline constexpr int ChangeName = 3;
        }
    }

    namespace Response {
        inline const auto Code = lstr("code");
        inline const auto Message = lstr("message");
    }
}

namespace NetworkField {
    inline const auto CODE = lstr("code");
    inline const auto DATA = lstr("data");
}

namespace HistoryField {
    inline const auto HISTORIES = lstr("histories");
}