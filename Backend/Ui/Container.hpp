#pragma once

#include <QProperty>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QHBoxLayout>
#include "Backend/Ui/Extensible.hpp"

class UiSession;
class QEvent;
class Container : public Extensible {
    Q_OBJECT
        Q_PROPERTY(qint32 currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged BINDABLE bindableCurrentIndex)

public:
    explicit Container(QWidget* parent = nullptr);
    ~Container() override = default;

    void fadeIn();
    void fadeOut();

    int  currentIndex() const;
    void setCurrentIndex(int idx);
    QBindable<int> bindableCurrentIndex();

signals:
    void currentIndexChanged(int);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    qsizetype appendWidget(QWidget* widget) override;
    QWidget* removeWidget(qsizetype index) noexcept override;
    void setCurrentTabbarIndex(int idx);
    void onScrollValueChanged(int value);

private:
    void syncLayout(bool setScroll = false);
    void applyTheme();
    void updateTransforms(int scrollOffset);
    bool eventFilter(QObject* watcher, QEvent* e) override;
private:
    inline static UiSession* ui_ = nullptr;

    QProperty<int> currentIndex_{ 0 };

    // == 核心控件（现代化重构） ========================
    QScrollArea* scrollArea_ = nullptr;
    QWidget* scrollContent_ = nullptr;
    QHBoxLayout* contentLayout_ = nullptr;

    QWidget* navContainer_ = nullptr;

    // QList<QWidget*>     pages_;
    QList<QPushButton*> navButtons_;
    QList<QLabel*>      pageTitles_;

    bool isInternalScrolling_ = false;
    bool isDrag_ = false;
};