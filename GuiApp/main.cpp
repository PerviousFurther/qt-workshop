#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QtLogging>

#include "Core/Utils.hpp"
#include "Core/Configuration.hpp"
#include "Core/Module.hpp"


// #include <crtdbg.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);

    qInfo() << "Current work directory:" << QDir::current().absolutePath();
    qInfo() << "Lib path:" << app.libraryPaths();

    Configuration cfg("Local/configuration.json");
    ModuleRegistry reg;
    QTimer::singleShot(0, &reg, &ModuleRegistry::load);

    const auto res = app.exec();
    qInfo() << "-----------------END EXECUTE----------------";
    return res;
}
