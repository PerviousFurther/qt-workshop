#include <QRandomGenerator>
#include <QFont>
#include <QGuiApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QIcon>
#include <QHBoxLayout>
#include <QFontComboBox>
#include <QSlider>
#include <QSpinBox>

// #include "Backend/Network/Session.hpp"
#include "Core/Configuration.hpp"

#include "Session.hpp"
#include "SessionPrivate.hpp"

#include "Extensible.hpp"

#include "PageDevice.hpp"
#include "PageUser.hpp"
#include "PageSetting.hpp"
#include "PageAiChat.hpp"
#include "Container.hpp"
#include "MainWindow.hpp"

UiSession *UiSession::instance_ = nullptr;

static const auto MODULE_NAME = lstr("UiSession");


namespace Field {
    using namespace UiField;
}

static const QVariantMap DEFAULT_JSON = {
    {Field::Themes, QVariantList{
        // =========================
        // Soft Blue (柔和蓝)
        // =========================
        QVariantMap{
            {Field::Name,           lstr("柔和蓝")},

            {Field::Background,     lstr("#F0F4F8")},
            {Field::Surface,        lstr("#FFFFFF")},
            {Field::Border,         lstr("#D0DFEE")},
            {Field::Grid,           lstr("#E1ECF4")},

            {Field::TextPrimary,    lstr("#1E293B")},
            {Field::TextSecondary,  lstr("#64748B")},
            {Field::TextHint,       lstr("#94A3B8")},

            {Field::Primary,        lstr("#3B82F6")},
            {Field::Secondary,      lstr("#0D9488")},
            {Field::Success,        lstr("#10B981")},
            {Field::Warning,        lstr("#F59E0B")},
            {Field::Danger,         lstr("#EF4444")},
            {Field::Accent,         lstr("#60A5FA")},

            // Signal Colors
            {Field::SignalEEG,      lstr("#0EA5E9")},
            {Field::SignalECG,      lstr("#2563EB")},
            {Field::SignalResp,     lstr("#10B981")},
            {Field::SignalSpO2,     lstr("#8B5CF6")},
            {Field::SignalBody,     lstr("#F59E0B")},
            {Field::SignalAlarm,    lstr("#DC2626")}
        },

        // =========================
        // Dark Clinical (深色医疗)
        // =========================
        QVariantMap{
            {Field::Name,           lstr("深色医疗")},

            {Field::Background,     lstr("#0F172A")},
            {Field::Surface,        lstr("#1E293B")},
            {Field::Border,         lstr("#334155")},
            {Field::Grid,           lstr("#1E293B")},

            {Field::TextPrimary,    lstr("#F8FAFC")},
            {Field::TextSecondary,  lstr("#94A3B8")},
            {Field::TextHint,       lstr("#475569")},

            {Field::Primary,        lstr("#38BDF8")},
            {Field::Secondary,      lstr("#2DD4BF")},
            {Field::Success,        lstr("#34D399")},
            {Field::Warning,        lstr("#FBBF24")},
            {Field::Danger,         lstr("#F87171")},
            {Field::Accent,         lstr("#A855F7")},

            {Field::SignalEEG,      lstr("#22D3EE")},
            {Field::SignalECG,      lstr("#60A5FA")},
            {Field::SignalResp,     lstr("#4ADE80")},
            {Field::SignalSpO2, lstr("#C084FC")},
            {Field::SignalBody,     lstr("#FB923C")},
            {Field::SignalAlarm,    lstr("#F87171")}
        },

        // =========================
        // Midnight Neon (暗夜霓虹 - )
        // =========================
        QVariantMap{
            {Field::Name,           lstr("暗夜霓虹")},

            {Field::Background,     lstr("#050505")},
            {Field::Surface,        lstr("#261818")},
            {Field::Border,         lstr("#262626")},
            {Field::Grid,           lstr("#1C1C1C")},

            {Field::TextPrimary,    lstr("#FFFFFF")},
            {Field::TextSecondary,  lstr("#A3A3A3")},
            {Field::TextHint,       lstr("#525252")},

            {Field::Primary,        lstr("#A855F7")},
            {Field::Secondary,      lstr("#06B6D4")},
            {Field::Success,        lstr("#22C55E")},
            {Field::Warning,        lstr("#EAB308")},
            {Field::Danger,         lstr("#EF4444")},
            {Field::Accent,         lstr("#EC4899")},

            {Field::SignalEEG,      lstr("#00FFFF")},
            {Field::SignalECG,      lstr("#39FF14")},
            {Field::SignalResp,     lstr("#FFFF00")},
            {Field::SignalSpO2,     lstr("#FF00FF")},
            {Field::SignalBody,     lstr("#FF9900")},
            {Field::SignalAlarm,    lstr("#FF3333")}
        },

        // =========================
        // Warm Sepia (温暖护眼)
        // =========================
        QVariantMap{
            {Field::Name,           lstr("温暖护眼")},

            {Field::Background,     lstr("#FDFBF7")},
            {Field::Surface,        lstr("#F4F1EA")},
            {Field::Border,         lstr("#E6DFD3")},
            {Field::Grid,           lstr("#EDE6D9")},

            {Field::TextPrimary,    lstr("#433422")},
            {Field::TextSecondary,  lstr("#7C6E5C")},
            {Field::TextHint,       lstr("#A39684")},

            {Field::Primary,        lstr("#C2410C")},
            {Field::Secondary,      lstr("#0F766E")},
            {Field::Success,        lstr("#16A34A")},
            {Field::Warning,        lstr("#D97706")},
            {Field::Danger,         lstr("#DC2626")},
            {Field::Accent,         lstr("#7C3AED")},

            {Field::SignalEEG,      lstr("#0284C7")},
            {Field::SignalECG,      lstr("#059669")},
            {Field::SignalResp,     lstr("#B45309")},
            {Field::SignalSpO2,     lstr("#6D28D9")},
            {Field::SignalBody,     lstr("#EA580C")},
            {Field::SignalAlarm,    lstr("#BE123C")}
        }
    }},

    // =========================
    // Fonts
    // =========================
    {Field::Font, QVariantMap{
        {Field::Family,          lstr("Microsoft YaHei")},
        {Field::FamilyMono,      lstr("Consolas")},
    
        // Size
        {Field::SizeTiny,        10},
        {Field::SizeSmall,       12},
        {Field::SizeNormal,      14},
        {Field::SizeLarge,       16},
        {Field::SizeTitle,       20},
    
        // Weight
        {Field::WeightLight,     300},
        {Field::WeightNormal,    400},
        {Field::WeightMedium,    500},
        {Field::WeightBold,      700},
    
        // UI Mapping
        {Field::CaptionSize,     10}, 
        {Field::CaptionWeight,   400},
        {Field::BodySize,        14},
        {Field::BodyWeight,      400},
        {Field::SubtitleSize,    16},
        {Field::SubtitleWeight,  500},
        {Field::TitleSize,       20},
        {Field::TitleWeight,     700},
        {Field::MonospaceSize,   13}, 
        {Field::MonospaceWeight, 400}
    }},

    {Field::App, QVariantMap{
        {Field::AppIcon, QVariant(lstr(""))}
    }},

    {Field::lastTheme, QVariant(0)}
};

