#include <QFrame>
#include <QAbstractAnimation>
#include <QGraphicsBlurEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QEvent>
#include <QMouseEvent>

#include "Core/Module.hpp"
#include "Backend/Ui/Session.hpp"
#include "Loading.hpp"
#include "Container.hpp"
#include "MainWindow.hpp"
// #include "GraphicsEffects.hpp"

// Main window.
MainWindow::MainWindow(UiSession* ui)
    : QMainWindow() {
    Q_ASSERT_X(ui, "MainWindow::MainWindow", "UiSession should be initialized before main window.");
    Q_ASSERT_X(!modules_, "MainWindow::MainWindow", 
        "Main window can only initailized once.");
    modules_ = &ModuleRegistry::instance();
    Q_ASSERT_X(modules_, "MainWindow::MainWindow", 
        "ModuleRegistry should initialize at main function, and initialize before any of dll.");

    this->resize(900, 600);
    this->setWindowTitle(lstr("RingApp"));

    stackedWidget_ = new QStackedWidget();

    startup_ = new LoadingWidget(this);

    stackedWidget_->addWidget(startup_);
    stackedWidget_->setCurrentWidget(startup_);

    this->setCentralWidget(stackedWidget_);

    startupTimer_ = new QTimer(this);
    startupTimer_->setInterval(1500); // 1.5s
    startupTimer_->setSingleShot(true);
    connect(&ModuleRegistry::instance(), &ModuleRegistry::loadFinished, this, [this]() {
            stackedWidget_->addWidget(mainPage_ = new Container(this));
        });
    connect(startupTimer_, &QTimer::timeout, this, [this]() {
        this->timesUp_ = true;
        if (this->modulesReady_) {
            this->switchToMain();
            qInfo() << "----------------BEGIN UI----------------";
        } else {
            this->showLoad();
        }
    });
    connect(modules_, &ModuleRegistry::loadFinished, this, [this]() {
        this->modulesReady_ = true;
        if (this->timesUp_) {
            this->switchToMain();
            qInfo() << "----------------BEGIN UI----------------";
        } else {
            this->showLoad();
        }
    });
    
    connect(ui, &UiSession::updating, this, &MainWindow::switchToLoad);
    connect(ui, &UiSession::updated, this, &MainWindow::switchToMain);

    startup_->fadeIn();
    startupTimer_->start();
    this->setStyleSheet(lstr("QMainWindow { background-color: %1; }")
        .arg(ui->theme().value(lstr("background")).toString()));
}

void MainWindow::switchToLoad() {
    if (mainPage_) {
        mainPage_->fadeOut();
    }
    if (startup_) {
        startup_->fadeIn();
    }
    stackedWidget_->setCurrentWidget(startup_);
}

void MainWindow::switchToMain() {
    if (mainPage_) {
        mainPage_->fadeIn();
    }
    if (startup_) {
        startup_->fadeOut();
    }
    stackedWidget_->setCurrentWidget(mainPage_);
}

void MainWindow::showLoad() {
    if (startup_) {
        QVariantMap loadParams;
        loadParams[lstr("text")] = lstr("Loading modules...");
        loadParams[lstr("animate")] = true;
        startup_->startLoading(loadParams);
    }
}


void MainWindow::push(QWidget* widget) {
    // qWarning() << "[MainWindow WARNING] Push operation is not finished.";
    stackedWidget_->addWidget(widget);
    stackedWidget_->setCurrentWidget(widget);
}

QWidget* MainWindow::pop() {
    // qWarning() << "[MainWindow WARNING] Pop operation is not finished.";
    auto w = stackedWidget_->currentWidget();
    w->setParent(nullptr);
    stackedWidget_->removeWidget(w);
    return w;
}



