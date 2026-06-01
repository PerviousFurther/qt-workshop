// CardWaveform.hpp
#pragma once

#include <QWidget>
#include <QColor>
#include <QPointF>
#include <QVector>
#include <QList>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QString>

#if defined(RINGAPP_TOOL_BASIC_EXPORT)
#define WAVEFORM_EXPORT Q_DECL_EXPORT
#else 
#define WAVEFORM_EXPORT Q_DECL_IMPORT
#endif

class QPaintEvent;
class QMouseEvent;
class QWheelEvent;
class QResizeEvent;

// ============================================================================
// BASE CLASS: AbstractWaveformWidget
// ============================================================================
class WAVEFORM_EXPORT AbstractWaveformWidget : public QWidget {
    Q_OBJECT;

    // --- 注册 Q_PROPERTY 属性，使其支持 QSS 样式表 ---
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor);
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor);
    Q_PROPERTY(QColor gridColor READ gridColor WRITE setGridColor);
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor);
    Q_PROPERTY(qreal cornerRadius READ cornerRadius WRITE setCornerRadius);

public:
    explicit AbstractWaveformWidget(QWidget* parent = nullptr);
    virtual ~AbstractWaveformWidget() override = default;

    // --- Core Data Interface ---
    void addCurve(const QList<QPointF>& points, const QColor& color = Qt::green, qreal width = 1.5);
    void removeCurve(qsizetype index);
    void clearCurves();

    // --- Getters & Setters ---
    qsizetype curveCount() const { return data_.size(); }
    QList<QPointF> curveData(qsizetype index) const;
    void setCurveData(qsizetype index, const QList<QPointF>& points);

    QColor curveColor(qsizetype index) const;
    void setCurveColor(qsizetype index, const QColor& color);

    qreal curveWidth(qsizetype index) const;
    void setCurveWidth(qsizetype index, qreal width);

    bool isCurveVisible(qsizetype index) const;
    void setCurveVisible(qsizetype index, bool visible);

    void setXStep(quint32 step);
    quint32 xStep() const noexcept { return this->xStep_; }
    void setYStep(quint32 step);
    quint32 yStep() const noexcept { return this->yStep_; }

    // --- Card Elements Interface (支持 QSS 的接口) ---
    void setTitle(const QString& title);
    QString title() const { return title_; }

    void setBorderColor(const QColor& color);
    QColor borderColor() const { return borderColor_; }

    void setCornerRadius(qreal radius);
    qreal cornerRadius() const { return cornerRadius_; }

    void setBackgroundColor(const QColor& background);
    QColor backgroundColor() const { return bkgColor_; }

    void setGridColor(const QColor& color);
    QColor gridColor() const { return gridColor_; }

    void setTextColor(const QColor& color);
    QColor textColor() const { return textColor_; }

    // --- Viewport Control Interface ---
    void selectCurve(qsizetype index);
    void clearSelection();
    void resetView();

    void setAllowDrag(bool enable) { allowDrag_ = enable; }
    void setAllowZoom(bool enable) { allowZoom_ = enable; }
    void setAllowCurveSelect(bool enable) { allowCurveSelect_ = enable; }
    void setGridVisible(bool visible) { gridEnabled_ = visible; }

protected:
    // --- Coordinate Transformation Pipeline ---
    QPointF mapFromScene(const QPointF& scenePos, qsizetype curveIdx) const;
    QPointF mapToScene(const QPoint& widgetPos, qsizetype curveIdx) const;

    // --- Qt Event Overrides ---
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // --- Virtual Hooks for Custom Rendering & Layout ---
    virtual void updatePlotRect() = 0;
    virtual void drawCustomGrids(QPainter& painter) = 0;
    virtual void drawCustomAxes(QPainter& painter) = 0;
    virtual void updateViewportOnPan(const QPoint& delta) = 0;
    virtual void updateViewportOnZoom(const QPoint& mousePos, qreal factor) = 0;

    void updateCurveBounds(qsizetype idx);

protected:
    QList<QList<QPointF>> data_;
    QVector<QColor> lineColors_;
    QVector<qreal> lineWidths_;
    QVector<bool> curveVisibility_;

    QVector<qreal> xMin_, xMax_, yMin_, yMax_;
    QVector<qreal> viewXMin_, viewXMax_, viewYMin_, viewYMax_;

    QList<qsizetype> selectedCurves_;

    QRect plotRect_;
    quint32 yStep_ = 4;
    quint32 xStep_ = 4;

    bool gridEnabled_ = true;
    bool allowDrag_ = true;
    bool allowZoom_ = true;
    bool allowCurveSelect_ = true;
    bool enableBackground_ = false;
    bool enableBorder_ = false;

    bool isPressed_ = false;
    QPoint lastMousePos_;

    // Card UI Styling Elements
    QString title_;
    QColor bkgColor_ = QColor(24, 24, 27);
    QColor borderColor_ = QColor(63, 63, 70);
    QColor gridColor_ = QColor(60, 60, 60);
    QColor textColor_ = QColor(228, 228, 231);
    qreal cornerRadius_ = 8.0;
};

// ============================================================================
// DERIVED CLASS 1: HistoryWaveformWidget (Independent X & Y Axes)
// ============================================================================
class WAVEFORM_EXPORT HistoryWaveformWidget : public AbstractWaveformWidget {
    Q_OBJECT

public:
    explicit HistoryWaveformWidget(QWidget* parent = nullptr);

protected:
    void updatePlotRect() override;
    void drawCustomGrids(QPainter& painter) override;
    void drawCustomAxes(QPainter& painter) override;
    void updateViewportOnPan(const QPoint& delta) override;
    void updateViewportOnZoom(const QPoint& mousePos, qreal factor) override;
};

// ============================================================================
// DERIVED CLASS 2: RecordWaveformWidget (Shared Time X-Axis, Tailored Y-Axes)
// ============================================================================
class WAVEFORM_EXPORT RecordWaveformWidget : public AbstractWaveformWidget {
    Q_OBJECT

public:
    explicit RecordWaveformWidget(QWidget* parent = nullptr);

    void startRecording();
    void stopRecording();
    void appendRecordPoint(qsizetype curveIdx, qreal yValue);

    void setStickyMode(bool enabled);
    bool isStickyMode() const { return stickyMode_; }

protected:
    void updatePlotRect() override;
    void drawCustomGrids(QPainter& painter) override;
    void drawCustomAxes(QPainter& painter) override;
    void updateViewportOnPan(const QPoint& delta) override;
    void updateViewportOnZoom(const QPoint& mousePos, qreal factor) override;

private:
    void adjustStickyOffset();

    QDateTime startTime_;
    bool isRecording_ = false;
    bool stickyMode_ = true;
    qreal timeWindowWidth_ = 10.0;
};