//QString UiSession::moduleName = "Ui";
//QString UiSession::name = "UiSession";

static auto configuration() {
    bool setToDefault = false;
    QVariantMap map;
    if (assign(map, Configuration::instance().get(UiField::ROOT))) {
        if (QVariantList list; copy(list, map, Field::Themes)) {
            for (auto& ele : list) {
                if (!ele.canConvert<QVariantMap>()) {
                    setToDefault = true;
                    break;
                }
            }
        } else 
            setToDefault = true;
        if (!setToDefault && !copy<int>(map, Field::lastTheme)) {
            setToDefault = true;
        }
        if (QVariantMap fontmap; !setToDefault && !copy(fontmap, map, Field::Font)) {
            // TODO: maybe more check.
            setToDefault = true;
        }
            
    } else 
        setToDefault = true;

    if (setToDefault) {
        qWarning("UiSession's configuration is not vaild, set to default.");
        Configuration::instance().set(UiField::ROOT, DEFAULT_JSON);
        map = DEFAULT_JSON;
    }
    return map;
}

static int Index_PageDevice = -1;
static int Index_PageUser = -1;
static int Index_PageSetting = -1;
static int Index_PageAiChat = -1;
Impl::Impl() 
    : UiSession(QML_MODULE_NAME, QML_MODULE_MAJOR, QML_MODULE_MINOR) {
    Q_ASSERT(!instance_);
    // InitRes(RESOURCE_NAME);

    auto cfg = configuration();
    auto themes = cfg.value(Field::Themes).value<QVariantList>();
    for (auto t : themes) 
        auto& map = this->themes_.emplaceBack(t.toMap());

    if (themes.isEmpty()) 
        FASTFAIL(lstr("配置文件已经损坏"), lstr(QML_MODULE_NAME));
    
    int index = 0;
    copy(index, cfg, Field::lastTheme);
    this->theme_ = this->themes_.value(index);

    this->font_ = cfg.value(Field::Font).value<QVariantMap>();
    this->app_ = cfg.value(Field::App).value<QVariantMap>();

    QFont f(this->font_.value(Field::Family).toString());
    QGuiApplication::setFont(f);

    connect(this, &Impl::updated, this, &Impl::appChanged);
    connect(this, &Impl::updated, this, &Impl::fontChanged);
    connect(this, &Impl::updated, this, &Impl::themeChanged);

    instance_ = this;

    Index_PageUser = this->pages_.size();
    this->pages_.emplaceBack(new PageUser(nullptr));

    Index_PageDevice = this->pages_.size();
    this->pages_.emplaceBack(new PageDevice(nullptr));
    
    // Index_PageAiChat = this->pages_.size();
    // this->pages_.emplaceBack(new PageChat);

    Index_PageSetting = this->pages_.size();
    auto page = new PageSetting(nullptr);
    this->pages_.emplaceBack(page);
    page->appendWidget(new ThemeFontCard(page));

    this->mainWin_ = new MainWindow(this);
    
}

