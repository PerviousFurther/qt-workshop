#include "Session.hpp"
#include <QJsonObject>
#include <QJsonArray>

namespace NetworkAddress {
    QString subdir(QString address) {
        return '/' + address;
    }
}

QString NetworkSession::moduleName() {
    return lstr(QML_MODULE_NAME);
}
