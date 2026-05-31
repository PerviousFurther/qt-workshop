#pragma once

#include <QHBoxLayout>

#include "Extensible.hpp"

class DeviceSearchPanel;
class ConnectedDevicesPanel;
class RecordsPanel;
class Driver;


class PageDevice : public QWidget {
    Q_OBJECT;
public:
    enum DisplayState {
        LeftState,
        CenteredState,
        RightState
    };
    
public:
    explicit PageDevice(QWidget* parent = nullptr);
    virtual ~PageDevice() override = default;

    Extensible* search();
    Extensible* connected();
    Extensible* record();

private slots:
    void onDeviceConnected(int driver);
    void handleLeft();
    void handleCenter();
    void handleRight();
    void updateTopTheme();

private:
    
    void transitionToState(int newState);

    QHBoxLayout* mainLayout_ = nullptr;
    DeviceSearchPanel* searchPanel_ = nullptr;
    ConnectedDevicesPanel* connectedPanel_ = nullptr;
    RecordsPanel* recordPanel_ = nullptr;

    int currentState = DisplayState::CenteredState;
};