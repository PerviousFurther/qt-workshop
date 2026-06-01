// CardWaveform.cpp
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QStyleOption> // 新增：支持样式表属性刷新
#include <cmath>
#include <limits>
#include <algorithm>

#include "CardWaveform.hpp"

namespace {
    constexpr qreal kSelectDistance = 15.0;
}

// ============================================================================
// BASE CLASS IMPLEMENTATION
// ============================================================================
AbstractWaveformWidget::AbstractWaveformWidget(QWidget* parent) : QWidget(parent) {
    this->setAutoFillBackground(false);
    // this->setAttribute(Qt::WA_OpaquePaintEvent, false);
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    this->setMouseTracking(true);
}

void AbstractWaveformWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updatePlotRect();
}

void AbstractWaveformWidget::addCurve(const QList<QPointF>& points, const QColor& color, qreal width) {
    data_.append(points);
    lineColors_.append(color);
    lineWidths_.append(width);
    curveVisibility_.append(true);

    xMin_.append(0.0); xMax_.append(1.0);
    yMin_.append(0.0); yMax_.append(1.0);
    viewXMin_.append(0.0); viewXMax_.append(1.0);
    viewYMin_.append(0.0); viewYMax_.append(1.0);

    updateCurveBounds(data_.size() - 1);
    updatePlotRect();
    resetView();
}

void AbstractWaveformWidget::removeCurve(qsizetype index) {
    if (index < 0 || index >= data_.size()) return;
    data_.removeAt(index);
    lineColors_.removeAt(index);
    lineWidths_.removeAt(index);
    curveVisibility_.removeAt(index);
    xMin_.removeAt(index); xMax_.removeAt(index);
    yMin_.removeAt(index); yMax_.removeAt(index);
    viewXMin_.removeAt(index); viewXMax_.removeAt(index);
    viewYMin_.removeAt(index); viewYMax_.removeAt(index);
    selectedCurves_.removeAll(index);
    for (auto& idx : selectedCurves_) { if (idx > index) idx--; }

    updatePlotRect();
    update();
}

void AbstractWaveformWidget::clearCurves() {
    data_.clear(); lineColors_.clear(); lineWidths_.clear(); curveVisibility_.clear();
    xMin_.clear(); xMax_.clear(); yMin_.clear(); yMax_.clear();
    viewXMin_.clear(); viewXMax_.clear(); viewYMin_.clear(); viewYMax_.clear();
    selectedCurves_.clear();

    updatePlotRect();
    update();
}

QList<QPointF> AbstractWaveformWidget::curveData(qsizetype index) const {
    return (index >= 0 && index < data_.size()) ? data_[index] : QList<QPointF>();
}

void AbstractWaveformWidget::setCurveData(qsizetype index, const QList<QPointF>& points) {
    if (index >= 0 && index < data_.size()) {
        data_[index] = points;
        updateCurveBounds(index);
        update();
    }
}

QColor AbstractWaveformWidget::curveColor(qsizetype index) const {
    return (index >= 0 && index < lineColors_.size()) ? lineColors_[index] : QColor();
}

void AbstractWaveformWidget::setCurveColor(qsizetype index, const QColor& color) {
    if (index >= 0 && index < lineColors_.size()) { lineColors_[index] = color; update(); }
}

qreal AbstractWaveformWidget::curveWidth(qsizetype index) const {
    return (index >= 0 && index < lineWidths_.size()) ? lineWidths_[index] : 0.0;
}

void AbstractWaveformWidget::setCurveWidth(qsizetype index, qreal width) {
    if (index >= 0 && index < lineWidths_.size()) { lineWidths_[index] = width; update(); }
}

bool AbstractWaveformWidget::isCurveVisible(qsizetype index) const {
    return (index >= 0 && index < curveVisibility_.size()) ? curveVisibility_[index] : false;
}

void AbstractWaveformWidget::setCurveVisible(qsizetype index, bool visible) {
    if (index >= 0 && index < curveVisibility_.size()) {
        curveVisibility_[index] = visible;
        updatePlotRect();
        update();
    }
}

