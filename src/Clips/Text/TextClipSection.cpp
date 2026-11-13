// ============================================================================
//  TextClipSection 实现 —— 文本片段专属编辑区（内容/字体/字号/颜色/样式）
//
//  交互约定（与 ClipPropertyPanel 同源）：
//    * 即时生效：任一控件变更 → onEdited() 直写绑定素材 asset.text_* +
//      测量回填 + ctx.on_edited 落定（宿主 sceneEdited → 自动下发）；
//    * 预填（构造）发生在信号连接之前——setValue 不触发写回；
//    * 灌值（refreshFromClip）QSignalBlocker 包裹 + 内容框焦点保护——
//      灌显示与用户编辑两条链路分离，不会回环（广播 → refreshFrom → 不写回）。
// ============================================================================

#include "TextClipSection.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include "media/TextRasterizer.hpp"

namespace ms {

namespace {

/// 0xRRGGBB → QColor（Asset 色值 ↔ Qt 色的唯一换算点）
QColor fromRgb24(std::uint32_t v) {
    return QColor(int((v >> 16) & 0xFF), int((v >> 8) & 0xFF),
                  int(v & 0xFF));
}

/// QColor → 0xRRGGBB（合法性由调用方以 isValid 保证）
std::uint32_t toRgb24(const QColor& c) {
    return (std::uint32_t(c.red()) << 16) | (std::uint32_t(c.green()) << 8) |
           std::uint32_t(c.blue());
}

/// 预览色块样式：非 0 实色 + 灰边；0 = 透明（虚线空框示意）
QString swatchStyle(std::uint32_t rgb, bool enabled) {
    if (!enabled || rgb == 0)
        return QStringLiteral("background:transparent;"
                              "border:1px dashed #999;");
    return QStringLiteral("background:#%1;border:1px solid #777;")
        .arg(rgb, 6, 16, QLatin1Char('0'));
}

/// 字体下拉项标签：映射表键（渲染端解析键，去空白英文）+ 常用中文字体
/// 中文别名——显示"微软雅黑（MicrosoftYaHei）"，data 存映射表键。
/// 表外键（理论上不可达，fontCatalog 与本表同源维护）按原键显示。
QString fontDisplayLabel(const std::string& key) {
    static const std::map<std::string, const char*> kAlias = {
        {"MicrosoftYaHei",    "微软雅黑"},
        {"MicrosoftYaHeiUI",  "微软雅黑 UI"},
        {"SimHei",            "黑体"},
        {"SimSun",            "宋体"},
        {"KaiTi",             "楷体"},
        {"FangSong",          "仿宋"},
        {"Arial",             "Arial"},
        {"SegoeUI",           "Segoe UI"},
        {"Consolas",          "Consolas"},
        {"TimesNewRoman",     "Times New Roman"},
    };
    const auto it = kAlias.find(key);
    if (it == kAlias.end()) return QString::fromStdString(key);
    return QStringLiteral("%1（%2）")
        .arg(QString::fromUtf8(it->second),
             QString::fromStdString(key));
}

} // namespace

// ---------------------------------------------------------------------------
// 构造：控件树 + 预填 + 信号连接（预填在 connect 之前，不触发写回）。
// 2026-11-13 片段插件化：初值素材按 ctx 现查（工厂已校验 kind==Text）。
// ---------------------------------------------------------------------------
TextClipSection::TextClipSection(const ClipSectionContext& ctx, QWidget* parent)
    : QWidget(parent), ctx_(ctx) {
    const Asset* src = boundAsset();
    static const Asset kEmpty{};
    const Asset& asset = src ? *src : kEmpty;

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    auto* box  = new QGroupBox(QStringLiteral("文本"), this);
    auto* lay  = new QVBoxLayout(box);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(4);

    // ---- 内容（多行编辑；显式 '\n' 换行，与渲染端排版语义一致）----
    content_ = new QPlainTextEdit(box);
    content_->setObjectName("textContent"); // 测试定位锚
    content_->setPlainText(QString::fromStdString(asset.text_content));
    content_->setMinimumHeight(56);
    content_->setToolTip("文本内容；Enter 显式换行（无自动折行——"
                         "拉伸归片段变换的宽/高）");
    lay->addWidget(content_);

    // ---- 样式表单（字体/字号/颜色/粗斜/对齐/背景）---------------------------
    auto* form = new QFormLayout();
    form->setSpacing(4);

    font_ = new QComboBox(box);
    font_->setObjectName("textFont");
    // 下拉数据源 = TextRasterizer::fontCatalog()（与渲染端同一映射表——
    // 杜绝"UI 列表与渲染解析表漂移"）；data = 映射表键（asset.text_font 值）
    for (const auto& name : TextRasterizer::fontCatalog())
        font_->addItem(fontDisplayLabel(name), QString::fromStdString(name));
    const int fidx = font_->findData(QString::fromStdString(asset.text_font));
    if (fidx >= 0)
        font_->setCurrentIndex(fidx); // 未知名字保留当前项（渲染端回退兜底）

    size_ = new IntDragValueWidget(box);
    size_->setObjectName("textFontSize");
    size_->setRange(8, 512); // 渲染端 clamp 4..4096；UI 限常用区间
    size_->setSuffix(QStringLiteral(" px"));
    size_->setValue(asset.text_font_size > 0 ? asset.text_font_size : 48);
    // 定宽（2026-11-13 用户截图：字体下拉 stretch=1 把字号框挤压至零宽——
    // 窄面板下「字号」标签贴右缘、控件不可见；定宽后空间不足时压缩 font_）
    size_->setFixedWidth(96);

    // 文字颜色：按钮（选色入口）+ 预览色块
    color_btn_    = new QPushButton(QStringLiteral("选色…"), box);
    color_btn_->setObjectName("textColor");
    color_swatch_ = new QLabel(box);
    color_swatch_->setFixedSize(28, 16);

    bold_   = new QCheckBox(QStringLiteral("粗体"), box);
    bold_->setObjectName("textBold");
    bold_->setChecked(asset.text_bold);
    italic_ = new QCheckBox(QStringLiteral("斜体"), box);
    italic_->setObjectName("textItalic");
    italic_->setChecked(asset.text_italic);

    align_ = new QComboBox(box);
    align_->setObjectName("textAlign");
    align_->addItem(QStringLiteral("左对齐"));   // currentIndex = 0（默认）
    align_->addItem(QStringLiteral("居中"));     // 1
    align_->addItem(QStringLiteral("右对齐"));   // 2
    align_->setCurrentIndex(std::clamp(asset.text_h_align, 0, 2));
    align_->setToolTip("多行文本的行间水平对齐（帧为自然尺寸，"
                       "行间对齐是唯一的水平自由度）");

    bg_enable_ = new QCheckBox(QStringLiteral("背景色块"), box);
    bg_enable_->setObjectName("textBgEnable");
    bg_enable_->setChecked(asset.text_bg_color != 0);
    bg_btn_    = new QPushButton(QStringLiteral("选色…"), box);
    bg_btn_->setObjectName("textBgColor");
    bg_swatch_ = new QLabel(box);
    bg_swatch_->setFixedSize(28, 16);

    // ---- 布局组装（表单行填充进 form，最后并入 box 纵向布局）----------------
    // 样式行用水平排布压缩纵向高度（面板空间宝贵）
    auto* font_row = new QHBoxLayout();
    font_row->addWidget(font_, /*stretch=*/1);
    font_row->addWidget(new QLabel(QStringLiteral("字号"), box));
    font_row->addWidget(size_);
    auto* color_row = new QHBoxLayout();
    color_row->addWidget(color_btn_);
    color_row->addWidget(color_swatch_);
    color_row->addStretch(1);
    auto* style_row = new QHBoxLayout();
    style_row->addWidget(bold_);
    style_row->addWidget(italic_);
    style_row->addStretch(1);
    auto* bg_row = new QHBoxLayout();
    bg_row->addWidget(bg_enable_);
    bg_row->addWidget(bg_btn_);
    bg_row->addWidget(bg_swatch_);
    bg_row->addStretch(1);
    form->addRow(QStringLiteral("字体"), font_row);
    form->addRow(QStringLiteral("文字颜色"), color_row);
    form->addRow(QStringLiteral("字型"), style_row);
    form->addRow(QStringLiteral("水平对齐"), align_);
    form->addRow(QStringLiteral("背景"), bg_row);
    lay->addLayout(form);

    root->addWidget(box);

    // ---- 颜色成员预填（原初始化列表迁构造体——初值素材现查后赋值）--------
    color_    = asset.text_color ? asset.text_color : 0xFFFFFF;
    bg_color_ = asset.text_bg_color;

    // ---- 即时生效：任一控件变更 → onEdited 直写素材（测量回填内化）--------
    // 预填已完成（connect 之前），此处连接后只有用户操作才触发写回。
    connect(content_, &QPlainTextEdit::textChanged,
            this, [this] { onEdited(); });
    connect(font_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onEdited(); });
    connect(size_, &IntDragValueWidget::valueChanged,
            this, [this](int) { onEdited(); });
    connect(bold_, &QCheckBox::toggled, this, [this](bool) { onEdited(); });
    connect(italic_, &QCheckBox::toggled, this, [this](bool) { onEdited(); });
    connect(align_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onEdited(); });
    connect(bg_enable_, &QCheckBox::toggled, this, [this](bool) {
        updateSwatches();
        onEdited();
    });
    connect(color_btn_, &QPushButton::clicked,
            this, &TextClipSection::onPickTextColor);
    connect(bg_btn_, &QPushButton::clicked,
            this, &TextClipSection::onPickBgColor);
    updateSwatches(); // 色块初值（connect 后调用不影响：无信号）
}

