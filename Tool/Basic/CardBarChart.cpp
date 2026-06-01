#include "CardBarChart.hpp"

BarChartWidget::BarChartWidget(QWidget* parent) : QWidget(parent) {
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAutoFillBackground(false);
    this->setAttribute(Qt::WA_Hover, true);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->backgroundColor_ = QColor(245, 245, 245, 120);
}

QRectF BarChartWidget::getChartRect() const {
    QRectF crect = contentsRect();
    return QRectF(crect.left() + 40, crect.top() + 20, crect.width() - 60, crect.height() - 50);
}

void BarChartWidget::setData(const QList<QList<qreal>>& values, const QList<QList<QString>>& labels) {
    values_ = values;
    labels_ = labels;
    selectedIds_.clear();
    this->updateBarGeometry();
    this->update();
}

void BarChartWidget::setSeriesColors(const QList<QColor>& colors) {
    seriesColor_ = colors;
    this->update();
}

void BarChartWidget::setYBounds(qreal yMin, qreal yMax) {
    if (yMin >= yMax) return;
    yMin_ = yMin;
    yMax_ = yMax;
    this->updateBarGeometry();
    this->update();
}

void BarChartWidget::setGaps(qreal seriesGap, qreal internalGap) {
    seriesGap_ = seriesGap;
    seriesInteralGap_ = internalGap;
    this->updateBarGeometry();
    this->update();
}

void BarChartWidget::setSeriesGap(qreal gap) {
    if (seriesGap_ != gap) {
        seriesGap_ = gap;
        this->updateBarGeometry();
        this->update();
    }
}

void BarChartWidget::setSeriesInternalGap(qreal gap) {
    if (seriesInteralGap_ != gap) {
        seriesInteralGap_ = gap;
        this->updateBarGeometry();
        this->update();
    }
}

void BarChartWidget::setYMin(qreal yMin) {
    if (yMin_ != yMin && yMin < yMax_) {
        yMin_ = yMin;
        this->updateBarGeometry();
        this->update();
    }
}

void BarChartWidget::setYMax(qreal yMax) {
    if (yMax_ != yMax && yMax > yMin_) {
        yMax_ = yMax;
        this->updateBarGeometry();
        this->update();
    }
}

void BarChartWidget::setEnableBackground(bool enable) {
    if (enableBackground_ != enable) {
        enableBackground_ = enable;
        this->update();
    }
}

void BarChartWidget::setBackgroundColor(const QColor& color) {
    backgroundColor_ = color;
    if (enableBackground_) {
        this->update();
    }
}

void BarChartWidget::setAllowSelect(bool allow) {
    allowSelect_ = allow;
    if (!allow) selectedIds_.clear();
    this->update();
}

void BarChartWidget::setSelectIsSeriesIds(bool isSeries) {
    if (!isSeries && !values_.isEmpty()) {
        int count = values_.first().size();
        for (const auto& series : values_) {
            if (series.size() != count) return;
        }
    }
    selectIsSeriesIds_ = isSeries;
    selectedIds_.clear();
    this->update();
}

void BarChartWidget::setEnableXAxis(bool enable) { if (enableXAxis_ != enable) { enableXAxis_ = enable; this->update(); } }
void BarChartWidget::setEnableYAxis(bool enable) { if (enableYAxis_ != enable) { enableYAxis_ = enable; this->update(); } }
void BarChartWidget::setEnableXLabel(bool enable) { if (enableXLabel_ != enable) { enableXLabel_ = enable; this->update(); } }
void BarChartWidget::setEnableYLabel(bool enable) { if (enableYLabel_ != enable) { enableYLabel_ = enable; this->update(); } }
void BarChartWidget::setEnableHGrid(bool enable) { if (enableHGrid_ != enable) { enableHGrid_ = enable; this->update(); } }
void BarChartWidget::setEnableVGrid(bool enable) { if (enableVGrid_ != enable) { enableVGrid_ = enable; this->update(); } }

void BarChartWidget::clearSelection() {
    if (!selectedIds_.isEmpty()) {
        selectedIds_.clear();
        emit selectionChanged(selectedIds_);
        this->update();
    }
}

void BarChartWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    this->updateBarGeometry();
}

