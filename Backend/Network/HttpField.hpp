#include "Core/Utils.hpp"

namespace HttpField {
    /**
     * @brief Container for HTTP header information.
     * Type: QVariantMap
     * Requirement: Optional.
     */
    inline const QString HEADER = lstr("header");

    /**
     * @brief Classification of the header entry.
     * Key: "type" (int)
     * Values: 2 (Http), 3 (Customized)
     */
    inline const QString HEADER_Type = lstr("type");

    inline const auto HEADER_Type_KnownHeader = 1;
    inline const auto HEADER_Type_Http = 2;
    inline const auto HEADER_Type_Customized = 3;

    /**
     * @brief The actual string value of the header.
     * Type: QString
     */
    inline const QString HEADER_Value = lstr("value");

    /**
     * @brief Internal data identifier for headers.
     * Type: QByteArray (HEADER_Type_Customized or HEADER_Type_Http use for key of the value) or int
     */
    inline const QString HEADER_Type_Data = lstr("^d");

    /* Standard Header Keys for HEADER_Value of HEADER_Type_Http (QString) */
    inline const QString HEADER_ACCEPT = lstr("Accept");
    inline const QString HEADER_CONTENT_TYPE = lstr("Content-Type");
    inline const QString HEADER_COOKIE = lstr("Cookie");

    // `headerObject` should have following structure:
    //  example: [ {
    //      HEADER_Type: int (HEADER_Type_*)
    //      HEADER_Type_Data: if HEADER_Type_KnownHeader then the enum QNetworkRequest::KnownHeader else QByteArray.
    //      HEADER_Value: QString
    // }, ...]

    /**
     * @brief Main data payload.
     * Type: QVariantMap (if JSON) or QByteArray (if raw data)
     * Requirement: Mandatory for successful responses containing a body.
     */
    inline const QString DATA = lstr("data");

    /**
     * @brief Specifies the structure/format of the DATA field.
     * Key: "^kind" (int)
     */
    inline const int DATA_Kind_Mixed = 1;
    inline const int DATA_Kind_Related = 2;
    inline const int DATA_Kind_FormData = 3;
    inline const int DATA_Kind_Alterated = 4;
    inline const int DATA_Kind_Json = 0; // Default JSON format

    inline const QString DATA_Kind = lstr("^kind");

    /* Nested Data Structure Constants */
    inline const QString MULTIDATA = DATA;
    inline const QString MULTIDATA_Header = HEADER;
    inline const QString MULTIDATA_Data = DATA;

    /**
     * @brief Progress Tracking (Automatically populated by HttpResponse)
     * ProgressSize (^ps): Bytes currently received (qint64)
     * TotalSize (^ts): Total expected bytes (qint64)
     */
    inline const QString ProgressSize = lstr("^ps");
    inline const QString TotalSize = lstr("^ts");
}

namespace ResponseCode {
    inline const int ProtocolError = 425; // Custom protocol error code
}

namespace ResponseString {
    inline const QString Success = lstr("success");
    inline const QString ClientOvertime = lstr("client overtime");
    inline const QString UnknownErr = lstr("Unknown error.");
}