Impl::~Impl() {
    auto cfg = configuration();
    QVariantList list;
    for (auto c : qMove(this->themes_))
        list.append(c);
    cfg[Field::Themes] = list;
    cfg[Field::Font] = this->font_;

    Configuration::instance().set(UiField::ROOT, cfg);
    for (auto c : this->pages_)
        c->deleteLater();
    // UninitRes(RESOURCE_NAME);

    delete mainWin_;
}

bool Impl::load(QString&) {
    mainWin_->show();
    return true;
}

bool Impl::unload(QString&) {
    mainWin_->close();
    return true;
}

Extensible* Impl::main() {
    return this->main_;
}

void Impl::main(Extensible* main) {
    for (auto c : this->pages_) {
        main->appendWidget(c);
    }
    auto c = static_cast<Container*>(this->main_ = main);
    auto h = static_cast<PageUser*>(this->home());
    auto d = this->pages_.at(Index_PageDevice);
    connect(h, &PageUser::requestConnection, c, [c, d]() {
        auto idx = c->widgets().indexOf(d);
        c->setCurrentIndex(idx);
    });
}

// qsizetype Impl::append(QWidget* widget) {
//     return main_->appendWidget(widget);
// }
// 
// QWidget* Impl::remove(qsizetype index) {
//     return main_->removeWidget(index);
// }

Extensible* Impl::home() {
    return static_cast<Extensible*>(this->pages_.at(Index_PageUser));
}

Extensible* Impl::settings() {
    return static_cast<Extensible*>(this->pages_.at(Index_PageSetting));
}

Extensible* Impl::searchDevicePage()
{
    return static_cast<PageDevice*>(pages_.value(Index_PageDevice))->search();
}

Extensible* Impl::connectedDevice()
{
    return static_cast<PageDevice*>(pages_.value(Index_PageDevice))->connected();
}

Extensible* Impl::deviceRecords()
{
    return static_cast<PageDevice*>(pages_.value(Index_PageDevice))->record();
}

void Impl::push(QWidget* widget) {
    mainWin_->push(widget);
}

QWidget* Impl::pop(){
    return mainWin_->pop();
}