void BarChartWidget::updateBarGeometry() {
    barRects_.clear();
    if (values_.isEmpty()) return;

    int numSeries = values_.size();
    int numCategories = 0;
    for (const auto& series : values_) {
        numCategories = qMax(numCategories, (int)series.size());
    }
    if (numCategories == 0) return;

    QRectF chartRect = getChartRect();

    qreal totalGaps = (numCategories - 1) * seriesGap_ + numCategories * (numSeries - 1) * seriesInteralGap_;
    qreal barWidth = (chartRect.width() - totalGaps) / (numCategories * numSeries);
    if (barWidth <= 0) barWidth = 1;

    qreal zeroRatio = (0.0 - yMin_) / (yMax_ - yMin_);
    qreal zeroLine = chartRect.bottom() - qBound(0.0, zeroRatio, 1.0) * chartRect.height();

    barRects_.resize(numSeries);

    for (int s = 0; s < numSeries; ++s) {
        int catSize = values_[s].size();
        barRects_[s].resize(catSize);

        for (int c = 0; c < catSize; ++c) {
            qreal groupWidth = numSeries * barWidth + (numSeries - 1) * seriesInteralGap_;
            qreal x = chartRect.left() + c * (groupWidth + seriesGap_) + s * (barWidth + seriesInteralGap_);
            qreal value = values_[s][c];
            qreal valueRatio = (value - yMin_) / (yMax_ - yMin_);
            qreal yTarget = chartRect.bottom() - qBound(0.0, valueRatio, 1.0) * chartRect.height();

            barRects_[s][c] = QRectF(x, qMin(zeroLine, yTarget), barWidth, qAbs(yTarget - zeroLine));
        }
    }
}

