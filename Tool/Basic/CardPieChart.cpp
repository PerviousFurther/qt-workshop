#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QtMath>

#include "CardPieChart.hpp"

PieChartWidget::PieChartWidget(QWidget* parent)
    : QWidget(parent)
    , backgroundColor_(Qt::transparent)
    , holeRadius_(0.0)
    , enableSelected_(true) {
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAutoFillBackground(false);
}

void PieChartWidget::setData(const QList<qreal>& percentages, const QList<QColor>& colors) {
    if (!percentages.isEmpty())
        percentages_ = percentages;
    if (!colors.isEmpty())
        colors_ = colors;
    Q_ASSERT_X(percentages_.size() <= colors_.size(), 
        "PieChartWidget::setData", "size of data should equal size of colors");
    selectedIds_.clear();
    this->update();
}

void PieChartWidget::setHoleRadius(qreal holeRadius) {
    holeRadius_ = qBound(0.0, holeRadius, 0.9);
    this->update();
}

void PieChartWidget::setEnableSelected(bool enable) {
    enableSelected_ = enable;
    if (!enableSelected_) {
        selectedIds_.clear();
        this->update();
    }
}

void PieChartWidget::setEnableBackground(bool enable) {
    if (enableBackground_ != enable) {
        enableBackground_ = enable;
        this->update();
    }
}

void PieChartWidget::setBackgroundColor(const QColor& color) {
    if (backgroundColor_ != color) {
        backgroundColor_ = color;
        if (enableBackground_) {
            this->update();
        }
    }
}

QList<qsizetype> PieChartWidget::selectedIds() const {
    return selectedIds_;
}

void PieChartWidget::clearSelection() {
    if (!selectedIds_.isEmpty()) {
        selectedIds_.clear();
        emit selectionChanged(selectedIds_);
        this->update();
    }
}

void PieChartWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (enableBackground_) {
        painter.fillRect(this->rect(), backgroundColor_);
    } else {
        painter.fillRect(this->rect(), Qt::GlobalColor::transparent);
    }

    if (percentages_.isEmpty()) return;

    qreal total = 0;
    for (qreal p : percentages_) total += p;
    if (total <= 0) return;

    int side = qMin(width(), height());
    qreal outerRadius = (side - 40) / 2.0;
    qreal innerRadius = outerRadius * holeRadius_;
    QPointF center(width() / 2.0, height() / 2.0);

    qreal startAngle = 0.0;

    for (qsizetype i = 0; i < percentages_.size(); ++i) {
        qreal sweepAngle = (percentages_[i] / total) * 360.0;
        QColor color = (i < colors_.size()) ? colors_[i] : QColor(Qt::gray);

        bool isSelected = selectedIds_.contains(i);
        QPointF sliceCenter = center;
        if (isSelected && enableSelected_) {
            qreal midAngleRad = qDegreesToRadians(startAngle + sweepAngle / 2.0);
            qreal offset = 15.0;
            sliceCenter += QPointF(offset * qCos(midAngleRad), -offset * qSin(midAngleRad));
        }

        QRectF outerRect(sliceCenter.x() - outerRadius, sliceCenter.y() - outerRadius, outerRadius * 2, outerRadius * 2);
        QRectF innerRect(sliceCenter.x() - innerRadius, sliceCenter.y() - innerRadius, innerRadius * 2, innerRadius * 2);

        QPainterPath path;
        if (innerRadius > 0) {
            path.arcMoveTo(outerRect, startAngle);
            path.arcTo(outerRect, startAngle, sweepAngle);
            path.arcTo(innerRect, startAngle + sweepAngle, -sweepAngle);
            path.closeSubpath();
        }
        else {
            path.moveTo(sliceCenter);
            path.arcTo(outerRect, startAngle, sweepAngle);
            path.closeSubpath();
        }

        painter.setPen(QPen(Qt::white, 1.5));
        painter.setBrush(color);
        painter.drawPath(path);

        startAngle += sweepAngle;
    }
}

void PieChartWidget::mousePressEvent(QMouseEvent* event) {
    if (!enableSelected_) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        qsizetype clickedIndex = getSliceIndexAt(event->pos());
        if (clickedIndex != -1) {
            if (selectedIds_.contains(clickedIndex)) {
                selectedIds_.removeOne(clickedIndex);
            }
            else {
                selectedIds_.clear();
                selectedIds_.append(clickedIndex);
            }

            emit sliceClicked(clickedIndex);
            emit selectionChanged(selectedIds_);
            this->update();
        }
    }
}

qsizetype PieChartWidget::getSliceIndexAt(const QPoint& pos) const {
    if (percentages_.isEmpty()) return -1;

    qreal total = 0;
    for (qreal p : percentages_) total += p;
    if (total <= 0) return -1;

    int side = qMin(width(), height());
    qreal outerRadius = (side - 40) / 2.0;
    qreal innerRadius = outerRadius * holeRadius_;
    QPointF center(width() / 2.0, height() / 2.0);

    qreal currentAngle = 0.0;
    for (qsizetype i = 0; i < percentages_.size(); ++i) {
        qreal sweepAngle = (percentages_[i] / total) * 360.0;

        QPointF sliceCenter = center;
        if (selectedIds_.contains(i) && enableSelected_) {
            qreal midAngleRad = qDegreesToRadians(currentAngle + sweepAngle / 2.0);
            qreal offset = 15.0;
            sliceCenter += QPointF(offset * qCos(midAngleRad), -offset * qSin(midAngleRad));
        }

        qreal dx = pos.x() - sliceCenter.x();
        qreal dy = sliceCenter.y() - pos.y();
        qreal distance = qSqrt(dx * dx + dy * dy);

        if (distance >= innerRadius && distance <= outerRadius) {
            qreal angle = qRadiansToDegrees(qAtan2(dy, dx));
            if (angle < 0) angle += 360.0;

            if (angle >= currentAngle && angle < (currentAngle + sweepAngle)) {
                return i;
            }
        }

        currentAngle += sweepAngle;
    }

    return -1;
}