void AbstractWaveformWidget::setTitle(const QString& title) {
    title_ = title;
    updatePlotRect();
    update();
}

void AbstractWaveformWidget::setBorderColor(const QColor& color) {
    if (borderColor_ != color) {
        borderColor_ = color;
        enableBorder_ = true;
        update();
    }
}

void AbstractWaveformWidget::setCornerRadius(qreal radius) {
    qreal validRadius = qMax(0.0, radius);
    if (!qFuzzyCompare(cornerRadius_, validRadius)) {
        cornerRadius_ = validRadius;
        update();
    }
}

void AbstractWaveformWidget::setBackgroundColor(const QColor& background) {
    if (bkgColor_ != background) {
        bkgColor_ = background;
        enableBackground_ = true;
        update();
    }
}

void AbstractWaveformWidget::setGridColor(const QColor& color) {
    if (gridColor_ != color) {
        gridColor_ = color;
        update();
    }
}

void AbstractWaveformWidget::setTextColor(const QColor& color) {
    if (textColor_ != color) {
        textColor_ = color;
        update();
    }
}

void AbstractWaveformWidget::selectCurve(qsizetype index) {
    if (!allowCurveSelect_) return;
    if (index >= 0 && index < data_.size() && !selectedCurves_.contains(index)) {
        selectedCurves_.append(index);
        update();
    }
}

void AbstractWaveformWidget::clearSelection() {
    selectedCurves_.clear();
    update();
}

void AbstractWaveformWidget::resetView() {
    for (qsizetype i = 0; i < data_.size(); ++i) {
        viewXMin_[i] = xMin_[i]; viewXMax_[i] = xMax_[i];
        viewYMin_[i] = yMin_[i]; viewYMax_[i] = yMax_[i];
        if (qFuzzyCompare(viewXMin_[i], viewXMax_[i])) viewXMax_[i] += 1.0;
        if (qFuzzyCompare(viewYMin_[i], viewYMax_[i])) viewYMax_[i] += 1.0;
    }
    update();
}

void AbstractWaveformWidget::updateCurveBounds(qsizetype idx) {
    if (idx < 0 || idx >= data_.size() || data_[idx].isEmpty()) return;
    qreal xmin = data_[idx].first().x(), xmax = xmin;
    qreal ymin = data_[idx].first().y(), ymax = ymin;
    for (const auto& p : data_[idx]) {
        xmin = qMin(xmin, p.x()); xmax = qMax(xmax, p.x());
        ymin = qMin(ymin, p.y()); ymax = qMax(ymax, p.y());
    }
    xMin_[idx] = xmin; xMax_[idx] = xmax;
    yMin_[idx] = ymin; yMax_[idx] = ymax;
}

QPointF AbstractWaveformWidget::mapFromScene(const QPointF& scenePos, qsizetype curveIdx) const {
    if (curveIdx < 0 || curveIdx >= data_.size()) return QPointF();

    qreal xRatio = (scenePos.x() - viewXMin_[curveIdx]) / (viewXMax_[curveIdx] - viewXMin_[curveIdx]);
    qreal yRatio = (scenePos.y() - viewYMin_[curveIdx]) / (viewYMax_[curveIdx] - viewYMin_[curveIdx]);
    return QPointF(plotRect_.left() + xRatio * plotRect_.width(), plotRect_.bottom() - (yRatio * plotRect_.height()));
}

QPointF AbstractWaveformWidget::mapToScene(const QPoint& widgetPos, qsizetype curveIdx) const {
    if (curveIdx < 0 || curveIdx >= data_.size()) return QPointF();

    qreal xRatio = static_cast<qreal>(widgetPos.x() - plotRect_.left()) / plotRect_.width();
    qreal yRatio = static_cast<qreal>(plotRect_.bottom() - widgetPos.y()) / plotRect_.height();
    qreal xScene = viewXMin_[curveIdx] + xRatio * (viewXMax_[curveIdx] - viewXMin_[curveIdx]);
    qreal yScene = viewYMin_[curveIdx] + yRatio * (viewYMax_[curveIdx] - viewYMin_[curveIdx]);
    return QPointF(xScene, yScene);
}