// ---------------------------------------------------------------------------
// 控件当前值 → 素材 text_* 全字段（无条件覆盖：文本域唯一写者是本区块，
// 无 ClipPropertyPanel 的过期快照交叉覆盖问题，无需脏位）
// ---------------------------------------------------------------------------
void TextClipSection::applyTo(Asset& a) const {
    a.text_content   = content_->toPlainText().toStdString();
    a.text_font      = font_->currentData().toString().toStdString();
    a.text_font_size = size_->value();
    a.text_color     = color_;
    a.text_bold      = bold_->isChecked();
    a.text_italic    = italic_->isChecked();
    a.text_h_align   = align_->currentIndex();
    // 未启用背景时写 0（透明）——选色记忆保留在本类成员，勾选恢复不丢
    a.text_bg_color  = bg_enable_->isChecked() ? bg_color_ : 0;
}

// ---------------------------------------------------------------------------
// 【双向同步·模型→面板向】素材当前值 → 控件（适配器广播；焦点保护 +
// QSignalBlocker 防回环，与 ClipPropertyPanel::refreshFrom 同约定）
// ---------------------------------------------------------------------------
void TextClipSection::refreshFrom(const Asset& a) {
    // QSignalBlocker 包裹全部灌值——不产生 valueChanged/toggled，不标脏
    const QSignalBlocker bc(content_), bf(font_), bs(size_), bb(bold_),
        bi(italic_), ba(align_), be(bg_enable_);
    // 内容框焦点保护：用户正在打字不灌（外部场景重载不覆盖输入中内容）
    if (!content_->hasFocus())
        content_->setPlainText(QString::fromStdString(a.text_content));
    const int fidx = font_->findData(QString::fromStdString(a.text_font));
    if (fidx >= 0)
        font_->setCurrentIndex(fidx); // 工作副本未知名字：保持当前显示
    size_->setValue(a.text_font_size > 0 ? a.text_font_size : 48);
    bold_->setChecked(a.text_bold);
    italic_->setChecked(a.text_italic);
    align_->setCurrentIndex(std::clamp(a.text_h_align, 0, 2));
    bg_enable_->setChecked(a.text_bg_color != 0);
    color_    = a.text_color ? a.text_color : 0xFFFFFF;
    bg_color_ = a.text_bg_color;
    updateSwatches(); // 色块无信号，直接刷
}

