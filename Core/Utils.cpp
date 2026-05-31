#include <QJsonDocument>

#include "Utils.hpp"

#if defined(RINGAPP_CORE_EXPORT)
Q_DECL_EXPORT
#else
Q_DECL_IMPORT
#endif
QVariantMap fromJson(QJsonDocument const& val) {
    return val.toVariant().value<QVariantMap>();
}
