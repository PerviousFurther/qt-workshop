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

    // 主布局：依然使用垂直布局包裹，或者直接用水平布局（此处保留主布局结构，内部使用水平布局）
    auto* mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // 【修改】改为横向布局 (QHBoxLayout)
    contentLayout_ = new QHBoxLayout();
    contentLayout_->setSpacing(16);

    mainLayout->addLayout(contentLayout_);
    this->setLayout(mainLayout);

    // ================= 监听全局 UiSession 信号 =================
    auto* session = UiSession::instance();
    if (session) {
        connect(session, &UiSession::themeChanged, this, &PageSetting::applyTheme);
        connect(session, &UiSession::fontChanged, this, &PageSetting::applyTheme);
    }

    // 初始化应用样式
    applyTheme();
}

void PageSetting::applyTheme() {
    auto* session = UiSession::instance();
    if (!session) return;

    // 1. 获取当前主题与字体属性
    auto currentTheme = session->theme();
    auto fontMap = session->font();

    // 提取颜色配置（如果为空，提供合理的默认降级色）
    QString bgStart = currentTheme.value(UiField::Background, lstr("#F8FAFC")).toString();
    QString bgEnd = currentTheme.value(UiField::Surface, lstr("#E2E8F0")).toString();
    QString border = currentTheme.value(UiField::Border, lstr("#CBD5E1")).toString();
    QString textPrimary = currentTheme.value(UiField::TextPrimary, lstr("#1E293B")).toString();

    // 提取字体配置
    QString fontFamily = fontMap.value(UiField::Family, lstr("Microsoft YaHei")).toString();
    int sizeNormal = fontMap.value(UiField::SizeNormal, 14).toInt();

    // 2. 动态构建并设置样式表
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

// 【修改】因为改成了横向布局，appendWidget 变得非常简单
qsizetype PageSetting::appendWidget(QWidget* widget) {
    if (!widget) return -1;

    int stretch = 0;
    assign(stretch, widget->property("stretch"));
    widget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    contentLayout_->addWidget(widget, stretch);
    return Extensible::appendWidget(widget);
}

// 【修改】移除了网格碰撞相关的解算
QWidget* PageSetting::removeWidget(qsizetype index) noexcept {
    if (index < 0 || index >= widgets_.size()) return nullptr;

    QWidget* widget = widgets_.at(index);
    if (widget) {
        // 直接从横向布局中移除
        contentLayout_->removeWidget(widget);
    }

    return Extensible::removeWidget(index);
}