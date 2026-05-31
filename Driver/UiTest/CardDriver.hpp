// CardDriver.hpp
#pragma once

#include <QWidget>

class DeviceSearchCard : public QWidget {
    Q_OBJECT
public:
    DeviceSearchCard(int id, const QString& name, const QString& mac, QWidget* parent = nullptr);

private:
    void updateStyle();
};

class DeviceConnectedCard : public QWidget {
    Q_OBJECT
public:
    DeviceConnectedCard(int id, const QString& name, const QString& ip, QWidget* parent = nullptr);

signals:
    void requestDetailPage(int id);

private:
    void updateStyle();
};

class DeviceRecordCard : public QWidget {
    Q_OBJECT
public:
    DeviceRecordCard(const QString& time, const QString& action, bool isSuccess, QWidget* parent = nullptr);

private:
    void updateStyle();
};