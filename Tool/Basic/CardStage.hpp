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

// 单个 Stage 序列小部件.
// Require each stageData can be connected. 
// (fronter one's startDuration + persistDuration == next's startDuration)
// if data have any gap, the behavior is undefined.
class 
#if defined(RINGAPP_TOOL_BASIC_EXPORT)
    Q_DECL_EXPORT
#else 
    Q_DECL_IMPORT
#endif
StagingWidget : public QWidget {
    Q_OBJECT;
    Q_PROPERTY(QColor axisColor READ axisColor WRITE setAxisColor);
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
    void setXLabelColor(QColor color);
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

    QColor backgroundColor_ = Qt::transparent;
    QColor xLabelColor_ = QColor(180, 180, 180, 120);
    QColor yLabelColor_ = QColor(180, 180, 180, 120);
    QColor axisColor_ = QColor(180, 180, 180, 120);

    bool enableYAxis_ = false;
    bool enableYLabel_ = true;
    bool enableXLabel_ = true;
    bool enableXAxis_ = true;
    bool barClickable_ = true;
    bool enableBackground_ = false; // 默认不绘制背景

    qreal barWidth_ = 25.;
    qreal maxDuration_ = -1.0;
};

// 多组事件并发或对比小部件
class
#if defined(RINGAPP_TOOL_BASIC_EXPORT)
    Q_DECL_EXPORT
#else 
    Q_DECL_IMPORT
#endif
EventWidget : public QWidget{
    Q_OBJECT;
public:
    explicit EventWidget(QWidget* parent = nullptr);

    void setValues(const QList<QList<StageData>>& values);
    void setXLabels(const QList<QString>& labels);
    void setYLabels(const QList<QString>& labels);

    void setBarWidth(qreal width);
    void setMaxDuration(qreal duration);
    void setAxesEnabled(bool x, bool y);
    void setLabelsEnabled(bool x, bool y);
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

    QColor backgroundColor_ = Qt::transparent;
    QColor xLabelColor_ = QColor(180, 180, 180);
    QColor yLabelColor_ = QColor(180, 180, 180);
    QColor axisColor_ = QColor(180, 180, 180);

    bool enableYAxis_ = false;
    bool enableYLabel_ = true;
    bool enableXLabel_ = true;
    bool enableXAxis_ = true;
    bool barClickable_ = true;
    bool enableBackground_ = false; // 默认不绘制背景

    qreal barWidth_ = 25;
    qreal maxDuration = -1.0;
};