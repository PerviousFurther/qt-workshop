#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>

#include "PageDevice.hpp"

class DeviceModule;
class QPushButton;
class QListWidget;

class VerticalTitleBar : public QWidget {
    Q_OBJECT;
public:
    explicit VerticalTitleBar(const QString& text, QWidget* parent = nullptr);
    void setLabelStyle(const QString& style);

signals:
    void clicked();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override {
        QWidget::mouseReleaseEvent(event);
        if (!event->isAccepted())
            emit clicked();
    }

private:
    QLabel* label_ = nullptr;
};


class DeviceSearchPanel : public Extensible {
    Q_OBJECT
public:
    explicit DeviceSearchPanel(QWidget* parent = nullptr);
    void setExpanded(bool expanded);
    void triggerSearch();

    qsizetype appendWidget(QWidget* widget) override;
    QWidget* widget(qsizetype id, QWidget* widget) override;
    QWidget* removeWidget(qsizetype id) noexcept override;

signals:
    void titleClicked();

private slots:
    void onBluetoothIconClicked();
    void onDeviceDiscoverError(int errorCode, const QString& msg);
    void onDeviceConnectionError(int id, int errorCode, const QString& msg);
    void refreshAvailableDevices();
    void updateThemeAndStyles();

private:
    void updateDisplay();

    VerticalTitleBar* titleBar_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QPushButton* bluetoothBtn_ = nullptr;
    QLabel* statusText_ = nullptr;
    QWidget* scrollWidget_ = nullptr;
    QVBoxLayout* bleDeviceListLayout_ = nullptr;
    QVBoxLayout* bleLayout_ = nullptr;

    bool isExpanded_;
};

class ConnectedDevicesPanel : public Extensible {
    Q_OBJECT;
public:
    explicit ConnectedDevicesPanel(QWidget* parent = nullptr);
    void setExpanded(bool expanded);

    qsizetype appendWidget(QWidget* widget) override;
    QWidget* widget(qsizetype id, QWidget* widget) override;
    QWidget* removeWidget(qsizetype id) noexcept override;

signals:
    void titleClicked();
    void addDeviceRequested();

private slots:
    // void refreshActiveDevices();
    void updateThemeAndStyles();

private:
    void updateDisplay();

    VerticalTitleBar* titleBar_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QLabel* hintLabel_ = nullptr;
    QVBoxLayout* connectedDevicesLayout_ = nullptr;
    QPushButton* addDeviceBtn_ = nullptr;

    bool isExpanded_;
};


class RecordsPanel : public Extensible {
    Q_OBJECT
public:
    explicit RecordsPanel(QWidget* parent = nullptr);
    void setExpanded(bool expanded);
    // void addLogItem(const QString& date, const QString& name, const QString& duration, bool success);

    qsizetype appendWidget(QWidget* widget) override;
    QWidget* widget(qsizetype id, QWidget* widget) override;
    QWidget* removeWidget(qsizetype id) noexcept override;

signals:
    void titleClicked();

private slots:
    void updateThemeAndStyles();

private:
    void updateDisplay();

    VerticalTitleBar* titleBar_ = nullptr;
    QWidget* contentWidget_ = nullptr;
    QListWidget* listWidget_ = nullptr;
    QLabel* hintLabel_;

    bool isExpanded_;
};