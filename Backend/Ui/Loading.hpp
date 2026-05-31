#pragma once

#include <QWidget>

class QVariantAnimation;
class QProgressBar;
class QGraphicsEffect;
class QGraphicsDropShadowEffect;
class LoadingWidget : public QWidget {
    Q_OBJECT;
public:
    explicit LoadingWidget(QWidget* parent = nullptr);

    void fadeIn();
    void fadeOut();

    void startLoading(const QVariantMap& params);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void animateTo(qreal value);

private slots:
    void updateTheme();

private:
    QFrame* panel_ = nullptr;
    QLabel* icon_ = nullptr;
    QLabel* title_ = nullptr;
    QLabel* subtitle_ = nullptr;
    QProgressBar* progress_ = nullptr;
};