void AbstractWaveformWidget::paintEvent(QPaintEvent*) {
    QStyleOption opt;
    opt.initFrom(this);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Render modern rounded card container background & border
    QRectF cardRect = rect();
    if (enableBorder_)
        painter.setPen(QPen(borderColor_, 1.0));
    if (enableBackground_)
        painter.setBrush(bkgColor_);
    if (enableBackground_ || enableBorder_)
        painter.drawRoundedRect(cardRect.adjusted(0.5, 0.5, -0.5, -0.5), cornerRadius_, cornerRadius_);

    // Render card title if specified
    if (!title_.isEmpty()) {
        painter.setPen(textColor_);
        QFont titleFont = painter.font();
        titleFont.setBold(true);
        titleFont.setPointSize(10);
        painter.setFont(titleFont);
        painter.drawText(15, 22, title_);
    }

    if (gridEnabled_)
        drawCustomGrids(painter);
    drawCustomAxes(painter);

    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) return;

    painter.save();
    painter.setClipRect(plotRect_);

    for (qsizetype i = 0; i < data_.size(); ++i) {
        if (!curveVisibility_[i] || data_[i].size() < 2) continue;

        bool selected = selectedCurves_.contains(i);
        QColor color = selected ? lineColors_[i].lighter(150) : lineColors_[i];
        qreal w = selected ? lineWidths_[i] * 2.5 : lineWidths_[i];
        painter.setPen(QPen(color, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        QPainterPath path;
        path.moveTo(mapFromScene(data_[i].first(), i));
        for (int j = 1; j < data_[i].size(); ++j) {
            path.lineTo(mapFromScene(data_[i][j], i));
        }
        painter.drawPath(path);
    }
    painter.restore();
}

void AbstractWaveformWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton) return;
    isPressed_ = true;
    lastMousePos_ = event->pos();
}

void AbstractWaveformWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!isPressed_ || !allowDrag_) return;
    QPoint delta = event->pos() - lastMousePos_;
    lastMousePos_ = event->pos();

    updateViewportOnPan(delta);
    update();
}

void AbstractWaveformWidget::mouseReleaseEvent(QMouseEvent* event) {
    isPressed_ = false;
    if (!allowCurveSelect_) return;

    qsizetype bestIdx = -1;
    qreal bestDist = std::numeric_limits<qreal>::max();

    for (qsizetype i = 0; i < data_.size(); ++i) {
        if (!curveVisibility_[i]) continue;
        for (const auto& p : data_[i]) {
            qreal d = QLineF(mapFromScene(p, i), event->position()).length();
            if (d < bestDist) { bestDist = d; bestIdx = i; }
        }
    }

    if (bestDist <= kSelectDistance) {
        clearSelection();
        selectCurve(bestIdx);
    }
    else {
        clearSelection();
    }
}

void AbstractWaveformWidget::wheelEvent(QWheelEvent* event) {
    if (!allowZoom_) return;
    qreal factor = (event->angleDelta().y() > 0) ? 0.85 : 1.15;
    updateViewportOnZoom(event->position().toPoint(), factor);
    update();
}

void AbstractWaveformWidget::setXStep(quint32 step) {
    if (xStep_ != step && step > 0) { xStep_ = step; update(); }
}

void AbstractWaveformWidget::setYStep(quint32 step) {
    if (yStep_ != step && step > 0) { yStep_ = step; update(); }
}

// ============================================================================
// CONCRETE IMPLEMENTATION 1: HistoryWaveformWidget
// ============================================================================
HistoryWaveformWidget::HistoryWaveformWidget(QWidget* parent) : AbstractWaveformWidget(parent) {}

void HistoryWaveformWidget::updatePlotRect() {
    int activeAxes = 0;
    for (qsizetype i = 0; i < curveVisibility_.size(); ++i) { if (curveVisibility_[i]) activeAxes++; }

    int leftMargin = (activeAxes > 0) ? (45 + 25 * (activeAxes - 1)) : 25;
    int bottomMargin = (activeAxes > 0) ? (35 + 25 * (activeAxes - 1)) : 35;
    int topMargin = title_.isEmpty() ? 25 : 45;

    plotRect_ = QRect(leftMargin, topMargin, width() - leftMargin - 40, height() - bottomMargin - topMargin);
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) plotRect_ = rect();
}

