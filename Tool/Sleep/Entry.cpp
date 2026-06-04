#include "Core/Utils.hpp"
#include "Entry.hpp"
#include "Backend/Device/Session.hpp"
#include "Backend/Ui/Session.hpp"
#include "Backend/Ui/Extensible.hpp"
#include "Driver/UiTest/Driver.hpp"

// #include "CardSleepStaging.hpp"

// #include "HeartRateWidget.hpp"
// #include "SpoWidget.hpp"
// #include "SleepWidget.hpp"
// #include "SleepPieWidget.hpp"
// #include "BarChart.hpp"

#include "SleepSummary.hpp"
#include "PageSleep.hpp"
#include "CardUiTestDetails.hpp"

// #include ""
// static auto uiSession() noexcept { return ModuleRegistry::instance().get<UiSession>(); }
static auto uiSession() noexcept { return UiSession::instance(); }
ToolSleep::ToolSleep()
    : Module(QML_MODULE_NAME, QML_MODULE_MAJOR, QML_MODULE_MINOR)
{
    
    auto ui = uiSession();
    auto uitest = UITestModule::instance();
    this->detailCards_ = new SleepDashboardWidget;
    this->pageSleep_ = new PSGSummaryCard;
    // this->pageSleep_ = new PSGMainPage;
    
    // this->cardIds_.append(ui->home()->appendWidget(card));
    connect(uitest, &UITestModule::deviceConnected, this, [ui, uitest, this]() {
        auto card = static_cast<PSGSummaryCard*>(this->pageSleep_);
        this->cardIds_.append(ui->home()->appendWidget(card));
        connect(card, &PSGSummaryCard::clicked, this, [ui, this]() {
            qInfo() << "enter psg main page.";
            ui->push(new PSGMainPage);
        });
        connect(uitest, &UITestModule::deviceConnectionError, this, [ui, card, this](int, int code, QString msg) {
            if ((code & DeviceField::Error_Disconnected) != 0) {
                ui->home()->removeWidget(card);
            }
        });
    });
    connect(uitest, &UITestModule::openingDetailPage, this, [uitest, this](int, QString identifier) {
        auto card = new SleepDashboardWidget();
        card->setObjectName("SleepDashboardWidget");
        card->loadRecord(identifier);
        uitest->detailPage()->appendWidget(card);
    });
    connect(uitest, &UITestModule::closeDetailPage, this, [uitest, this](int, QString identifier) {
        // detailCards_->stopRecording();
    });
    auto dv = DeviceSession::instance();
}

ToolSleep::~ToolSleep() {
    for (auto it = this->cardIds_.rbegin(); it != this->cardIds_.rend(); it++)
        uiSession()->home()->removeWidget(*it);
}

void ToolSleep::release() noexcept { delete this; }

bool ToolSleep::load(QString&) {
    return true;
}

bool ToolSleep::unload(QString&) {
    // auto* ui = uiSession();
    // if (ui) {
    //     for (auto id : cardIds_) {
    //         ui->remove_homeCards(id);
    //     }
    // }
    // cardIds_.clear();
    return true;
}

void ToolSleep::onConnected(int driver) {
    if (!driver) {
        qCritical() << "[ToolSleep ERROR] detect empty driver connected.";
    }
    // if (driver->id() == DriverField::UiTest::Id) {
    // 
    // }

    // qInfo() << "Sleep system detect `uitest` driver connected.";
    // auto* ui = uiSession();
    // if (!ui) {
    //     return;
    // }


    // for (const auto& card : cards) {
    //     cardIds_.append(ui->home(card));
    // }

    // emit deviceChanged();
}

void ToolSleep::onConnectionError(int, int, QString)
{
    // if (!driver || driver->id() != DriverField::UiTest::Id || code != DriverField::Error_Disconnected) {
    //     return;
    // }

    // auto* ui = uiSession();
    // for (auto id : cardIds_) {
    //     ui->remove_homeCards(id);
    // }
    // cardIds_.clear();
    // emit deviceChanged();
}



MODULE_ENTRY(Q_DECL_EXPORT)()
{
    return new ToolSleep();
}