QString UiSession::moduleName() { return lstr(QML_MODULE_NAME); }

void Impl::setTheme(QVariantMap which) {
    theme_ = which;
    emit this->themeChanged();
    auto index{ 0u };
    for (auto t : this->themes_)
        if (t.value(Field::Name) == which.value(Field::Name))
            break;
        else
            index++;
    Configuration::instance().set(Field::LAST_THEME, index);
}

void Impl::setFont(QVariantMap value) {
    font_ = value;
    emit this->fontChanged();
    Configuration::instance().set(Field::FONT, this->font_);
}





// ==============================
// 设置卡片
// ===================


static const int FontSizeMapping[] = { 12, 14, 16 };

ThemeFontCard::ThemeFontCard(QWidget* parent) : QWidget(parent) {
    // 设置卡片在 Grid 中的跨度属性（两行一列）
    this->setProperty("rowSpan", 2);
    this->setProperty("colSpan", 1);

    // 主布局：垂直排列
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(20); // 增大间距，让两组设置视觉上区分更明显

    // ================= 行 1：主题设置 =================
    auto* themeTitleLabel = new QLabel(lstr("系统主题设置"), this);
    auto titleFont = themeTitleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    themeTitleLabel->setFont(titleFont);
    mainLayout->addWidget(themeTitleLabel);

    themeList_ = new QListWidget(this);
    mainLayout->addWidget(themeList_, 1); // 分配拉伸权重

    // ================= 行 2：字体设置 =================
    auto* fontTitleLabel = new QLabel(lstr("系统字体设置"), this);
    fontTitleLabel->setFont(titleFont);
    mainLayout->addWidget(fontTitleLabel);

    // 字号
    auto* sizeLayout = new QHBoxLayout();
    auto* sizeLabel = new QLabel(lstr("标准大小:"), this);

    sizeSlider_ = new QSlider(Qt::Horizontal, this);
    sizeSlider_->setRange(0, 2);
    sizeSlider_->setTickPosition(QSlider::TicksBelow);
    sizeSlider_->setTickInterval(1);

    sizeTextLabel_ = new QLabel(this);
    sizeTextLabel_->setAlignment(Qt::AlignCenter);
    sizeTextLabel_->setMinimumWidth(32);

    sizeLayout->addWidget(sizeLabel);
    sizeLayout->addWidget(sizeSlider_);
    sizeLayout->addWidget(sizeTextLabel_);

    mainLayout->addLayout(sizeLayout);

    mainLayout->addStretch();

    // ================= 信号与槽连接 =================
    // 内部组件事件
    connect(themeList_, &QListWidget::itemClicked, this, &ThemeFontCard::onThemeSelected);
    connect(sizeSlider_, &QSlider::valueChanged, this, &ThemeFontCard::applyFontChange);

    // 监听全局 Session 事件
    auto* session = UiSession::instance();
    if (session) {
        connect(session, &UiSession::themeChanged, this, &ThemeFontCard::syncWithSession);
        connect(session, &UiSession::fontChanged, this, &ThemeFontCard::syncWithSession);
    }

    // 初始化数据同步
    syncWithSession();
}

ThemeFontCard::~ThemeFontCard() {}

