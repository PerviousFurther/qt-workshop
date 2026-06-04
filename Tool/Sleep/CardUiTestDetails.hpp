#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QVariantMap>
#include <QColor>
#include <QSpinBox>
#include <QList>
#include <QPointF>
#include <QTimer>

#include "Driver/UiTest/Field.hpp"
#include "Tool/Basic/CardWaveform.hpp"
#include "Tool/Basic/CardBarChart.hpp"
#include "Tool/Basic/CardPieChart.hpp"
#include "Tool/Basic/CardStage.hpp"
#include "Backend/Ui/Field.hpp"


// ============================================================================
// 基础睡眠卡片类
// ============================================================================
class SleepBaseCard : public QWidget {
    Q_OBJECT
public:
    explicit SleepBaseCard(const QString& title, QWidget* parent = nullptr);

public:
    bool checked();
    void setViewState(bool measuring, bool hasData);

protected:
    int addView(QWidget* widget);
    virtual void updateThemeStyles(const QVariantMap& theme, const QVariantMap& font) = 0;

protected slots:
    void onToggleViewClicked();
    void onThemeChanged();

private:
    void applyCardBaseStyle(const QVariantMap& theme);

protected:
    QLabel* titleLabel_ = nullptr;
    QPushButton* toggleBtn_ = nullptr;
    QStackedWidget* stackedWidget_ = nullptr;
    QPushButton* enableBtn_ = nullptr;
};

// ============================================================================
// 1. 睡眠分期卡片 
// ============================================================================
class SleepStagingCard : public SleepBaseCard {
    Q_OBJECT
public:
    explicit SleepStagingCard(QWidget* parent = nullptr);
    void setData(const QList<StageData>& stageData, const QList<qreal>& piePercentages);

protected:
    void updateThemeStyles(const QVariantMap& theme, const QVariantMap& font) override;

private:
    StagingWidget* stagingWidget_ = nullptr;
    PieChartWidget* pieChartWidget_ = nullptr;
};

// ============================================================================
// 2. 呼吸事件卡片
// ============================================================================
class RespiratoryEventsCard : public SleepBaseCard {
    Q_OBJECT
public:
    explicit RespiratoryEventsCard(QWidget* parent = nullptr);
    void setData(const QList<QList<StageData>>& eventData,
        const QList<QList<qreal>>& barValues,
        const QList<QList<QString>>& barLabels);

protected:
    void updateThemeStyles(const QVariantMap& theme, const QVariantMap& font) override;

private:
    EventWidget* eventWidget_ = nullptr;
    BarChartWidget* barChartWidget_ = nullptr;
};

// ============================================================================
// 3. 血氧趋势卡片
// ============================================================================
class SpO2TrendCard : public SleepBaseCard {
    Q_OBJECT
public:
    explicit SpO2TrendCard(QWidget* parent = nullptr);
    void setData(const QList<QPointF>& trendPoints, const QList<qreal>& distributionPercentages);

protected:
    void updateThemeStyles(const QVariantMap& theme, const QVariantMap& font) override;

private:
    HistoryWaveformWidget* waveformWidget_ = nullptr;
    PieChartWidget* pieChartWidget_ = nullptr;
};

// ============================================================================
// 4. 体位卡片
// ============================================================================
class BodyPositionCard : public SleepBaseCard {
    Q_OBJECT
public:
    explicit BodyPositionCard(QWidget* parent = nullptr);
    void setData(const QList<StageData>& positionData, const QList<qreal>& piePercentages);

protected:
    void updateThemeStyles(const QVariantMap& theme, const QVariantMap& font) override;

private:
    StagingWidget* stagingWidget_ = nullptr;
    PieChartWidget* pieChartWidget_ = nullptr;
};

// ============================================================================
// 横向卡片大面板组件 (Dashboard)
// ============================================================================
class SleepDashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit SleepDashboardWidget(QWidget* parent = nullptr);
    ~SleepDashboardWidget();

    SleepStagingCard* stagingCard() const { return stagingCard_; }
    RespiratoryEventsCard* respiratoryCard() const { return respiratoryCard_; }
    SpO2TrendCard* spo2Card() const { return spo2Card_; }
    BodyPositionCard* bodyPositionCard() const { return bodyPositionCard_; }
    void loadRecord(const QString& identifier);
    void stopRecording();

private:
    void updateCardsWithHistoryData();

private slots:
    void onFileRead(const QString& identifier, const QByteArray& data);
    void onEnabledToggled(bool checked);
    void onBytesChanged(int value);
    void onThemeChanged();
    void processSleepStagingData(const int16_t* samples, int count);
    void processSpO2Data(const int16_t* samples, int count);
    void processRespiratoryData(const int16_t* samples, int count, int code);
    void processBodyPositionData(const int16_t* samples, int count);
    void pollDeviceData();
    void onReadResponded(int id, int serialCode, QByteArray arr, QByteArray responseData);

private:
    void generateInitialFakeData();

private:
    SleepStagingCard* stagingCard_ = nullptr;
    RespiratoryEventsCard* respiratoryCard_ = nullptr;
    SpO2TrendCard* spo2Card_ = nullptr;
    BodyPositionCard* bodyPositionCard_ = nullptr;

    QPushButton* switchEnabledBtn_ = nullptr;
    QSpinBox* bytesSpinBox_ = nullptr;

    // 核心会话数据成员变量 - 用于存下当次对话/会话的所有历史数据
    QList<StageData> stagingHistory_;
    QList<StageData> positionHistory_;
    QList<QPointF> spo2TrendPoints_;

    // 呼吸历史数据状态维护
    int normalCount_ = 0;
    int hypopneaCount_ = 0;
    int apneaCount_ = 0;
    QList<StageData> apneaHistory_;
    QList<StageData> obstructiveHistory_;
    QList<StageData> centralHistory_;
    qreal currentRespTime_ = 0.0;

    int timeIndex_ = 0;
    int deviceId_ = 0;
    QTimer* pollTimer_ = nullptr;

    quint8 lastEnabledChannels_ = 0;
};