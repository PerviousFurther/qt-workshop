#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLibrary>
#include <QQueue>
#include <QSet>
#include <QDebug>
#include "Configuration.hpp"
#include "Module.hpp"

static const auto MODULE_NAME = lstr("Module");

namespace ErrorCategory {
    static const auto MODULE = lstr("模块损坏");
}

namespace ModuleField {
    static const auto ROOT = lstr("modules");
    static const auto NAME = lstr("name");
    static const auto PATH = lstr("module");
    static const auto KIND = lstr("kind");
    static const auto DEPS = lstr("dependencies");
    static const auto STATE = lstr("state");

    static const auto Enabled = lstr("enabled");
    static const auto Disabled = lstr("disabled");
    static const auto Suspend = lstr("suspended");
}

// The dependency should only contain public module. (Have MODULE_ENTRY(Q_DECL_EXPORT) and return the ModuleInstance)
// Like "Driver", they just offer some basic class, the dll shoould link it implicitly, when indicate in cmakelists. MUST not registered in here.
static const QVariantList DEFAULT_JSON = {
    QVariantMap{{ModuleField::PATH, lstr("Backend.Network")},      {ModuleField::DEPS, QVariantList{}}},
    QVariantMap{{ModuleField::PATH, lstr("Backend.AiChat")},       {ModuleField::DEPS, QVariantList{lstr("Backend.Network")}}},
    QVariantMap{{ModuleField::PATH, lstr("Backend.Device")},       {ModuleField::DEPS, QVariantList{}}},
    QVariantMap{{ModuleField::PATH, lstr("App.Ui")},               {ModuleField::DEPS, QVariantList{lstr("Backend.Device"), lstr("Backend.Network"), lstr("Backend.AiChat")}}},
    QVariantMap{{ModuleField::PATH, lstr("Driver.UiTest")},        {ModuleField::DEPS, QVariantList{lstr("Backend.Network"), lstr("App.Ui")}}},
    // QVariantMap{{ModuleField::PATH, lstr("Driver.History")},       {ModuleField::DEPS, QVariantList{lstr("Backend.Network")}}},
    QVariantMap{
        {ModuleField::PATH, lstr("Tools.Sleep")},
        {ModuleField::DEPS, QVariantList{
            lstr("Backend.Device"), lstr("Backend.Network"), lstr("App.Ui"), lstr("Driver.UiTest")}}},
};

static QVariantList config() {
    QVariantList modList;
    if (assign(modList, Configuration::instance().get(ModuleField::ROOT)) && !modList.isEmpty()) {
        return modList;
    }
    qWarning() << lstr("Module list is empty. Set to default.");
    Configuration::instance().set(ModuleField::ROOT, DEFAULT_JSON);
    return DEFAULT_JSON;
}

// --- 拓扑排序辅助方法 ---

static bool visit(QString& error,
    QString currentPath,
    QHash<QString, QVariantList> depMap,
    QSet<QString>& visited,
    QSet<QString>& visiting,
    QStringList& sortedList) {

    if (visiting.contains(currentPath)) {
        error = lstr("存在循环依赖: %1").arg(currentPath);
        return false;
    }
    if (visited.contains(currentPath)) return true;

    visiting.insert(currentPath);

    // 获取当前路径对应的依赖项
    if (depMap.contains(currentPath)) {
        for (const QVariant& depVar : depMap.value(currentPath)) {
            QString depPath = depVar.toString();
            if (!depMap.contains(depPath)) { error = lstr("模块%1尝试引用不存在的模块%2").arg(currentPath, depPath); }
            if (!visit(error, depPath, depMap, visited, visiting, sortedList)) {
                error = lstr("%1模块依赖出现循环").arg(currentPath);
                return false;
            }
        }
    }

    visiting.remove(currentPath);
    visited.insert(currentPath);
    sortedList.append(currentPath);
    return true;
}

static QStringList resolveOrder(QString& error, const QVariantList& configs) {
    QStringList sortedList;
    QSet<QString> visited, visiting;
    QHash<QString, QVariantList> depMap;

    for (const auto& item : configs) {
        QVariantMap m = item.toMap();
        depMap.insert(m.value(ModuleField::PATH).toString(), m.value(ModuleField::DEPS).toList());
    }

    for (const QString& path : depMap.keys()) {
        if (!visited.contains(path)) {
            if (!visit(error, path, depMap, visited, visiting, sortedList)) 
                return QStringList();
        }
    }
    return sortedList;
}