// ---------------------------------------------------------------------------
// 「选色…」（文字颜色）：QColorDialog → 确认才更新成员 + 色块 + 写回
// （拖选过程不产生中间信号——避免每次 mouse move 都触发场景下发）
// ---------------------------------------------------------------------------
void TextClipSection::onPickTextColor() {
    const QColor c = QColorDialog::getColor(
        fromRgb24(color_), this, QStringLiteral("文字颜色"));
    if (!c.isValid()) return; // 取消/关闭：不动当前值
    color_ = toRgb24(c);
    updateSwatches();
    onEdited();
}

void TextClipSection::onPickBgColor() {
    const QColor init = bg_color_ ? fromRgb24(bg_color_) : QColor(Qt::white);
    const QColor c = QColorDialog::getColor(init, this,
                                            QStringLiteral("背景颜色"));
    if (!c.isValid()) return;
    bg_color_ = toRgb24(c);
    // 选了背景色即视为启用。勾选若实际改变状态会触发 toggled → 写回+
    // sceneEdited（一次）；此时末尾不再重复 onEdited（防同一次选色双
    // 写回双下发）。未发生状态变化（本来就勾着）则走末尾常规写回。
    bool toggled_write = false;
    if (!bg_enable_->isChecked()) {
        bg_enable_->setChecked(true);
        toggled_write = true;
    }
    updateSwatches();
    if (!toggled_write) onEdited();
}

