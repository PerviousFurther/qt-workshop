#include <QTimer>
#include <QtMath>
#include <QRandomGenerator>

#include "Backend/Device/Session.hpp"
#include "Backend/Network/Session.hpp"
#include "Backend/Ui/Extensible.hpp"
#include "Backend/Ui/Session.hpp"
#include "CardDriver.hpp"
#include "DetailPage.hpp"
#include "Driver.hpp"

UITestModule::UITestModule()
    : BasicDeviceModule(QML_MODULE_NAME, QML_MODULE_MAJOR, QML_MODULE_MINOR) {
    instance_ = this;
    simTimer_ = new QTimer(this);
    connect(simTimer_, &QTimer::timeout, this, &UITestModule::onTimerTimeout);
    connect(this, &UITestModule::recordsChanged, this, &UITestModule::onRecordsChanged);
}

void UITestModule::onRecordsChanged() {
    auto ui = UiSession::instance();
    auto record = ui->deviceRecords();
    for (auto id : this->recordIds_) {
        record->deleteWidget(id);
    }
    for (auto [key, name] : this->recordsToFullPath_.asKeyValueRange()) {
        auto card = new DeviceRecordCard(this->deviceId_, key, name);
        connect(card, &DeviceRecordCard::requestRecordPage, this, &UITestModule::openDetailPageW);
        this->recordIds_.append(record->appendWidget(card));
    }
}

// UiTest immediately reply, but concrete device should notice them and emit responded signal from device.
bool UITestModule::read(int id, int serialCode, QByteArray arr) {
    QByteArray responseData;
    switch (serialCode) {
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
        responseData = qExchange(channelBuffers_[serialCode], {});
        break;
    default:
        return false;
    }

    emit this->readResponded(id, serialCode, arr, responseData);
    return true;
}

// UiTest immediately reply, but concrete device should notice them and emit responded signal from device.
bool UITestModule::write(int id, int serialCode, QByteArray arr) {
    QByteArray ackData;
    bool success = false;
    switch (serialCode) {
    case DeviceField::UiTest::RequestSample: if (arr.size() > 2) {
        success = true;
        int hz = static_cast<uint8_t>(arr[0]);
        enabledChannels_ = (arr.size() > 1) ? static_cast<quint8>(arr[1]) : 0x1F;
        if (hz > 0) {
            auto interval = 1000 / hz;
            simTimer_->start(interval);
        } else {
            simTimer_->stop();
        }

        ackData = QByteArray("\x06");
    } else {
        ackData = QByteArray("\x16");
    } break;

    default:
        success = false;
        ackData = QByteArray("\x15");
        break;
    }

    emit this->writeResponded(id, serialCode, arr, ackData);
    return success;
}

// ==================== QTimer 定时器数据模拟 ====================
void UITestModule::onTimerTimeout() {
    sampleIndex_++;

    if (enabledChannels_ & (1 << (DeviceField::UiTest::SignalECG - 1))) {
        int16_t ecgSample = static_cast<int16_t>(512 + 200 * qSin(sampleIndex_ * 0.2));
        channelBuffers_[DeviceField::UiTest::SignalECG].append(reinterpret_cast<const char*>(&ecgSample), sizeof(ecgSample));
    }

    if (enabledChannels_ & (1 << (DeviceField::UiTest::SignalPPG - 1))) {
        int16_t ppgSample = static_cast<int16_t>(200 + 100 * qSin(sampleIndex_ * 0.05));
        channelBuffers_[DeviceField::UiTest::SignalPPG].append(reinterpret_cast<const char*>(&ppgSample), sizeof(ppgSample));
    }

    if (enabledChannels_ & (1 << (DeviceField::UiTest::SignalEEG - 1))) {
        int16_t eegSample = static_cast<int16_t>(128 + 30 * qSin(sampleIndex_ * 0.8));
        channelBuffers_[DeviceField::UiTest::SignalEEG].append(reinterpret_cast<const char*>(&eegSample), sizeof(eegSample));
    }

    if (enabledChannels_ & (1 << (DeviceField::UiTest::SignalCap - 1))) {
        int16_t nasalSample = static_cast<int16_t>(1000 + 50 * qCos(sampleIndex_ * 0.1));
        channelBuffers_[DeviceField::UiTest::SignalCap].append(reinterpret_cast<const char*>(&nasalSample), sizeof(nasalSample));
    }

    if (enabledChannels_ & (1 << (DeviceField::UiTest::SignalNoseNoise - 1))) {
        int16_t noiseSample = static_cast<int16_t>(QRandomGenerator::global()->bounded(0, 100));
        channelBuffers_[DeviceField::UiTest::SignalNoseNoise].append(reinterpret_cast<const char*>(&noiseSample), sizeof(noiseSample));
    }

    for (auto it = channelBuffers_.begin(); it != channelBuffers_.end(); ++it) {
        if (it.value().size() > 4096) {
            it.value().clear();
        }
    }
}
Extensible* UITestModule::detailPage() { return this->detailPage_; }

