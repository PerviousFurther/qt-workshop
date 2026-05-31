#pragma once

#include <QWidget>
#include <QList>
#include <QTimer>

#include "Tool/Basic/CardWaveform.hpp"
#include "Tool/Basic/CardStage.hpp"
#include "Tool/Basic/CardPieChart.hpp"
#include "Tool/Basic/CardBarChart.hpp"

class QVBoxLayout;
class QHBoxLayout;
class QLabel;

class PSGMainPage : public QWidget {
    Q_OBJECT
public:
    explicit PSGMainPage(QWidget* parent = nullptr);
    ~PSGMainPage() override = default;

private:
    void initLayout();
    void initStyleSheet();
    void generateMockData();

private:
    // ---- 左侧时序监控区组件 ----
    HistoryWaveformWidget* waveformMonitor_ = nullptr;
    StagingWidget* hypnogramChart_ = nullptr;
    EventWidget* respiratoryEvents_ = nullptr;

    // ---- 右侧统计分析区组件 ----
    PieChartWidget* stagePieChart_ = nullptr;
    BarChartWidget* positionBarChart_ = nullptr;
    BarChartWidget* apneaBarChart_ = nullptr;

    QTimer* mockDataTimer_ = nullptr;
    int tickCount_ = 0;
};