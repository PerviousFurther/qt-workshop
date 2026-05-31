#pragma once

#include <QWidget>
#include <QList>
#include <QPoint>
#include <QString>

class QLabel;
class QFrame;
class QHBoxLayout;
class QVBoxLayout;
class QGridLayout;

class HistoryWaveformWidget;
class PieChartWidget;
class BarChartWidget;
class PSGSummaryCard : public QWidget {
    Q_OBJECT;
public:
    explicit PSGSummaryCard(QWidget* parent = nullptr);
    virtual ~PSGSummaryCard() override = default;

    void updateDashboardData(const QString& duration, double ahi, int efficiency);

signals:
    void clicked();

protected:
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void initUI();
    void setupMockData();
    void syncWithSession();

private:
    QLabel* valueDuration_ = nullptr;
    QLabel* valueAHI_ = nullptr;
    QLabel* valueEfficiency_ = nullptr;

    QLabel* tipDuration_ = nullptr;
    QLabel* tipAHI_ = nullptr;
    QLabel* tipEfficiency_ = nullptr;

    PieChartWidget* pieChart_ = nullptr;
    BarChartWidget* barChart_ = nullptr;
    HistoryWaveformWidget* waveformWidget_ = nullptr;

    QLabel* statusPieCard_ = nullptr;
    QLabel* statusBarCard_ = nullptr;
    QLabel* statusWaveformCard_ = nullptr;

    QFrame* containerFrame_ = nullptr;
    QFrame* bottomBarFrame_ = nullptr;
    QList<QFrame*> subCards_;
};