void UITestModule::release() noexcept { delete this; }

QStringList UITestModule::devices() { return {DeviceField::UiTest::Name}; }
QVariantMap UITestModule::infoOf(int id) { return {}; }

bool UITestModule::load(QString& error) { 
    return BasicDeviceModule::load(error) && DeviceSession::instance()->registerDriverModule(this);
}
bool UITestModule::unload(QString& error) {
    for (auto c : this->recordIds_) 
        UiSession::instance()->deviceRecords()->deleteWidget(c);
    return BasicDeviceModule::unload(error) && DeviceSession::instance()->unregisterDriverModule(this);
}

bool UITestModule::writeRecordFile(QString identifier, QString filename, QByteArray rawData) {
    auto result = BasicDeviceModule::writeRecordFile(identifier, filename, rawData);
    if (result) {
        // auto record = new DeviceRecordCard(filename, lstr("测量"), true);
        // this->connectedIds_.append(UiSession::instance()->deviceRecords()->appendWidget());
    } else {

    }
    return result;
}

bool UITestModule::readRecordFile(QString identifier) {
    auto result = BasicDeviceModule::readRecordFile(identifier);
    if (result) {

    }
    return result;
}


bool UITestModule::activate(int id, QBluetoothDeviceInfo const&) {
    if (this->deviceId_ != -1) return false;
    this->deviceId_ = id;
    auto card = new DeviceSearchCard(id, DeviceField::UiTest::Name, "XXXX-UiTestXXXX");
    this->searchId_ = UiSession::instance()->searchDevicePage()->appendWidget(card);
    // connect(DeviceSession, &DeviceSearchCard::)
    return true;
}

bool UITestModule::deactivate(int id) {
    if (this->deviceId_ == -1) return false;
    auto ui = UiSession::instance();
    if (this->searchId_ != -1)
        ui->searchDevicePage()->deleteWidget(this->searchId_);
    if (this->connectedId_ != -1)
        ui->connectedDevice()->deleteWidget(this->connectedId_);
    qExchange(this->deviceId_, -1);
    return true;
}

bool UITestModule::connectDevice(int id) {
    deviceId_ = id;
    QTimer::singleShot(1000, [this, id]() {
        qInfo() << "[UiTest Message] Device connected, id:" << id;
        auto ui = UiSession::instance();
        ui->searchDevicePage()->deleteWidget(qExchange(this->searchId_, -1));
        auto card = new DeviceConnectedCard(id, DeviceField::UiTest::Name, "XXXX-UiTestXXXX");
        connect(card, &DeviceConnectedCard::requestDetailPage, this, &UITestModule::openDetailPage);
        this->connectedId_ = ui->connectedDevice()->appendWidget(card);
        emit this->deviceConnected(id);
    });
    return true;
}

bool UITestModule::disconnectDevice(int id) {
    QTimer::singleShot(1000, [this, id]() {
        qInfo() << "[UiTest Message] Device disconnected, id:" << id;
        auto ui = UiSession::instance();
        emit this->deviceConnectionError(id, DeviceField::Error_StopManually, lstr());
        ui->connectedDevice()->deleteWidget(qExchange(this->connectedId_, -1));
    });

    if (simTimer_->isActive()) {
        simTimer_->stop();
    }
    
    return true;
}

void UITestModule::openDetailPage(int id) {
    return openDetailPageW(id, {});
}

void UITestModule::openDetailPageW(int id, QString ide) {
    auto page = new UiTestDetailPage(id, "UI测试页面");
    this->detailPage_ = page;
    emit this->openingDetailPage(id, ide);
    UiSession::instance()->push(page);
    connect(page, &UiTestDetailPage::requestBack, this, [this, page, id, ide]() {
        while (UiSession::instance()->pop() != page);
        emit this->closeDetailPage(id, ide);
        delete page;
    });
    connect(page, &UiTestDetailPage::requestMeasure, this, &UITestModule::requestMeasure);
}


MODULE_ENTRY(Q_DECL_EXPORT)() {
    return new UITestModule();
}