void BarChartWidget::paintEvent(QPaintEvent* /*event*/) {
    // 核心修改：利用 QStyleOption 渲染样式表规定的背景、边框、圆角等
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(this->font());

    QRectF chartRect = getChartRect();

    // 1. 绘制内部图表背景
    if (enableBackground_) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(backgroundColor_);
        painter.drawRect(chartRect);
    }

    if (values_.isEmpty() || barRects_.isEmpty()) return;

    int numSeries = values_.size();
    int numCategories = 0;
    for (const auto& series : values_) {
        numCategories = qMax(numCategories, (int)series.size());
    }

    // 2. 绘制辅助网格线与 Y 轴刻度标签
    int ySegments = 4;
    for (int i = 0; i <= ySegments; ++i) {
        qreal val = yMin_ + i * (yMax_ - yMin_) / ySegments;
        qreal valueRatio = (val - yMin_) / (yMax_ - yMin_);
        qreal y = chartRect.bottom() - qBound(0.0, valueRatio, 1.0) * chartRect.height();

        if (enableHGrid_ && i > 0) {
            painter.setPen(QPen(QColor(200, 200, 200, 120), 1, Qt::DashLine));
            painter.drawLine(chartRect.left(), y, chartRect.right(), y);
        }

        if (enableYLabel_) {
            painter.setPen(this->palette().color(QPalette::Text));
            QString yStr = QString::number(val, 'g', 4);
            QRectF textRect(0, y - 10, chartRect.left() - 5, 20);
            painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, yStr);
        }
    }

    qreal zeroRatio = (0.0 - yMin_) / (yMax_ - yMin_);
    qreal zeroLine = chartRect.bottom() - qBound(0.0, zeroRatio, 1.0) * chartRect.height();

    painter.setPen(QPen(QColor(200, 200, 200, 120), 1, Qt::DashLine));
    painter.drawLine(chartRect.left(), zeroLine, chartRect.right(), zeroLine);

    painter.setPen(QPen(Qt::darkGray, 1, Qt::SolidLine));
    if (enableXAxis_) {
        painter.drawLine(chartRect.bottomLeft(), chartRect.bottomRight());
    }
    if (enableYAxis_) {
        painter.drawLine(chartRect.topLeft(), chartRect.bottomLeft());
    }

    bool hasSelection = !selectedIds_.isEmpty();

    // 3. 绘制柱状图及柱顶数值
    for (int s = 0; s < numSeries; ++s) {
        QColor baseColor = (s < seriesColor_.size()) ? seriesColor_[s] : QColor::fromHsv((s * 50) % 360, 200, 200);

        for (int c = 0; c < values_[s].size(); ++c) {
            if (c >= barRects_[s].size()) continue;
            QRectF bar = barRects_[s][c];

            bool isSelected = false;
            if (hasSelection) {
                isSelected = selectIsSeriesIds_ ? selectedIds_.contains(s) : selectedIds_.contains(c);
            }

            QColor finalColor = baseColor;
            finalColor.setAlpha(hasSelection && !isSelected ? 50 : 255);

            if (numSeries == 1 && c < seriesColor_.size()) {
                finalColor = seriesColor_.value(c);
            }

            painter.setPen(Qt::NoPen);
            painter.setBrush(finalColor);
            painter.drawRect(bar);

            // 绘制柱体上的具体数值
            QString valueStr = QString::number(values_[s][c], 'g', 4);
            QColor textColor = this->palette().color(QPalette::Text);
            if (hasSelection && !isSelected) {
                textColor.setAlpha(100);
            }
            painter.setPen(textColor);

            QFontMetrics fm(this->font());
            int textWidth = fm.horizontalAdvance(valueStr) + 10;
            int textHeight = fm.height();

            QPointF textPos = (values_[s][c] >= 0)
                ? QPointF(bar.center().x(), bar.top() - 5)
                : QPointF(bar.center().x(), bar.bottom() + textHeight + 5);

            QRectF textRect(textPos.x() - textWidth / 2.0,
                (values_[s][c] >= 0) ? textPos.y() - textHeight : bar.bottom() + 5,
                textWidth,
                textHeight);

            painter.drawText(textRect, Qt::AlignCenter, valueStr);
        }
    }

    // 4. 绘制 X 轴分类标签 (Labels)
    if (enableXLabel_ && numCategories > 0) {
        painter.setPen(this->palette().color(QPalette::Text));

        for (int c = 0; c < numCategories; ++c) {
            qreal minX = chartRect.right();
            qreal maxX = chartRect.left();
            bool hasValidBar = false;

            for (int s = 0; s < numSeries; ++s) {
                if (c < barRects_[s].size()) {
                    minX = qMin(minX, barRects_[s][c].left());
                    maxX = qMax(maxX, barRects_[s][c].right());
                    hasValidBar = true;
                }
            }

            if (hasValidBar) {
                QString labelStr;
                if (!labels_.isEmpty() && !labels_[0].isEmpty() && c < labels_[0].size()) {
                    labelStr = labels_[0][c];
                }
                else {
                    labelStr = QString::number(c + 1);
                }

                QRectF labelRect(minX, chartRect.bottom() + 8, maxX - minX, 20);
                painter.drawText(labelRect, Qt::AlignCenter, labelStr);
            }
        }
    }
}

void BarChartWidget::mousePressEvent(QMouseEvent* event) {
    if (!allowSelect_ || values_.isEmpty() || barRects_.isEmpty()) {
        QWidget::mousePressEvent(event);
        return;
    }

    QRectF chartRect = getChartRect();
    bool clickedAny = false;

    for (int s = 0; s < values_.size(); ++s) {
        for (int c = 0; c < values_[s].size(); ++c) {
            if (c >= barRects_[s].size()) continue;
            QRectF bar = barRects_[s][c];

            if (bar.contains(event->position())) {
                clickedAny = true;
                qsizetype targetId = selectIsSeriesIds_ ? s : c;

                if (event->modifiers() & Qt::ControlModifier) {
                    if (selectedIds_.contains(targetId)) {
                        selectedIds_.removeOne(targetId);
                    }
                    else {
                        selectedIds_.append(targetId);
                    }
                }
                else {
                    if (selectedIds_.size() == 1 && selectedIds_.contains(targetId)) {
                        selectedIds_.clear();
                    }
                    else {
                        selectedIds_.clear();
                        selectedIds_.append(targetId);
                    }
                }
                break;
            }
        }
        if (clickedAny) break;
    }

    if (!clickedAny && chartRect.contains(event->position())) {
        selectedIds_.clear();
    }

    emit selectionChanged(selectedIds_);
    this->update();
    QWidget::mousePressEvent(event);
}