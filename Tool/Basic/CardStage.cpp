#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QStyleOption>
#include <algorithm>

#include "CardStage.hpp"

// ==========================================
// StagingWidget Implementation
// ==========================================

StagingWidget::StagingWidget(QWidget* parent)
    : QWidget(parent) {
    // 允许通过 StyleSheet 控制背景。如果想保留透明层级，用户可以在 QSS 里写 background: transparent;
    // this->setAttribute(Qt::WA_TranslucentBackground);
    // this->setAutoFillBackground(false);
}

void StagingWidget::setData(QList<StageData> data) {
    if (data.isEmpty()) {
        mergedData_.clear();
        cachedRects_.clear();
        this->maxDuration_ = 1.0;
        this->update();
        return;
    }

    ::std::sort(data.begin(), data.end(),
        [](const StageData& a, const StageData& b) {
            return a.startDuration < b.startDuration;
        });

    mergedData_.clear();
    for (auto begin{ 0u }, end{ 1u }; begin < data.size(); ) {
        auto& cur = data.at(begin);
        qreal persistTime = cur.persistDuration;
        end = begin + 1;
        while (end < data.size() && data.at(end).stageIndex == cur.stageIndex)
            persistTime += data.at(end++).persistDuration;

        mergedData_.append(StageData{
            .stageIndex{cur.stageIndex},
            .startDuration{cur.startDuration},
            .persistDuration{persistTime}
            });
        begin = end;
    }

    this->updateLayout();
}

void StagingWidget::updateLayout() {
    cachedRects_.clear();
    qsizetype stageCount = barColors_.size();

    if (stageCount > 0 && !mergedData_.isEmpty()) {
        int leftMargin = enableYLabel_ ? 75 : 20;
        int rightMargin = 30;
        int topMargin = 30;
        int bottomMargin = enableXLabel_ ? 40 : 20;

        int drawWidth = width() - leftMargin - rightMargin;
        int drawHeight = height() - topMargin - bottomMargin;

        if (drawWidth > 0 && drawHeight > 0) {
            qreal maxTime = 0.0;
            for (const auto& item : mergedData_) {
                maxTime = qMax(maxTime, item.startDuration + item.persistDuration);
            }
            if (maxTime <= 0) maxTime = 1.0;
            this->maxDuration_ = maxTime;

            qreal rowHeight = static_cast<qreal>(drawHeight) / stageCount;
            qreal barThickness = qMin(rowHeight * 0.35, 24.0);

            for (qsizetype i = 0; i < mergedData_.size(); ++i) {
                const auto& d = mergedData_.at(i);

                qreal xStart = leftMargin + (d.startDuration / maxTime) * drawWidth;
                qreal xEnd = leftMargin + ((d.startDuration + d.persistDuration) / maxTime) * drawWidth;
                qreal yCenter = topMargin + (d.stageIndex + 0.5) * rowHeight;

                QRectF barRect(xStart, yCenter - barThickness / 2.0, xEnd - xStart, barThickness);
                cachedRects_.append({ barRect, i, d.stageIndex });
            }
        }
    }
    this->update();
}

void StagingWidget::setBarColors(const QList<QColor>& colors) {
    barColors_ = colors;
    this->updateLayout();
}

void StagingWidget::setAxisColor(const QColor& color) {
    if (axisColor_ != color) { axisColor_ = color; this->update(); }
}

void StagingWidget::setEnableYAxis(bool enable) {
    if (enableYAxis_ != enable) { enableYAxis_ = enable; this->update(); }
}

void StagingWidget::setEnableYLabel(bool enable) {
    if (enableYLabel_ != enable) { enableYLabel_ = enable; this->updateLayout(); }
}

void StagingWidget::setEnableXLabel(bool enable) {
    if (enableXLabel_ != enable) { enableXLabel_ = enable; this->updateLayout(); }
}

void StagingWidget::setBarClickable(bool clickable) {
    barClickable_ = clickable;
}

void StagingWidget::setXLabel(QList<QString> labels) {
    xLabels_ = labels;
    this->update();
}

void StagingWidget::setYLabel(QList<QString> labels) {
    yLabels_ = labels;
    this->update();
}

void StagingWidget::setXLabelColor(QColor color) {
    if (xLabelColor_ != color) { xLabelColor_ = color; this->update(); }
}

void StagingWidget::setYLabelColor(QColor color) {
    if (yLabelColor_ != color) { yLabelColor_ = color; this->update(); }
}

