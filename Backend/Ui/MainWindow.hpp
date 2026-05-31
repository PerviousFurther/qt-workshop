#pragma once

#include <QMainWindow>
#include <QVariantMap>
#include <QPointer>
#include <QStack>

class ModuleRegistry;
class QStackedWidget;
class QTimer;
class QStackWidget;
class QProgressBar;
class QListWidget;

class UiSession;
class LoadingWidget;
class Container;
class MainWindow : public QMainWindow {
    Q_OBJECT;
public:
    explicit MainWindow(UiSession* parent);
    ~MainWindow() override = default;

    // Extensible* main() { return mainPage_; }
    
public slots:
    // void onLoadFinished();
    void switchToLoad();
    void switchToMain();
    void showLoad();

    // TODO
    void push(QWidget* widget);
    QWidget* pop();

private:
    // One time use value.
    inline static bool modulesReady_ = false;
    inline static bool timesUp_ = false;
    inline static ModuleRegistry* modules_ = nullptr;

    QStackedWidget* stackedWidget_ = nullptr;
    QTimer* startupTimer_ = nullptr;

    QPointer<LoadingWidget> startup_;
    QPointer<Container> mainPage_;
    // QStack<QWidget*> widgetStack_;
};
