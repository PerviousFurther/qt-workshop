#pragma once

#include <QWidget>
#include <QList>
#include <QString>
#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QStyleOption>


// Recommand use qss rather than use setter/getter directly.
class
#if defined(RINGAPP_TOOL_BASIC_EXPORT)
    Q_DECL_EXPORT
#else 
    Q_DECL_IMPORT
#endif
BarChartWidget: public QWidget{
    Q_OBJECT;

    // These property can be use from qss.

    Q_PROPERTY(bool enableBackground READ enableBackground WRITE setEnableBackground);
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor);
    Q_PROPERTY(bool enableXAxis READ enableXAxis WRITE setEnableXAxis);
    Q_PROPERTY(bool enableYAxis READ enableYAxis WRITE setEnableYAxis);
    Q_PROPERTY(bool enableXLabel READ enableXLabel WRITE setEnableXLabel);
    Q_PROPERTY(bool enableYLabel READ enableYLabel WRITE setEnableYLabel);
    Q_PROPERTY(bool enableHGrid READ enableHGrid WRITE setEnableHGrid);
    Q_PROPERTY(bool enableVGrid READ enableVGrid WRITE setEnableVGrid);
    Q_PROPERTY(bool allowSelect READ allowSelect WRITE setAllowSelect);
    Q_PROPERTY(bool selectIsSeriesIds READ selectIsSeriesIds WRITE setSelectIsSeriesIds);
    Q_PROPERTY(qreal seriesGap READ seriesGap WRITE setSeriesGap);
    Q_PROPERTY(qreal seriesInternalGap READ seriesInternalGap WRITE setSeriesInternalGap);
    Q_PROPERTY(qreal yMin READ yMin WRITE setYMin);
    Q_PROPERTY(qreal yMax READ yMax WRITE setYMax);

public:
    explicit BarChartWidget(QWidget * parent = nullptr);

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
    bool allowSelect() const { return allowSelect_; }

    void setSelectIsSeriesIds(bool isSeries);
    bool selectIsSeriesIds() const { return selectIsSeriesIds_; }

    QList<qsizetype> selectedIds() const { return selectedIds_; }
    void clearSelection();

    // === 新增：为 QSS 对应的独立 setter/getter ===
    qreal seriesGap() const { return seriesGap_; }
    void setSeriesGap(qreal gap);

    qreal seriesInternalGap() const { return seriesInteralGap_; }
    void setSeriesInternalGap(qreal gap);

    qreal yMin() const { return yMin_; }
    void setYMin(qreal yMin);

    qreal yMax() const { return yMax_; }
    void setYMax(qreal yMax);

signals:
    void selectionChanged(const QList<qsizetype>& selectedIds);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateBarGeometry();
    QRectF getChartRect() const; // 新增：内部自适应 QSS 边距的绘图区域计算

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
    bool enableHGrid_ = true;
    bool enableVGrid_ = true;
    bool enableBackground_ = false;
    bool selectIsSeriesIds_ = true;
    bool allowSelect_ = true;
};