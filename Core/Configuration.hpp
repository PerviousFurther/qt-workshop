#pragma once

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QReadWriteLock>
#include "Utils.hpp"

struct BasicError {
    int code;
    QString message;
};

class
#if defined(RINGAPP_CORE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
Configuration : public QObject {
    Q_OBJECT;
public:
    enum SetResult {
        SetError,          // error.
        SetAssigned,       // assign value.
        SetCreateDirect,   // set with "xxx"
        SetCreateWithDot   // set with "xxx.xxx". 
    };

public:
    Configuration(QString path, QObject* parent = nullptr);
    ~Configuration();

    static Configuration& instance() noexcept { ASSUME(singletonInstance_); return *singletonInstance_; }

signals:
    void propertiesChanged(QString path, QVariant const& value);
    
public:
    static const QString modulePath;

    void save();

    QVariantMap get() const noexcept;
    QVariant get(QString path) const noexcept;
    SetResult set(QString path, QVariant value) noexcept;
    void patch(QString path, QVariantMap value) noexcept;
private:
    inline static Configuration* singletonInstance_ = nullptr;

    mutable QReadWriteLock lock_{};

    QVariantMap document_{};
    QString path_{};
};

namespace ErrorCategory {
    inline const QString CONFIGURATION = lstr("配置错误");
}

namespace Field {
    inline const auto ROOT = lstr("");
    
#if defined(RINGAPP_CORE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
    QString member(QString name);

    // Binary's directory.
    // the implmentation should place them below some folder to avoid filename intersection.
    inline const auto BinDirectory = lstr("binDir");
    // Cache's directory.
    // json or other configuration's file.
    inline const auto CacheDirectory = lstr("cacheDir");
    // should be "x.x.x.x"
    inline const auto VersionNumber = lstr("version");

}