void HistoryWaveformWidget::drawCustomGrids(QPainter& painter) {
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) return;

    painter.setPen(QPen(gridColor_, 1.0, Qt::DashLine));
    for (quint32 i = 1; i < yStep_; ++i) {
        int y = plotRect_.top() + (plotRect_.height() * i) / yStep_;
        painter.drawLine(plotRect_.left(), y, plotRect_.right(), y);
    }
    for (quint32 i = 1; i < xStep_; ++i) {
        int x = plotRect_.left() + (plotRect_.width() * i) / xStep_;
        painter.drawLine(x, plotRect_.top(), x, plotRect_.bottom());
    }
}

void HistoryWaveformWidget::drawCustomAxes(QPainter& painter) {
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) return;

    int activeAxes = 0;
    for (qsizetype i = 0; i < data_.size(); ++i) { if (curveVisibility_[i]) activeAxes++; }

    QFont axisFont = painter.font();
    axisFont.setPointSize(8);
    painter.setFont(axisFont);

    int axisOffset = 0;
    for (qsizetype i = 0; i < data_.size(); ++i) {
        if (!curveVisibility_[i]) continue;
        auto color = lineColors_.value(i);
        color.setAlpha(120);
        painter.setPen(QPen(color, 1.2));

        int currentXAxisOffset = plotRect_.left() - 25 * (activeAxes - 1 - axisOffset);
        painter.drawLine(currentXAxisOffset, plotRect_.top(), currentXAxisOffset, plotRect_.bottom());
        painter.drawText(currentXAxisOffset - 10, plotRect_.top() - 8, QString("Y%1").arg(i));

        for (quint32 j = 0; j <= yStep_; ++j) {
            int y = plotRect_.top() + (plotRect_.height() * j) / yStep_;
            painter.drawLine(currentXAxisOffset, y, currentXAxisOffset - 4, y);

            qreal val = viewYMax_[i] - (j * (viewYMax_[i] - viewYMin_[i]) / yStep_);
            QString label = QString::number(val, 'g', 4);

            QRectF textR(currentXAxisOffset - 35, y - 6, 30, 12);
            painter.drawText(textR, Qt::AlignRight | Qt::AlignVCenter, label);
        }

        int currentYAxisOffset = plotRect_.bottom() + 25 * (activeAxes - 1 - axisOffset);
        painter.drawLine(plotRect_.left(), currentYAxisOffset, plotRect_.right(), currentYAxisOffset);
        painter.drawText(plotRect_.right() + 8, currentYAxisOffset + 4, QString("X%1").arg(i));

        for (quint32 j = 0; j <= xStep_; ++j) {
            int x = plotRect_.left() + (plotRect_.width() * j) / xStep_;
            painter.drawLine(x, currentYAxisOffset, x, currentYAxisOffset + 4);

            qreal val = viewXMin_[i] + (j * (viewXMax_[i] - viewXMin_[i]) / xStep_);
            QString label = QString::number(val, 'g', 4);

            QRectF textR(x - 20, currentYAxisOffset + 6, 40, 12);
            painter.drawText(textR, Qt::AlignCenter, label);
        }

        axisOffset++;
    }
}

void HistoryWaveformWidget::updateViewportOnPan(const QPoint& delta) {
    int pWidth = qMax(1, plotRect_.width());
    int pHeight = qMax(1, plotRect_.height());

    bool hasSelection = !selectedCurves_.isEmpty();
    for (qsizetype i = 0; i < data_.size(); ++i) {
        if (hasSelection && !selectedCurves_.contains(i)) continue;

        qreal dx = (static_cast<qreal>(delta.x()) / pWidth) * (viewXMax_[i] - viewXMin_[i]);
        qreal dy = (static_cast<qreal>(delta.y()) / pHeight) * (viewYMax_[i] - viewYMin_[i]);
        viewXMin_[i] -= dx; viewXMax_[i] -= dx;
        viewYMin_[i] += dy; viewYMax_[i] += dy;
    }
}

