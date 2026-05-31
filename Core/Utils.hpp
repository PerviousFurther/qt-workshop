#pragma once

#include <QString>
#include <QtLogging>
#include <QPointer>
#include <QVariant>

#if defined(Q_CC_MSVC)
#   define ASSUME(cond) __assume(cond)
#elif defined(Q_CC_CLANG)
#   define ASSUME(cond) __builtin_assume(cond)
#elif defined(Q_CC_GNU)
#   define ASSUME(cond) __attribute__((assume(cond)))
#else
#   define ASSUME(cond) [[assume(cond)]]
#endif

#define lstr QStringLiteral

#define AS_STR_IMPL(x) #x
#define AS_STR(x) AS_STR_IMPL(x)

[[noreturn]] inline void (FASTFAIL)(QString const& detail, QString const& moduleName = lstr("^Unknown^"), QString const& category = lstr("内部错误")) {
    qCritical().noquote() << moduleName << detail;
    qFatal().noquote() << category << ':' << detail;
}
#define FASTFAIL(str, ...) FASTFAIL(str, MODULE_NAME __VA_OPT__(,__VA_ARGS__))

using namespace Qt::StringLiterals;

template<typename T>
concept QObjectInterchangable = requires(T *value, QObject *p) { static_cast<QObject*>(value); static_cast<T*>(p); };
template<QObjectInterchangable T>
QVariant box(T* value) 
{ return QVariant::fromValue(static_cast<QObject*>(value)); }

// can set parent to switch ownership.
template<QObjectInterchangable T>
T* unbox(QVariant value, QObject* parent = nullptr) {
    auto obj = value.template value<QObject*>();
    if (obj && parent) obj->setParent(parent);
    return static_cast<T*>(obj); 
}

template<typename...Ts, typename Map>
bool copy(Map const& map, QString key) noexcept 
    requires(requires{ bool(map.contains(key) && ((map[key].template canConvert<Ts>()) || ...)); }) 
{ return map.contains(key) && ((map[key].template canConvert<Ts>()) || ...); }

template<typename T, typename Map>
bool copy(T& out, Map const& map, QString key) noexcept 
    requires(requires{ (map.contains(key) && map[key].template canConvert<T>()); }) {
    if (map.contains(key) && map[key].template canConvert<T>()) {
        out = map[key].template value<T>();
        return true;
    } else
        return false;
}

template<typename T>
bool assign(T& value, QVariant var) noexcept {
    if (var.canConvert<T>()) { value = var.value<T>(); return true; }
    else return false;
}

template<typename T, typename Map>
T valueOr(Map&& map, QString key, T defaultVal = {}) noexcept
    requires(requires{ copy<T>(map, key); }) {
    if (copy<T>(map, key)) 
        return static_cast<Map&&>(map)[key].value<T>();
    else 
        return qMove(defaultVal);
}

class QJsonDocument;

#if defined(RINGAPP_CORE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
QVariantMap fromJson(QJsonDocument const&);


#include <QRegularExpression>
static const QRegularExpression IllegalFilenameRegex("[\\\\/:*?\"<>|\\x00-\\x1F]");

// #define SingletonInQml(classname, instance) \
// qmlRegisterSingletonInstance(QML_MODULE_NAME, QML_MODULE_MAJOR, QML_MODULE_MINOR, classname, instance)
// 
// #define InterfaceInQml(type) \
// qmlRegisterUncreatableType<type>(QML_MODULE_NAME, QML_MODULE_MAJOR, QML_MODULE_MINOR, #type, #type " is ")
// 
// #define QML_PROPERTY(type, name, ...) \
// Q_PROPERTY(type name READ name NOTIFY name##Changed);\
// signals: void name##Changed(); public: __VA_ARGS__ type name()


template<typename T>
T map(QVariantMap map, QString key, T fallback) {
    if (auto it = map.find(key); it != map.end())
        if (it->canConvert<T>())
            return it->value<T>();
        else
            return qMove(fallback);
    else
        return qMove(fallback);
}

template<typename T, typename C, typename D>
T clamp(T value, C minValue, D maxValue) {
    return qMax(static_cast<T>(minValue), qMin(maxValue, static_cast<T>(value)));
}

#define DEFINE_ALLOC() \
   struct DEBUG_header_ { \
        quint32 serial;                                                       \
        quint32 MAGIC_CODE;                                                   \
   };                                                                         \
   void* operator new(::std::size_t count) { \
       static qsizetype counter = 0;                                          \
       auto c = static_cast<DEBUG_header_*>(                                  \
           ::std::malloc(sizeof(DEBUG_header_) + count));                     \
       c->serial = counter++; c->MAGIC_CODE = 0xffff0000u;                    \
       qDebug() << __FILE__ << "Serial code" << c->serial                     \
        << "Allocate size:" << count;                                         \
       return c + 1;                                                          \
   }                                                                          \
   void operator delete(void* ptr) noexcept { \
       if (!ptr) return;                                                      \
       auto c = static_cast<DEBUG_header_*>(ptr) - 1;                         \
       if (c->MAGIC_CODE == 0xffff0000u) { \
            qDebug() << __FILE__ << "Serial code" << c->serial << "delete.";   \
            ::std::free(c);                                                    \
       } else { \
            qDebug() << __FILE__ << "Unknown delete.";                         \
            ::std::free(ptr);                                                  \
       }                                                                      \
   };
