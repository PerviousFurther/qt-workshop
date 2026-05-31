#pragma once

#include <QHash>
#include <QString>
#include <QVariant>
#include <QLibrary>
#include <QMutex>

// #if RINGAPP_ENABLE_GUI
// #   include <QQml.h>
// #endif

#include "Utils.hpp"

class Module;
class ModuleRegistry;

using FnGetModuleInstance = Module*(*)(); // No need parent, registry will automatically setParent.

class
#if defined(RINGAPP_CORE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
ModuleComponent : public QObject {
    Q_OBJECT;
public:
    using QObject::QObject;

    // For initialze objects and other things.
    // return true means the load operation is success.
    virtual bool load(QString& error) = 0;
    // For deinitialze objects and other things.
    // return true means the unload operation is success.
    virtual bool unload(QString& error) = 0;
};

// On design, the implementation's constructor will be called at `main` function.
// `load` and `unload` will call at runtime.
// all neccessary check should be done at constructor, because it can call FASTFAIL to exit process.
// but load and unload cannot call FASTFAIL, since the process is already started.
class
#if defined(RINGAPP_CORE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
Module : public ModuleComponent {
    Q_OBJECT;
public:
    Module(QString name, int major = 1, int minor = 0, int patch = 0);

    virtual ModuleComponent* getByName(QString name) noexcept {
        // const auto key = QString(name);
        // Q_ASSERT(singletons_.contains(key));
        // return // singletons_[key];
        // TODO: maybe?
        return nullptr;
    }

    // do delete the object. should be noexcept.
    // USER Should not call at this method.
    virtual void release() noexcept = 0;
    
    int majorVersion() const noexcept { return majorVer_; }
    int minorVersion() const noexcept { return minorVer_; }

    QString moduleName() const noexcept { return modname_; }

    // Not thread safe.
    // template<typename Interface>
    // bool registered(bool setFalse = false) {
    //     static bool isReg = false;
    //     bool w = isReg;
    //     if (setFalse) isReg = false;
    //     else isReg = true;
    //     return w;
    // }
    // Not thread safe, the register operation no need to assign. althrough it return the `Implmentation`'s pointer.
    // template<typename Interface, typename Implementation, typename...Args>
    // Implementation* registerSingleton(Args&&... args) {
    //     auto* object = registerSingletonWithoutQml<Interface, Implementation>(static_cast<Args&&>(args)...);
    //     registerQmlSingleton<Interface>(object);
    //     return object;
    // }
    // Not thread safe.
    // template<typename Interface, typename Implementation, typename...Args>
    // Implementation* registerSingletonWithoutQml(Args&&... args) {
    //     if (registered<Interface>())
    //         (FASTFAIL)(QStringLiteral("Interface '%1' have already registered.").arg(Interface::name), Interface::name, lstr("模块注册错误"));
    //     const auto key = QString::fromUtf8(Interface::name);
    //     Q_ASSERT(!singletons_.contains(key));
    //     auto* object = new Implementation(static_cast<Args&&>(args)...);
    //     singletons_.insert(key, object);
    //     return object;
    // }
    // Not thread safe.
    // template<typename Interface>
    // void registerQmlSingleton(Interface* object) {
    // #if RINGAPP_ENABLE_GUI
    //     // Expose the module-owned session instance to QML as a singleton type.
    //     qmlRegisterSingletonInstance<Interface>(
    //         Interface::moduleName,
    //         majorVer_,
    //         minorVer_,
    //         Interface::name,
    //         object);
    // #endif
    // }
    // QHash<QString, ModuleComponent*> singletons_{};

protected:
    int majorVer_{};
    int minorVer_{};
    int patchVer_{};
    QString modname_{};

};

class QLibrary;
class
#if defined(RINGAPP_CORE_EXPORT)
    Q_DECL_EXPORT
#else
    Q_DECL_IMPORT
#endif
ModuleRegistry : public QObject {
    Q_OBJECT;
    // QML_ELEMENT;
    // QML_SINGLETON;

// public:
    // Q_PROPERTY(QString mainQml READ mainQml NOTIFY mainQmlChanged FINAL)

public:
    static ModuleRegistry& instance() { ASSUME(instance_); return *instance_; }

    explicit ModuleRegistry(QObject* parent = nullptr);

    ModuleRegistry(ModuleRegistry&&) = delete;
    ModuleRegistry& operator=(ModuleRegistry&&) = delete;

    ~ModuleRegistry();

signals:
    void loadFinished();

public:
    void load();

    // Not thread safe.
    // Resolve the binary from configuration so lazy callers only depend on the logical module name.
    // For execution load.
    bool signIn(QString moduleName);
    // Not thread safe.
    // require module with the name is already registered.
    // For execution unload.
    // if disable is true, mark the module is disabled, and next time will not load the module.
    bool signOut(QString moduleName, bool disable = false) noexcept;
    
    Q_INVOKABLE bool marked(QString moduleName) const noexcept;

    template<typename T>
    T* get() noexcept { return static_cast<T*>(this->getByName(T::moduleName())); }

    // Do static_cast to interface if you already know what interface it is.
    // DO NOT cast to implmentation class, the behavior is undefined.
    ModuleComponent* getByName(QString moduleName, QString name) noexcept;
    // This method is for get modules.
    Module* getByName(QString instanceName) noexcept;

private:
    bool verifyModuleName(QString& error, QString& moduleName);

private:
    inline static ModuleRegistry* instance_ = nullptr;

private:
    mutable QRecursiveMutex lock_;
    QHash<QString, QVariantMap> moduleStates_;
    QHash<QString, Module*> modules_;
    QHash<QString, QLibrary*> lib_;
    QStringList order_;
};



#define MODULE_ENTRY(...) extern "C" __VA_ARGS__ Module* getModuleInstance
#define MODULE_ENTRY_NAME "getModuleInstance"

// On design, this object is use for header files.
#define BEGIN_MODULE(name) \
class name : public Module { \
public: \
    explicit name(); \
    ~name(); \
    bool load(QString& error) override { if(!loaded) for (auto s : this->singletons_.values()) if (!s->load(error)) return false; loaded = true; return true; } \
    bool unload(QString& error) override { if(loaded) for (auto s : this->singletons_.values()) if (!s->unload(error)) return false; loaded = false; return true; } \
    void release() noexcept override; \
    template<typename T> \
    T* get() noexcept { return static_cast<T*>(getByName(T::name)); } \
private: \
    bool loaded = false; \
}; \
MODULE_ENTRY(Q_DECL_EXPORT)()

#define END_MODULE(name) \
MODULE_ENTRY(Q_DECL_EXPORT)() { return new name(); } \
void name::release() noexcept { delete this; } \
name::~name() = default;

#define InitRes(xx) Q_INIT_RESOURCE(xx) 
#define UninitRes(xx) Q_CLEANUP_RESOURCE(xx) 