void HistoryWaveformWidget::updateViewportOnZoom(const QPoint& mousePos, qreal factor) {
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) return;

    bool hasSelection = !selectedCurves_.isEmpty();
    for (qsizetype i = 0; i < data_.size(); ++i) {
        if (hasSelection && !selectedCurves_.contains(i)) continue;

        QPointF anchor = mapToScene(mousePos, i);
        qreal newXRange = (viewXMax_[i] - viewXMin_[i]) * factor;
        qreal newYRange = (viewYMax_[i] - viewYMin_[i]) * factor;

        qreal xRatio = static_cast<qreal>(mousePos.x() - plotRect_.left()) / plotRect_.width();
        qreal yRatio = static_cast<qreal>(plotRect_.bottom() - mousePos.y()) / plotRect_.height();

        viewXMin_[i] = anchor.x() - xRatio * newXRange;
        viewXMax_[i] = viewXMin_[i] + newXRange;
        viewYMin_[i] = anchor.y() - yRatio * newYRange;
        viewYMax_[i] = viewYMin_[i] + newYRange;
    }
}

// ============================================================================
// CONCRETE IMPLEMENTATION 2: RecordWaveformWidget
// ============================================================================
RecordWaveformWidget::RecordWaveformWidget(QWidget* parent) : AbstractWaveformWidget(parent) {}

void RecordWaveformWidget::updatePlotRect() {
    int activeAxes = 0;
    for (qsizetype i = 0; i < curveVisibility_.size(); ++i) { if (curveVisibility_[i]) activeAxes++; }

    int leftMargin = (activeAxes > 0) ? (45 + 25 * (activeAxes - 1)) : 45;
    int topMargin = title_.isEmpty() ? 20 : 40;

    plotRect_ = QRect(leftMargin, topMargin, width() - leftMargin - 65, height() - topMargin - 35);
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) plotRect_ = rect();
}

void RecordWaveformWidget::startRecording() {
    startTime_ = QDateTime::currentDateTime();
    isRecording_ = true;
    clearCurves();
}

void RecordWaveformWidget::stopRecording() {
    isRecording_ = false;
}

void RecordWaveformWidget::appendRecordPoint(qsizetype curveIdx, qreal yValue) {
    if (!isRecording_ || curveIdx < 0 || curveIdx >= data_.size()) return;

    qreal elapsedSeconds = startTime_.msecsTo(QDateTime::currentDateTime()) / 1000.0;
    data_[curveIdx].append(QPointF(elapsedSeconds, yValue));
    updateCurveBounds(curveIdx);

    if (stickyMode_) {
        adjustStickyOffset();
    }
    else {
        update();
    }
}

void RecordWaveformWidget::setStickyMode(bool enabled) {
    stickyMode_ = enabled;
    if (stickyMode_) adjustStickyOffset();
    update();
}

void RecordWaveformWidget::adjustStickyOffset() {
    if (data_.isEmpty()) return;

    qreal latestX = 0.0;
    for (qsizetype i = 0; i < data_.size(); ++i) {
        if (!data_[i].isEmpty()) latestX = qMax(latestX, xMax_[i]);
    }

    qreal targetXMax = latestX;
    qreal targetXMin = qMax(0.0, latestX - timeWindowWidth_);

    for (qsizetype i = 0; i < data_.size(); ++i) {
        viewXMin_[i] = targetXMin;
        viewXMax_[i] = targetXMax;
    }
    update();
}

void RecordWaveformWidget::drawCustomGrids(QPainter& painter) {
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) return;

    painter.setPen(QPen(gridColor_, 1.0, Qt::DashLine));
    for (quint32 i = 1; i < xStep_; ++i) {
        int x = plotRect_.left() + (plotRect_.width() * i) / xStep_;
        painter.drawLine(x, plotRect_.top(), x, plotRect_.bottom());
    }
    for (quint32 i = 1; i < yStep_; ++i) {
        int y = plotRect_.top() + (plotRect_.height() * i) / yStep_;
        painter.drawLine(plotRect_.left(), y, plotRect_.right(), y);
    }
}