void StagingWidget::paintEvent(QPaintEvent* /*event*/) {
    // 关键点：让常规 StyleSheet (如 background-color, border) 生效
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    qsizetype stageCount = barColors_.size();
    if (stageCount == 0 || cachedRects_.isEmpty()) return;

    int leftMargin = enableYLabel_ ? 75 : 20;
    int rightMargin = 30;
    int topMargin = 30;
    int bottomMargin = enableXLabel_ ? 40 : 20;

    int drawWidth = width() - leftMargin - rightMargin;
    int drawHeight = height() - topMargin - bottomMargin;

    if (drawWidth <= 0 || drawHeight <= 0) return;

    qreal rowHeight = static_cast<qreal>(drawHeight) / stageCount;
    qreal maxTime = maxDuration_;
    if (maxTime <= 0) maxTime = 1.0;

    // 1. 绘制背景参考虚线及 Y 轴标签
    for (qsizetype i = 0; i < stageCount; ++i) {
        qreal yCenter = topMargin + (i + 0.5) * rowHeight;

        QColor dashColor = barColors_[i];
        dashColor.setAlpha(40);
        QPen dashPen(dashColor, 1.5, Qt::DashLine);
        painter.setPen(dashPen);
        painter.drawLine(QPointF(leftMargin, yCenter), QPointF(width() - rightMargin, yCenter));

        if (enableYLabel_) {
            painter.setPen(yLabelColor_);
            QRectF labelRect(10, yCenter - rowHeight / 2, leftMargin - 20, rowHeight);
            QString yText = (i < yLabels_.size()) ? yLabels_.at(i) : QString("Stage %1").arg(i + 1);
            painter.drawText(labelRect, Qt::AlignRight | Qt::AlignVCenter, yText);
        }
    }

    // 2. 绘制连接曲线
    for (qsizetype i = 0; i < cachedRects_.size() - 1; ++i) {
        const auto& cur = cachedRects_[i];
        const auto& nxt = cachedRects_[i + 1];

        auto delta = nxt.stageIndex - cur.stageIndex;
        auto colorD = 1.0 / qAbs(delta);
        int sign = delta < 0 ? -1 : 1;

        qreal x1 = cur.rect.right() - 2;
        qreal y1 = delta > 0 ? cur.rect.bottom() + 2 : cur.rect.top() - 2;

        qreal x2 = nxt.rect.left() + 2;
        qreal y2 = delta > 0 ? nxt.rect.top() - 2 : nxt.rect.bottom() + 2;

        if (x2 >= x1 && cur.stageIndex != nxt.stageIndex) {
            QPainterPath path;
            path.moveTo(x1, y1);

            qreal controlDeltaX = (x2 - x1) * 0.5;
            path.cubicTo(x1 + controlDeltaX, y1, x2 - controlDeltaX, y2, x2, y2);

            QLinearGradient gradient(QPointF(x1, y1), QPointF(x2, y2));

            for (qsizetype index{ 0 }; cur.stageIndex + sign * index != nxt.stageIndex; ++index)
                gradient.setColorAt(colorD * index, barColors_.value(cur.stageIndex + sign * index));
            gradient.setColorAt(1.0, barColors_.value(nxt.stageIndex));

            qreal curveThickness = this->barWidth_ * 0.05;
            QPen curvePen(QBrush(gradient), curveThickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

            painter.setPen(curvePen);
            painter.drawPath(path);
        }
    }

    // 3. 直接通过缓存的数据渲染矩形块
    for (const auto& item : cachedRects_) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(barColors_[item.stageIndex]);
        qreal barThickness = item.rect.height();
        painter.drawRoundedRect(item.rect, barThickness / 4.0, barThickness / 4.0);
    }

    // 4. 绘制 X/Y 轴及刻度
    qreal axisY = height() - bottomMargin;
    QPen axisPen(axisColor_, 1.5);
    painter.setPen(axisPen);

    if (enableYAxis_) {
        painter.drawLine(QPointF(leftMargin, topMargin * 0.5), QPointF(leftMargin, axisY));
    }
    if (enableXAxis_) {
        painter.drawLine(QPointF(leftMargin, axisY), QPointF(width() - rightMargin, axisY));
    }

    if (enableXLabel_) {
        int segments = !xLabels_.isEmpty() ? xLabels_.size() - 1 : 5;
        if (segments < 1) segments = 1;

        for (int i = 0; i <= segments; ++i) {
            qreal t = (maxTime / segments) * i;
            qreal x = leftMargin + (static_cast<qreal>(i) / segments) * drawWidth;

            painter.setPen(axisPen);
            painter.drawLine(QPointF(x, axisY), QPointF(x, axisY + 4));

            painter.setPen(xLabelColor_);
            QRectF textRect(x - 40, axisY + 8, 80, 20);

            QString xText = (!xLabels_.isEmpty() && i < xLabels_.size())
                ? xLabels_.at(i)
                : QString::number(t, 'f', 1);
            painter.drawText(textRect, Qt::AlignCenter, xText);
        }
    }
}

