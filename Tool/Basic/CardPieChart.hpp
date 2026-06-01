#pragma once

#include <QWidget>
#include <QList>
#include <QColor>

// Recommand use stylesheet instead of directly call setter or getter.

class
#if defined(RINGAPP_TOOL_BASIC_EXPORT)
    Q_DECL_EXPORT
#else 
    Q_DECL_IMPORT
#endif
PieChartWidget: public QWidget{
    Q_OBJECT

    Q_PROPERTY(qreal holeRadius READ holeRadius WRITE setHoleRadius)
    Q_PROPERTY(bool enableSelected READ enableSelected WRITE setEnableSelected)
    Q_PROPERTY(bool enableBackground READ enableBackground WRITE setEnableBackground)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)

public:
    explicit PieChartWidget(QWidget * parent = nullptr);
    void setData(const QList<qreal>& percentages, const QList<QColor>& colors = {});

    void setHoleRadius(qreal holeRadius);
    qreal holeRadius() const { return holeRadius_; }

    void setEnableSelected(bool enable);
    bool enableSelected() const { return enableSelected_; }

    void clearSelection();
    QList<qsizetype> selectedIds() const;

    void setEnableBackground(bool enable);
    bool enableBackground() const { return enableBackground_; }
    void setBackgroundColor(const QColor& color);
    QColor backgroundColor() const { return backgroundColor_; }

    qsizetype count() const noexcept { return colors_.size(); }
    QList<qreal> percentages() const { return percentages_; }

signals:
    void sliceClicked(qsizetype index);
    void selectionChanged(const QList<qsizetype>& selectedIds);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    qsizetype getSliceIndexAt(const QPoint& pos) const;

private:
    QList<QColor> colors_;
    QList<qreal> percentages_;
    QList<qsizetype> selectedIds_;

    QColor backgroundColor_;
    qreal holeRadius_ = 1.0;
    bool enableSelected_ = false;
    bool enableBackground_ = false;
};