void RecordWaveformWidget::drawCustomAxes(QPainter& painter) {
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) return;

    int activeAxes = 0;
    for (qsizetype i = 0; i < data_.size(); ++i) { if (curveVisibility_[i]) activeAxes++; }

    QFont axisFont = painter.font();
    axisFont.setPointSize(8);
    painter.setFont(axisFont);

    painter.setPen(QPen(textColor_, 1.2));
    painter.drawLine(plotRect_.left(), plotRect_.bottom(), plotRect_.right(), plotRect_.bottom());
    painter.drawText(plotRect_.right() + 8, plotRect_.bottom() + 4, "Time(s)");

    if (data_.size() > 0) {
        for (quint32 j = 0; j <= xStep_; ++j) {
            int x = plotRect_.left() + (plotRect_.width() * j) / xStep_;
            painter.drawLine(x, plotRect_.bottom(), x, plotRect_.bottom() + 4);

            qreal val = viewXMin_[0] + (j * (viewXMax_[0] - viewXMin_[0]) / xStep_);
            QString label = QString::number(val, 'f', 1);

            QRectF textR(x - 20, plotRect_.bottom() + 6, 40, 12);
            painter.drawText(textR, Qt::AlignCenter, label);
        }
    }

    int axisOffset = 0;
    for (qsizetype i = 0; i < data_.size(); ++i) {
        if (!curveVisibility_[i]) continue;
        painter.setPen(QPen(lineColors_[i], 1.2));

        int currentX = plotRect_.left() - 25 * (activeAxes - 1 - axisOffset);
        painter.drawLine(currentX, plotRect_.top(), currentX, plotRect_.bottom());
        painter.drawText(currentX - 10, plotRect_.top() - 8, QString("Y%1").arg(i));

        for (quint32 j = 0; j <= yStep_; ++j) {
            int y = plotRect_.top() + (plotRect_.height() * j) / yStep_;
            painter.drawLine(currentX, y, currentX - 4, y);

            qreal val = viewYMax_[i] - (j * (viewYMax_[i] - viewYMin_[i]) / yStep_);
            QString label = QString::number(val, 'g', 4);

            QRectF textR(currentX - 35, y - 6, 30, 12);
            painter.drawText(textR, Qt::AlignRight | Qt::AlignVCenter, label);
        }

        axisOffset++;
    }
}

void RecordWaveformWidget::updateViewportOnPan(const QPoint& delta) {
    if (stickyMode_) stickyMode_ = false;

    int pWidth = qMax(1, plotRect_.width());
    int pHeight = qMax(1, plotRect_.height());

    qreal sharedXDelta = (static_cast<qreal>(delta.x()) / pWidth) * (viewXMax_[0] - viewXMin_[0]);
    for (qsizetype i = 0; i < data_.size(); ++i) {
        viewXMin_[i] -= sharedXDelta;
        viewXMax_[i] -= sharedXDelta;

        qreal dy = (static_cast<qreal>(delta.y()) / pHeight) * (viewYMax_[i] - viewYMin_[i]);
        viewYMin_[i] += dy;
        viewYMax_[i] += dy;
    }
}

void RecordWaveformWidget::updateViewportOnZoom(const QPoint& mousePos, qreal factor) {
    if (stickyMode_) stickyMode_ = false;
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) return;

    timeWindowWidth_ *= factor;

    for (qsizetype i = 0; i < data_.size(); ++i) {
        QPointF anchor = mapToScene(mousePos, i);
        qreal xRatio = static_cast<qreal>(mousePos.x() - plotRect_.left()) / plotRect_.width();
        qreal yRatio = static_cast<qreal>(plotRect_.bottom() - mousePos.y()) / plotRect_.height();

        qreal newXRange = (viewXMax_[i] - viewXMin_[i]) * factor;
        viewXMin_[i] = anchor.x() - xRatio * newXRange;
        viewXMax_[i] = viewXMin_[i] + newXRange;

        qreal newYRange = (viewYMax_[i] - viewYMin_[i]) * factor;
        viewYMin_[i] = anchor.y() - yRatio * newYRange;
        viewYMax_[i] = viewYMin_[i] + newYRange;
    }
}