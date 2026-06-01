#pragma once

#include <QWidget>
#include <QList>
#include <QColor>
#include <QMouseEvent>
#include <QResizeEvent>

struct StageData {
    qsizetype stageIndex;
    qreal startDuration = -1.;
    qreal persistDuration = -1.;
};

// require startDuration + persistDuration == next one's startDuration.
class
#if defined(RINGAPP_TOOL_BASIC_EXPORT)
    Q_DECL_EXPORT
#else 
    Q_DECL_IMPORT
#endif
StagingWidget: public QWidget{
    Q_OBJECT;
    Q_PROPERTY(QColor axisColor READ axisColor WRITE setAxisColor);
    Q_PROPERTY(QColor xLabelColor READ xLabelColor WRITE setXLabelColor);
    Q_PROPERTY(QColor yLabelColor READ yLabelColor WRITE setYLabelColor);
    Q_PROPERTY(bool enableYAxis READ enableYAxis WRITE setEnableYAxis);
    Q_PROPERTY(bool enableYLabel READ enableYLabel WRITE setEnableYLabel);
    Q_PROPERTY(bool enableXLabel READ enableXLabel WRITE setEnableXLabel);
    Q_PROPERTY(bool barClickable READ barClickable WRITE setBarClickable);

public:
    explicit StagingWidget(QWidget* parent = nullptr);
    ~StagingWidget() override = default;

    void setData(QList<StageData> data);

    void setBarColors(const QList<QColor>& colors);
    QList<QColor> barColors() const { return barColors_; }

    QColor axisColor() const { return axisColor_; }
    void setAxisColor(const QColor& color);

    bool enableYAxis() const { return enableYAxis_; }
    void setEnableYAxis(bool enable);

    bool enableYLabel() const { return enableYLabel_; }
    void setEnableYLabel(bool enable);

    bool enableXLabel() const { return enableXLabel_; }
    void setEnableXLabel(bool enable);

    bool barClickable() const { return barClickable_; }
    void setBarClickable(bool clickable);

    void setXLabel(QList<QString> labels);
    void setYLabel(QList<QString> labels);

    QColor xLabelColor() const { return xLabelColor_; }
    void setXLabelColor(QColor color);

    QColor yLabelColor() const { return yLabelColor_; }
    void setYLabelColor(QColor color);

signals:
    void barClicked(qsizetype dataIndex, qsizetype stageIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    struct RenderRect {
        QRectF rect;
        qsizetype dataIndex;
        qsizetype stageIndex;
    };

    void updateLayout();

private:
    QList<StageData> mergedData_;
    QList<RenderRect> cachedRects_;

    QList<QString> yLabels_;
    QList<QString> xLabels_;
    QList<QColor> barColors_;

    QColor xLabelColor_ = QColor(180, 180, 180, 120);
    QColor yLabelColor_ = QColor(180, 180, 180, 120);
    QColor axisColor_ = QColor(180, 180, 180, 120);

    bool enableYAxis_ = false;
    bool enableYLabel_ = true;
    bool enableXLabel_ = true;
    bool enableXAxis_ = true;
    bool barClickable_ = true;

    qreal barWidth_ = 25.;
    qreal maxDuration_ = -1.0;
};

class
#if defined(RINGAPP_TOOL_BASIC_EXPORT)
    Q_DECL_EXPORT
#else 
    Q_DECL_IMPORT
#endif
EventWidget: public QWidget{
    Q_OBJECT;
    Q_PROPERTY(QColor axisColor READ axisColor WRITE setAxisColor);
    Q_PROPERTY(QColor xLabelColor READ xLabelColor WRITE setXLabelColor);
    Q_PROPERTY(QColor yLabelColor READ yLabelColor WRITE setYLabelColor);
    Q_PROPERTY(qreal barWidth READ barWidth WRITE setBarWidth);
    Q_PROPERTY(bool enableXAxis READ enableXAxis WRITE setEnableXAxis);
    Q_PROPERTY(bool enableYAxis READ enableYAxis WRITE setEnableYAxis);
    Q_PROPERTY(bool enableXLabel READ enableXLabel WRITE setEnableXLabel);
    Q_PROPERTY(bool enableYLabel READ enableYLabel WRITE setEnableYLabel);
    Q_PROPERTY(bool barClickable READ barClickable WRITE setBarClickable);

public:
    explicit EventWidget(QWidget* parent = nullptr);

    void setValues(const QList<QList<StageData>>& values);
    void setXLabels(const QList<QString>& labels);
    void setYLabels(const QList<QString>& labels);

    qreal barWidth() const { return barWidth_; }
    void setBarWidth(qreal width);
    void setMaxDuration(qreal duration);

    void setAxesEnabled(bool x, bool y);
    void setLabelsEnabled(bool x, bool y);

    QColor axisColor() const { return axisColor_; }
    void setAxisColor(const QColor& color);

    QColor xLabelColor() const { return xLabelColor_; }
    void setXLabelColor(const QColor& color);

    QColor yLabelColor() const { return yLabelColor_; }
    void setYLabelColor(const QColor& color);

    bool enableXAxis() const { return enableXAxis_; }
    void setEnableXAxis(bool enable);

    bool enableYAxis() const { return enableYAxis_; }
    void setEnableYAxis(bool enable);

    bool enableXLabel() const { return enableXLabel_; }
    void setEnableXLabel(bool enable);

    bool enableYLabel() const { return enableYLabel_; }
    void setEnableYLabel(bool enable);

    bool barClickable() const { return barClickable_; }
    void setBarClickable(bool clickable);

signals:
    void barClicked(qsizetype dataIndex, qsizetype stageIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    struct RenderRect {
        QRectF rect;
        qsizetype dataIndex;
        qsizetype stageIndex;
    };

    void updateMaxDuration();
    void updateLayout();
    QColor getColorForStage(qsizetype stageIndex) const;

private:
    QList<QList<StageData>> values_;
    QList<RenderRect> cachedRects_;
    QRectF plotRect_;

    QList<QString> yLabels_;
    QList<QString> xLabels_;

    QColor xLabelColor_ = QColor(180, 180, 180);
    QColor yLabelColor_ = QColor(180, 180, 180);
    QColor axisColor_ = QColor(180, 180, 180);

    bool enableYAxis_ = false;
    bool enableYLabel_ = true;
    bool enableXLabel_ = true;
    bool enableXAxis_ = true;
    bool barClickable_ = true;

    qreal barWidth_ = 25;
    qreal maxDuration = -1.0;
};