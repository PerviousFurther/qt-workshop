#pragma once

#include <QWidget>
#include <QList>
#include <QString>
#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

class 
#if defined(RINGAPP_TOOL_BASIC_EXPORT)
    Q_DECL_EXPORT
#else 
    Q_DECL_IMPORT
#endif
BarChartWidget : public QWidget {
    Q_OBJECT;
public:
    explicit BarChartWidget(QWidget* parent = nullptr);

    void setData(const QList<QList<qreal>>& values, const QList<QList<QString>>& labels = {});
    void setSeriesColors(const QList<QColor>& colors);
    void setYBounds(qreal yMin, qreal yMax);
    void setGaps(qreal seriesGap, qreal internalGap);

    void setEnableBackground(bool enable);
    bool enableBackground() const { return enableBackground_; }
    void setBackgroundColor(const QColor& color);
    QColor backgroundColor() const { return backgroundColor_; }

    void setEnableXAxis(bool enable);
    bool enableXAxis() const { return enableXAxis_; }

    void setEnableYAxis(bool enable);
    bool enableYAxis() const { return enableYAxis_; }

    void setEnableXLabel(bool enable);
    bool enableXLabel() const { return enableXLabel_; }

    void setEnableYLabel(bool enable);
    bool enableYLabel() const { return enableYLabel_; }

    void setEnableHGrid(bool enable);
    bool enableHGrid() const { return enableHGrid_; }

    void setEnableVGrid(bool enable);
    bool enableVGrid() const { return enableVGrid_; }

    void setAllowSelect(bool allow);
    void setSelectIsSeriesIds(bool isSeries);
    QList<qsizetype> selectedIds() const { return selectedIds_; }
    void clearSelection();

signals:
    void selectionChanged(const QList<qsizetype>& selectedIds);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateBarGeometry();

private:
    QList<QList<QString>> labels_;
    QList<QList<qreal>> values_;
    QList<QList<QRectF>> barRects_;
    QList<qsizetype> selectedIds_;
    QList<QColor> seriesColor_;

    qreal seriesGap_ = 15.0;
    qreal seriesInteralGap_ = 3.0;
    qreal yMax_ = 100.0;
    qreal yMin_ = 0.0;

    QColor backgroundColor_;
    
    bool enableYAxis_ = true;
    bool enableXAxis_ = true;
    bool enableYLabel_ = true;
    bool enableXLabel_ = true;
    // dash line inside chart.
    bool enableHGrid_ = true;
    bool enableVGrid_ = true;
    bool enableBackground_ = false;
    bool selectIsSeriesIds_ = true;
    bool allowSelect_ = true;
};