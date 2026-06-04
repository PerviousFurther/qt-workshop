#include <utility>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QPainter>
#include <QPainterPath>
#include <QVariantMap>
#include "Core/Utils.hpp"
#include "Session.hpp" 
#include "PageSetting.hpp"
#include "PageSettingPrivate.hpp"

// ============================================================================
// PageSetting 类成员实现
// ============================================================================

PageSetting::PageSetting(QWidget* parent) : Extensible(parent) {
    this->setAttribute(Qt::WA_StyledBackground);
    this->setProperty("iconText", lstr("⚙"));
    this->setProperty("title", lstr("设置"));

    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    contentLayout_ = new QHBoxLayout();
    contentLayout_->setSpacing(16);

    mainLayout->addLayout(contentLayout_);
    this->setLayout(mainLayout);

    auto* session = UiSession::instance();
    connect(session, &UiSession::themeChanged, this, &PageSetting::applyTheme);
    connect(session, &UiSession::fontChanged, this, &PageSetting::applyTheme);

    applyTheme();
}

void PageSetting::applyTheme() {
    auto* session = UiSession::instance();
    if (!session) return;

    auto currentTheme = session->theme();
    auto fontMap = session->font();

    QString bgStart = currentTheme.value(UiField::Background, lstr("#F8FAFC")).toString();
    QString bgEnd = currentTheme.value(UiField::Surface, lstr("#E2E8F0")).toString();
    QString border = currentTheme.value(UiField::Border, lstr("#CBD5E1")).toString();
    QString textPrimary = currentTheme.value(UiField::TextPrimary, lstr("#1E293B")).toString();

    QString fontFamily = fontMap.value(UiField::Family, lstr("Microsoft YaHei")).toString();
    int sizeNormal = fontMap.value(UiField::SizeNormal, 14).toInt();

    this->setStyleSheet(lstr(
        "PageSetting {"
        "    background: qlineargradient("
        "        x1: 0, y1: 0, x2: 1, y2: 1,"
        "        stop: 0 %1, "
        "        stop: 1 %2"
        "    );"
        "    border: 1px solid %3;"
        "    border-radius: 12px;"
        "    font-family: '%4';"
        "    font-size: %5px;"
        "    color: %6;"
        "}"
    ).arg(bgStart, bgEnd, border, fontFamily).arg(sizeNormal).arg(textPrimary));
}

qsizetype PageSetting::appendWidget(QWidget* widget) {
    if (!widget) return -1;

    int stretch = 0;
    assign(stretch, widget->property("stretch"));
    widget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    contentLayout_->addWidget(widget, stretch);
    return Extensible::appendWidget(widget);
}

QWidget* PageSetting::removeWidget(qsizetype index) noexcept {
    if (index < 0 || index >= widgets_.size()) return nullptr;

    QWidget* widget = widgets_.at(index);
    if (widget) {
        contentLayout_->removeWidget(widget);
    }

    return Extensible::removeWidget(index);
}