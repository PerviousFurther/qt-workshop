#pragma once 

#include <QVector>
#include "Extensible.hpp"

class QHBoxLayout;
class PageSetting : public Extensible {
    Q_OBJECT
public:
    explicit PageSetting(QWidget* parent = nullptr);

    qsizetype appendWidget(QWidget* widget) override;
    QWidget* removeWidget(qsizetype index) noexcept override;

private:
    auto findEmptySlot(int rowSpan, int colSpan);
    void markOccupied(int startRow, int startCol, int rowSpan, int colSpan, bool occupied);

private slots:
    void applyTheme();

private:
    QHBoxLayout* contentLayout_{ nullptr };
    QVector<QVector<bool>> gridOccupancy_;
};