void ThemeFontCard::syncWithSession() {
    auto* session = UiSession::instance();
    if (!session) return;

    // 统一阻止信号，避免同步引发死循环
    QSignalBlocker blockerList(themeList_);
    QSignalBlocker blockerSlider(sizeSlider_);

    // 1. 获取最新数据
    auto currentTheme = session->theme();
    QVariantMap fontMap = session->font();

    QString currentName = currentTheme.value(Field::Name).toString();
    QString border = currentTheme.value(Field::Border).toString();
    QString surface = currentTheme.value(Field::Surface).toString();
    QString grid = currentTheme.value(Field::Grid).toString();
    QString primary = currentTheme.value(Field::Primary).toString();
    QString textPrimary = currentTheme.value(Field::TextPrimary).toString();

    // 获取字体配置
    QString fontFamily = fontMap.value(Field::Family, lstr("Microsoft YaHei")).toString();
    int sizeNormal = fontMap.value(Field::SizeNormal, 14).toInt();
    int sizeSmall = fontMap.value(Field::SizeSmall, 12).toInt();
    int sizeTitle = fontMap.value(Field::SizeTitle, 20).toInt();

    // 2. 映射字号到 Slider
    int sliderValue = 1;
    if (sizeNormal <= FontSizeMapping[0]) sliderValue = 0;
    else if (sizeNormal >= FontSizeMapping[2]) sliderValue = 2;
    else sliderValue = 1;

    // 3. 更新字体 UI 状态
    sizeSlider_->setValue(sliderValue);

    if (sliderValue == 0) sizeTextLabel_->setText(lstr("小"));
    else if (sliderValue == 1) sizeTextLabel_->setText(lstr("中"));
    else sizeTextLabel_->setText(lstr("大"));

    this->setStyleSheet(lstr("font-family: '%1'; color: %2; font-size: %3px;")
        .arg(fontFamily, textPrimary).arg(sizeNormal));

    themeList_->setStyleSheet(lstr(
        "QListWidget { border: none; background: transparent; padding: 0px; outline: none; }"
        "QListWidget::item { background-color: %2; border: 1px solid %1; border-radius: 8px; padding: 10px 12px; margin-bottom: 6px; color: %5; font-size: %6px; }"
        "QListWidget::item:hover { background-color: %3; border-color: %4; }"
        "QListWidget::item:selected { background-color: %4; color: #FFFFFF; border-color: %4; }"
    ).arg(border, surface, grid, primary, textPrimary).arg(sizeNormal));

    sizeTextLabel_->setStyleSheet(lstr("color: %1; font-weight: bold; font-size: %2px;").arg(textPrimary).arg(sizeNormal));

    sizeSlider_->setStyleSheet(lstr(
        "QSlider::handle:horizontal { background: %1; width: 12px; height: 12px; border-radius: 6px; }"
    ).arg(primary));

    // 5. 重新载入主题列表项
    themeList_->clear();
    auto themes = session->themes();
    for (const auto& themeMap : themes) {
        QString name = themeMap.value(Field::Name).toString();
        QString primaryColorStr = themeMap.value(Field::Primary).toString();
        QString bgColorStr = themeMap.value(Field::Background).toString();

        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setBrush(QColor(bgColorStr));
        painter.setPen(QPen(QColor(border), 1));
        painter.drawEllipse(1, 1, 28, 28);

        painter.setBrush(QColor(primaryColorStr));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(8, 8, 14, 14);
        painter.end();

        auto* item = new QListWidgetItem(QIcon(pixmap), name, themeList_);
        item->setData(Qt::UserRole, themeMap);

        if (name == currentName) {
            themeList_->setCurrentItem(item);
        }
    }
}
void ThemeFontCard::onThemeSelected(QListWidgetItem* item) {
    auto* session = UiSession::instance();
    if (!session || !item) return;

    auto themeMap = item->data(Qt::UserRole).toMap();
    session->setTheme(themeMap);
    emit session->themeChanged();
}

void ThemeFontCard::applyFontChange() {
    auto* session = UiSession::instance();
    if (!session) return;

    QVariantMap fontMap = session->font();

    int sliderValue = sizeSlider_->value();
    int newSize = FontSizeMapping[sliderValue];

    fontMap[Field::SizeNormal] = newSize;

    // 按比例计算其他关联字号
    fontMap[Field::SizeTiny] = std::max(10, newSize - 4);
    fontMap[Field::SizeSmall] = std::max(12, newSize - 2);
    fontMap[Field::SizeLarge] = newSize + 2;
    fontMap[Field::SizeTitle] = newSize + 6;

    fontMap[Field::BodySize] = newSize;
    fontMap[Field::CaptionSize] = std::max(10, newSize - 4);
    fontMap[Field::SubtitleSize] = newSize + 2;
    fontMap[Field::TitleSize] = newSize + 6;

    session->setFont(fontMap);
}

// Module entry.

MODULE_ENTRY(Q_DECL_EXPORT)() {
    return new Impl();
} 
void Impl::release() noexcept {
    delete this;
}


