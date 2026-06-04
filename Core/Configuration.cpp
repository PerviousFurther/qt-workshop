#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QReadWriteLock>
// #include <qqml.h>
#include <QDir>

#include "Configuration.hpp"
#include "Utils.hpp"

#define MODULE_NAME "Configuration"

namespace Field {
    static constexpr auto MemberSplit = QLatin1Char('.');

#if defined(RINGAPP_CORE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
    QString member(QString name) {
        return MemberSplit + name;
    }
}

static const auto LocalDir = lstr("Local");

static const QVariantMap DefaultConfiguration = {
    {Field::BinDirectory, "bin"},
    {Field::CacheDirectory, "cache"},
};

static QVariant variantGet(const QVariant& container, const QString& path) {
    if (container.typeId() != QMetaType::QVariantMap) {
        return {};
    }

    const QVariantMap map = container.toMap();
    if (map.contains(path)) {
        return map.value(path);
    }

    for (qsizetype dotIndex = path.indexOf(Field::MemberSplit);
        dotIndex != -1;
        dotIndex = path.indexOf(Field::MemberSplit, dotIndex + 1)) {

        QString prefix = path.left(dotIndex);
        QVariant nested = map.value(prefix);

        if (nested.typeId() == QMetaType::QVariantMap) {
            QVariant result = variantGet(nested, path.mid(dotIndex + 1));
            if (result.isValid()) return result;
        }
    }
    return {};
}

using SetResult = Configuration::SetResult;
static SetResult variantSet(QVariant& container, const QString& path, const QVariant& value) {
    if (container.typeId() != QMetaType::QVariantMap) {
        return Configuration::SetError;
    }

    QVariantMap map = container.toMap();
    SetResult res = Configuration::SetError;

    if (map.contains(path)) {
        map[path] = value;
        container = map;
        return Configuration::SetAssigned;
    }

    qsizetype dotIndex = path.indexOf(Field::MemberSplit);
    bool hasDot = (dotIndex != -1);

    for (; dotIndex != -1; dotIndex = path.indexOf(Field::MemberSplit, dotIndex + 1)) {
        QString prefix = path.left(dotIndex);
        if (map.contains(prefix) && map[prefix].typeId() == QMetaType::QVariantMap) {
            QVariant nested = map[prefix];
            res = variantSet(nested, path.mid(dotIndex + 1), value);
            if (res != Configuration::SetError) {
                map[prefix] = nested;
                container = map;
                return res;
            }
        }
    }

    map.insert(path, value);
    container = map;
    return hasDot ? Configuration::SetCreateWithDot : Configuration::SetCreateDirect;
}

static bool variantPatchObject(QVariantMap& target, const QVariantMap& patch) {
    bool patched = false;
    for (auto it = patch.begin(); it != patch.end(); ++it) {
        if (target.contains(it.key())) {
            target[it.key()] = it.value();
            patched = true;
        }
    }
    return patched;
}

static bool variantPatch(QVariant& container, const QString& path, const QVariantMap& patchData) {
    QVariantMap map;
    if (!assign(map, container)) return false;
    if (path.isEmpty()) {
        bool ok = variantPatchObject(map, patchData);
        if (ok) container = map;
        return ok;
    }

    if (map.contains(path) && map[path].typeId() == QMetaType::QVariantMap) {
        QVariantMap subMap = map[path].toMap();
        if (variantPatchObject(subMap, patchData)) {
            map[path] = subMap;
            container = map;
            return true;
        }
    }

    for (qsizetype dotIndex = path.indexOf(Field::MemberSplit);
        dotIndex != -1;
        dotIndex = path.indexOf(Field::MemberSplit, dotIndex + 1)) {

        QString prefix = path.left(dotIndex);
        if (map.contains(prefix) && map[prefix].typeId() == QMetaType::QVariantMap) {
            QVariant nested = map[prefix];
            if (variantPatch(nested, path.mid(dotIndex + 1), patchData)) {
                map[prefix] = nested;
                container = map;
                return true;
            }
        }
    }
    return false;
}

Configuration::Configuration(QString path, QObject* parent)
    : QObject{parent}
    , path_{path.trimmed()} {
    Q_ASSERT(!singletonInstance_);
    Q_ASSERT(!path_.isEmpty());

    bool loaded = false;
    if (QFile file(path_); file.open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::ExistingOnly)) {
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            document_ = doc.toVariant().toMap();
            loaded = true;
        } else {
            FASTFAIL(lstr("JSON Parse Error: %1").arg(parseError.errorString()));
        }
    }
    if (!loaded) {
        document_ = DefaultConfiguration;
    }

    singletonInstance_ = this;
    // SingletonInQml(MODULE_NAME, this);
}

Configuration::~Configuration() {
    save();
}

void Configuration::save() {
    if (auto idx = path_.lastIndexOf('/'), ridx = path_.lastIndexOf('\\'); idx != -1 || ridx != -1) {
        idx = idx > ridx ? idx : ridx;
        auto path = path_.left(idx);
        QDir dir; dir.mkpath(path);
    }
    if (QFile file(path_); file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::NewOnly) 
        || file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::ExistingOnly)) {
        QJsonDocument doc = QJsonDocument::fromVariant(document_);
        file.write(doc.toJson());
        qInfo() << "Save configuration into" << path_ << ".";
    } else {
        qCritical() << "Save configuration into" << path_ << "failure: " << file.errorString();
    }
}

QVariant Configuration::get(QString path) const noexcept {
    path = path.trimmed();
    if (path.isEmpty()) return document_;

    QReadLocker _(&this->lock_);
    return variantGet(document_, path);
}

SetResult Configuration::set(QString path, QVariant val) noexcept {
    path = path.trimmed();
    if (path.isEmpty()) return SetError;

    QWriteLocker _(&this->lock_);
    QVariant root = document_;
    SetResult res = variantSet(root, path, val);
    document_ = root.toMap();

    emit this->propertiesChanged(path, val);
    qInfo() << "[Configuration MESSAGE] Set path " << path;
    return res;
}

void Configuration::patch(QString path, QVariantMap value) noexcept {
    path = path.trimmed();
    if (value.isEmpty()) return;

    QWriteLocker _(&this->lock_);
    QVariant root = document_;
    if (variantPatch(root, path, value)) {
        document_ = root.toMap();
        emit this->propertiesChanged(path, value);
    }
    qInfo() << "[Configuration MESSAGE] Patch path " << path;
}