void StagingWidget::mousePressEvent(QMouseEvent* event) {
    if (barClickable_ && event->button() == Qt::LeftButton) {
        QPointF localPos = event->position();
        for (const auto& item : cachedRects_) {
            if (item.rect.contains(localPos)) {
                emit barClicked(item.dataIndex, item.stageIndex);
                event->accept();
                return;
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void StagingWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    this->updateLayout();
}


// ==========================================
// EventWidget Implementation
// ==========================================

EventWidget::EventWidget(QWidget* parent)
    : QWidget(parent) {
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAutoFillBackground(false);
}

void EventWidget::setValues(const QList<QList<StageData>>& values) {
    values_ = values;
    this->updateMaxDuration();
    this->updateLayout();
    this->update();
}

void EventWidget::setXLabels(const QList<QString>& labels) {
    xLabels_ = labels;
    this->updateLayout();
    this->update();
}

void EventWidget::setYLabels(const QList<QString>& labels) {
    yLabels_ = labels;
    this->updateLayout();
    this->update();
}

void EventWidget::setBarWidth(qreal width) {
    barWidth_ = width;
    this->updateLayout();
    this->update();
}

void EventWidget::setMaxDuration(qreal duration) {
    maxDuration = duration;
    if (maxDuration <= 0) {
        this->updateMaxDuration();
    }
    this->updateLayout();
    this->update();
}

void EventWidget::setAxesEnabled(bool x, bool y) {
    enableXAxis_ = x;
    enableYAxis_ = y;
    this->update();
}

void EventWidget::setLabelsEnabled(bool x, bool y) {
    enableXLabel_ = x;
    enableYLabel_ = y;
    this->updateLayout();
    this->update();
}

void EventWidget::setAxisColor(const QColor& color) {
    if (axisColor_ != color) { axisColor_ = color; this->update(); }
}

void EventWidget::setXLabelColor(const QColor& color) {
    if (xLabelColor_ != color) { xLabelColor_ = color; this->update(); }
}

void EventWidget::setYLabelColor(const QColor& color) {
    if (yLabelColor_ != color) { yLabelColor_ = color; this->update(); }
}

void EventWidget::setEnableXAxis(bool enable) {
    if (enableXAxis_ != enable) { enableXAxis_ = enable; this->update(); }
}

void EventWidget::setEnableYAxis(bool enable) {
    if (enableYAxis_ != enable) { enableYAxis_ = enable; this->update(); }
}

void EventWidget::setEnableXLabel(bool enable) {
    if (enableXLabel_ != enable) { enableXLabel_ = enable; this->updateLayout(); }
}

void EventWidget::setEnableYLabel(bool enable) {
    if (enableYLabel_ != enable) { enableYLabel_ = enable; this->updateLayout(); }
}

void EventWidget::setBarClickable(bool clickable) {
    barClickable_ = clickable;
}

void EventWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    this->updateLayout();
}

void EventWidget::updateLayout() {
    cachedRects_.clear();

    if (values_.isEmpty()) {
        plotRect_ = QRectF();
        return;
    }

    QFontMetrics fm(font());
    int leftMargin = 20;
    int bottomMargin = 20;
    int topMargin = 20;
    int rightMargin = 20;

    if (enableYLabel_ && !yLabels_.isEmpty()) {
        int maxW = 0;
        for (const auto& label : yLabels_) {
            maxW = qMax(maxW, fm.horizontalAdvance(label));
        }
        leftMargin += maxW;
    }

    if (enableXLabel_ && !xLabels_.isEmpty()) {
        bottomMargin += fm.height();
    }

    plotRect_ = rect().adjusted(leftMargin, topMargin, -rightMargin, -bottomMargin);
    if (plotRect_.width() <= 0 || plotRect_.height() <= 0) {
        return;
    }

    qreal rowHeight = plotRect_.height() / values_.size();

    for (qsizetype i = 0; i < values_.size(); ++i) {
        qreal rowTop = plotRect_.top() + i * rowHeight;
        qreal rowCenterY = rowTop + rowHeight / 2.0;

        for (const auto& stage : values_[i]) {
            if (stage.startDuration < 0 || stage.persistDuration <= 0) {
                continue;
            }

            qreal startX = plotRect_.left() + (stage.startDuration / maxDuration) * plotRect_.width();
            qreal width = (stage.persistDuration / maxDuration) * plotRect_.width();
            qreal actualBarWidth = qMin(barWidth_, rowHeight * 0.8);
            qreal barTop = rowCenterY - actualBarWidth / 2.0;
            QRectF barRect(startX, barTop, width, actualBarWidth);
            cachedRects_.append(RenderRect{ barRect, static_cast<int>(i), static_cast<int>(stage.stageIndex) });
        }
    }
}

void EventWidget::updateMaxDuration() {
    if (maxDuration > 0) return;

    qreal calculatedMax = 0.0;
    for (const auto& row : values_) {
        for (const auto& stage : row) {
            if (stage.startDuration >= 0 && stage.persistDuration > 0) {
                calculatedMax = qMax(calculatedMax, stage.startDuration + stage.persistDuration);
            }
        }
    }
    maxDuration = calculatedMax > 0 ? calculatedMax : 100.0;
}

QColor EventWidget::getColorForStage(qsizetype stageIndex) const {
    int hue = (stageIndex * 137) % 360;
    return QColor::fromHsv(hue, 180, 220);
}

void EventWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    // 关键点：让常规 StyleSheet (如 background-color, border) 生效
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    if (plotRect_.width() <= 0 || plotRect_.height() <= 0 || values_.isEmpty()) {
        return;
    }

    QFontMetrics fm = painter.fontMetrics();

    // 1. 绘制 X 轴标签
    if (enableXLabel_ && !xLabels_.isEmpty()) {
        painter.setPen(xLabelColor_);
        qsizetype xCount = xLabels_.size();
        qreal step = plotRect_.width() / (xCount > 1 ? (xCount - 1) : 1);

        for (qsizetype i = 0; i < xCount; ++i) {
            qreal xPos = plotRect_.left() + i * step;
            QRectF textRect(xPos - 50, plotRect_.bottom() + 5, 100, fm.height());
            painter.drawText(textRect, Qt::AlignCenter, xLabels_[i]);
        }
    }

    // 2. 绘制 Y 轴标签
    qreal rowHeight = plotRect_.height() / values_.size();
    if (enableYLabel_ && !yLabels_.isEmpty()) {
        painter.setPen(yLabelColor_);
        for (qsizetype i = 0; i < values_.size(); ++i) {
            if (i < yLabels_.size()) {
                qreal rowTop = plotRect_.top() + i * rowHeight;
                QRectF textRect(0, rowTop, plotRect_.left() - 10, rowHeight);
                painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, yLabels_[i]);
            }
        }
    }

    // 3. 绘制行背景参考虚线
    QColor dashColor = axisColor_;
    dashColor.setAlpha(40);
    QPen dashPen(dashColor, 1.0, Qt::DashLine);
    painter.setPen(dashPen);

    for (qsizetype i = 0; i < values_.size(); ++i) {
        qreal rowCenterY = plotRect_.top() + (i + 0.5) * rowHeight;
        painter.drawLine(QPointF(plotRect_.left(), rowCenterY),
            QPointF(plotRect_.right(), rowCenterY));
    }

    // 4. 直接通过缓存的数据渲染矩形块
    for (const auto& item : cachedRects_) {
        QColor baseColor = getColorForStage(item.stageIndex);
        painter.setBrush(baseColor);
        painter.setPen(baseColor.darker(120));
        painter.drawRoundedRect(item.rect, 4, 4);
    }

    // 5. 绘制 X/Y 轴线
    painter.setPen(QPen(axisColor_, 1.5));
    if (enableXAxis_) {
        painter.drawLine(plotRect_.bottomLeft(), plotRect_.bottomRight());
    }
    if (enableYAxis_) {
        painter.drawLine(plotRect_.topLeft(), plotRect_.bottomLeft());
    }
}

void EventWidget::mousePressEvent(QMouseEvent* event) {
    if (!barClickable_) {
        QWidget::mousePressEvent(event);
        return;
    }

    QPointF clickPos = event->position();

    for (auto it = cachedRects_.rbegin(); it != cachedRects_.rend(); ++it) {
        if (it->rect.contains(clickPos)) {
            emit barClicked(it->dataIndex, it->stageIndex);
            event->accept();
            return;
        }
    }

    QWidget::mousePressEvent(event);
}