ModuleRegistry::ModuleRegistry(QObject* parent)
    : QObject(parent)
{
    Q_ASSERT(!instance_);
    instance_ = this;

    QString error;
    QVariantList rawConfigs = config();
    QStringList sortedPaths = resolveOrder(error, rawConfigs);
    if (sortedPaths.isEmpty() && !rawConfigs.isEmpty()) {
        FASTFAIL(error, ErrorCategory::MODULE);
    }

    for (const QString& path : sortedPaths) {
        qInfo() << "[Module Message] start initialize:" << path;
        QVariantMap mCfg;
        for (const auto& raw : rawConfigs) {
            if (raw.toMap().value(ModuleField::PATH).toString() == path) {
                mCfg = raw.toMap();
                break;
            }
        }

        QObject stub;
        QLibrary* lb = new QLibrary(path, &stub);
        if (!lb->load()) {
            FASTFAIL(lstr("加载库 %1 失败: %2").arg(path, lb->errorString()), ErrorCategory::MODULE);
        }

        auto fn = reinterpret_cast<FnGetModuleInstance>(lb->resolve(MODULE_ENTRY_NAME));
        if (!fn) FASTFAIL(lstr("库 %1 缺少入口").arg(path), ErrorCategory::MODULE);

        Module* mod = fn();
        if (!mod) FASTFAIL(lstr("库 %1 实例创建失败").arg(path), ErrorCategory::MODULE);
        mod->setParent(&stub);

        QString realName = mod->moduleName();
        mCfg[ModuleField::NAME] = realName;
        mCfg[ModuleField::STATE] = ModuleField::Enabled;

        this->moduleStates_.insert(realName, mCfg);
        this->modules_.insert(realName, mod);
        mod->setParent(this);

        this->lib_.insert(realName, lb);
        lb->setParent(this);

        this->order_.append(realName);
    }

    instance_ = this;
    // SingletonInQml("Module", this);
}

ModuleRegistry::~ModuleRegistry() {
    for (auto it = this->order_.rbegin(); it != this->order_.rend(); ++it) {
        this->signOut(*it);
        if (this->modules_.contains(*it)) {
            this->modules_.value(*it)->release();
        }
    }

    QVariantList saveList;
    for (const auto& name : this->order_) {
        QVariantMap outMap = this->moduleStates_[name];
        // outMap.remove(ModuleField::NAME);
        saveList.append(outMap);
    }
    Configuration::instance().set(ModuleField::ROOT, saveList);
}

void ModuleRegistry::load() {
    for (auto& moduleName : this->order_) {
        const auto& c = this->moduleStates_.value(moduleName);
        qInfo() << lstr("[MODULE load] Loading %1...").arg(moduleName);
        if (c.value(ModuleField::STATE).toString() == ModuleField::Enabled)
            this->signIn(moduleName);
    }
    emit loadFinished();
}

bool ModuleRegistry::verifyModuleName(QString& error, QString& moduleName) {
    moduleName = moduleName.trimmed();
    if (moduleName.isEmpty()) {
        error = lstr("Module name is empty.");
        return false;
    }
    if (!this->moduleStates_.contains(moduleName)) {
        error = lstr("Module '%1' not found.").arg(moduleName);
        return false;
    }
    return true;
}

bool ModuleRegistry::signIn(QString name) {
    qInfo() << "[Module Message] Try sign in:" << name;
    QMutexLocker _{ &this->lock_ };
    QString error;
    if (!verifyModuleName(error, name)) return false;

    auto& state = moduleStates_[name];
    auto mod = this->modules_[name];
    if (!mod->load(error)) {
        qCritical() << lstr(" %1 module load occurs error! %2").arg(mod->moduleName(), error);
        return false;
    }
        
    state[ModuleField::STATE] = ModuleField::Enabled;
    return true;
}

bool ModuleRegistry::signOut(QString name, bool disable) noexcept {
    qInfo() << "[Module Message] Try sign out:" << name;
    QMutexLocker _{ &this->lock_ };
    QString error;
    if (!verifyModuleName(error, name)) return false;

    auto& state = moduleStates_[name];
    auto mod = this->modules_[name];
    if (!mod->unload(error)) {
        qCritical() << lstr("%1 module unload occurs error! %2").arg(mod->moduleName(), error);
        return false;
    }
    state[ModuleField::STATE] = (disable ? ModuleField::Suspend : ModuleField::Enabled);
    return true;
}

ModuleComponent* ModuleRegistry::getByName(QString moduleName, QString name) noexcept {
    if (auto module = getByName(moduleName))
        return module->getByName(name);
    return nullptr;
}

Module* ModuleRegistry::getByName(QString moduleName) noexcept {
    QMutexLocker _{ &this->lock_ };
    QString error;
    if (!verifyModuleName(error, moduleName)) return nullptr;
    auto& state = this->moduleStates_[moduleName];
    if (state[ModuleField::STATE].toString() != ModuleField::Enabled) return nullptr;
    return this->modules_.value(moduleName);
}

bool ModuleRegistry::marked(QString moduleName) const noexcept {
    return this->moduleStates_.contains(moduleName);
}

Module::Module(QString name, int major, int minor, int patch)
    : majorVer_(major), minorVer_(minor), patchVer_(patch), modname_(name)
{
}