// ---------------------------------------------------------------------------
// 预览色块刷新：颜色成员 → QLabel 样式（背景未启用 = 虚线透明框）
// ---------------------------------------------------------------------------
void TextClipSection::updateSwatches() {
    color_swatch_->setStyleSheet(swatchStyle(color_, true));
    bg_swatch_->setStyleSheet(swatchStyle(bg_color_, bg_enable_->isChecked()));
}

// ---------------------------------------------------------------------------
// 绑定素材定位：按 ctx.clip->asset_id 现查工作副本素材表（换绑安全——
// 每次现查不缓存；未绑定/类型不符返回 nullptr，写回与灌值统一防御）
// ---------------------------------------------------------------------------
const Asset* TextClipSection::boundAsset() const {
    if (!ctx_.clip || !ctx_.scene || ctx_.clip->asset_id.empty()) return nullptr;
    return ctx_.scene->findAsset(ctx_.clip->asset_id);
}

Asset* TextClipSection::writableAsset() {
    if (!ctx_.clip || !ctx_.scene || ctx_.clip->asset_id.empty()) return nullptr;
    for (auto& asset : ctx_.scene->assets)
        if (asset.id == ctx_.clip->asset_id) return &asset;
    return nullptr;
}

// ---------------------------------------------------------------------------
// 【ClipSectionInterface】宿主广播灌值：const ms::Clip* 经 void* 穿越 →
// 按其当前绑定现查素材 → refreshFrom（焦点保护 + 防回环见 refreshFrom）
// ---------------------------------------------------------------------------
void TextClipSection::refreshFromClip(const void* clip) {
    auto* c = static_cast<const ms::Clip*>(clip);
    if (!c || !ctx_.scene || c->asset_id.empty()) return;
    const Asset* a = ctx_.scene->findAsset(c->asset_id);
    if (!a || a->kind != AssetKind::Text) return;
    refreshFrom(*a);
}

// ---------------------------------------------------------------------------
// 任一控件变更 → 直写素材 text_* 全字段 + 测量回填自然尺寸 + on_edited
// 落定（原适配器 applyClipTextEdit 职责内化，2026-11-13 片段插件化）
// ---------------------------------------------------------------------------
void TextClipSection::onEdited() {
    Asset* a = writableAsset();
    if (!a || a->kind != AssetKind::Text) return; // 已删/未绑定：静默忽略

    applyTo(*a); // 控件值 → asset.text_* 全字段
    // 测量回填自然尺寸（与 makeAssetFromFile/TextMediaSource::prepare
    // 同一渲染入口测量——三处同源，无漂移）：auto-size 语义下显示尺寸
    // 即时跟随字号/字体变化
    const TextLayout lay =
        TextRasterizer::measure(TextRasterizer::styleFromAsset(*a));
    a->width  = lay.width;
    a->height = lay.height;

    if (ctx_.on_edited) ctx_.on_edited(); // 宿主落定：sceneEdited + 广播
}

} // namespace ms
