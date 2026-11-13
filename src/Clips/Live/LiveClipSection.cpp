// ============================================================================
//  LiveClipSection 实现 —— 实时源片段专属编辑区（接入方式/源选择/延迟）
// ============================================================================

#include "LiveClipSection.hpp"

#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "live/LiveIngest.hpp" // Ingest 注册表（接入方式/源枚举唯一数据源）

namespace ms {

namespace {

/// 自家再发布标记（LiveHub::ensureAsset 的发布名前缀）——源下拉过滤，
/// 防把 Hub 转发流当采集源（自环：自己发自己收）
constexpr const char* kRepublishMark = "ms-live";

/// 源名是否自家再发布流。归一化比对：NDI 发现列表会把源名中的 '/'
/// 等非法字符替换为空格（"ms-live/qa/live-qa" → "ms-live qa live-qa"），
/// 且列表项带 "MACHINE (源名)" 机器前缀——固定 startsWith 两种形态都
/// 漏过滤（2026-11-08 实测），改按标记串包含匹配（大小写不敏感）
bool isRepublishName(const QString& name) {
    QString norm = name;
    norm.replace(QLatin1Char('/'), QLatin1Char(' '))
        .replace(QLatin1Char('\\'), QLatin1Char(' '));
    return norm.contains(QLatin1String(kRepublishMark), Qt::CaseInsensitive);
}

/// 下拉中按 data（ref 串）找项索引——label 是显示名（窗口捕获带标题），
/// ref 才是存储值（asset.live_source_name），两者分离（注册表契约）
int findIndexByData(const QComboBox* box, const QString& ref) {
    for (int i = 0; i < box->count(); ++i)
        if (box->itemData(i).toString() == ref) return i;
    return -1;
}

} // namespace

// ---------------------------------------------------------------------------
// 构造：控件树 + 预填 + 信号连接（预填在 connect 之前，不触发写回）。
// 2026-11-13 片段插件化：初值素材按 ctx 现查（工厂已校验 kind==Live）。
// ---------------------------------------------------------------------------
LiveClipSection::LiveClipSection(const ClipSectionContext& ctx, QWidget* parent)
    : QWidget(parent), ctx_(ctx) {
    const Asset* src = boundAsset();
    static const Asset kEmpty{};
    const Asset& asset = src ? *src : kEmpty;

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    auto* box = new QGroupBox(QStringLiteral("实况源"), this);
    auto* lay = new QVBoxLayout(box);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(4);

    // ---- 接入方式（Ingest 注册表驱动；未实现协议置灰展示）-------------------
    ingest_ = new QComboBox(box);
    ingest_->setObjectName("liveIngest"); // 测试定位锚
    auto* ing_model = new QStandardItemModel(ingest_); // 置灰项（setEnabled false）
    ingest_->setModel(ing_model);
    {
        const auto& reg = ingestRegistry();
        int preselect = -1;
        for (std::size_t i = 0; i < reg.size(); ++i) {
            auto* item = new QStandardItem(QString::fromUtf8(reg[i].name));
            item->setEnabled(reg[i].implemented);
            ing_model->appendRow(item);
            if (static_cast<std::int32_t>(reg[i].kind) == asset.live_ingest)
                preselect = static_cast<int>(i); // 注册表顺序恒定 → 下标即映射
        }
        if (preselect >= 0) ingest_->setCurrentIndex(preselect);
    }
    lay->addWidget(ingest_);

    // ---- 源标识（可编辑下拉：发现列表 + 手输跨网段源）------------------------
    // itemData 存 ref（存储串）、text 存 label（显示名）——NDI/Spout 两者
    // 相同；窗口/显示器捕获 label 是友好标题、ref 带定位后缀
    auto* src_row = new QHBoxLayout();
    source_ = new QComboBox(box);
    source_->setObjectName("liveSource"); // 测试定位锚
    source_->setEditable(true);           // 列表没有的源可直接键入
    source_->setInsertPolicy(QComboBox::NoInsert); // 键入值不污染下拉列表
    source_->setMinimumContentsLength(24);
    // 预填当前源标识（始终保留——发现列表可能尚未上线该源）
    if (!asset.live_source_name.empty()) {
        const QString ref = QString::fromStdString(asset.live_source_name);
        source_->addItem(ref, ref); // 无列表对照：label=ref 自映射
        source_->setCurrentIndex(0);
    }
    src_row->addWidget(source_, /*stretch=*/1);
    refresh_ = new QPushButton(QStringLiteral("刷新"), box);
    refresh_->setObjectName("liveRefresh");
    refresh_->setToolTip(QStringLiteral("重新枚举当前接入方式的源列表（过滤本机再发布流）"));
    src_row->addWidget(refresh_);
    lay->addLayout(src_row);

    // ---- 延迟模式 -----------------------------------------------------------
    latency_ = new QComboBox(box);
    latency_->setObjectName("liveLatency");
    latency_->addItem(QStringLiteral("最低延迟（帧到即显）")); // index 0 = 默认
    latency_->addItem(QStringLiteral("同步（缓冲 2~3 帧抗抖动）")); // 1
    latency_->setCurrentIndex(asset.live_latency == 1 ? 1 : 0);
    lay->addWidget(latency_);

    root->addWidget(box);

    // ---- 即时生效：任一控件变更 → applier 写回 -------------------------------
    connect(ingest_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                onEdited();
                onRefreshSources(); // 接入方式变化 → 源列表语义变化，自动重枚举
            });
    connect(source_, &QComboBox::editTextChanged,
            this, [this](const QString&) { onEdited(); });
    connect(source_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onEdited(); });
    connect(latency_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onEdited(); });
    connect(refresh_, &QPushButton::clicked,
            this, &LiveClipSection::onRefreshSources);
}

// ---------------------------------------------------------------------------
// currentDesc：接入方式下拉当前选中的注册表条目
// ---------------------------------------------------------------------------
const IngestDesc* LiveClipSection::currentDesc() const {
    const int idx = ingest_ ? ingest_->currentIndex() : -1;
    const auto& reg = ingestRegistry();
    if (idx < 0 || idx >= static_cast<int>(reg.size())) return nullptr;
    return &reg[static_cast<std::size_t>(idx)];
}

// ---------------------------------------------------------------------------
// applyTo：控件值 → asset.live_*（Live 域唯一写者，无条件覆盖）
// ---------------------------------------------------------------------------
void LiveClipSection::applyTo(Asset& a) const {
    if (a.kind != AssetKind::Live) return;
    const IngestDesc* desc = currentDesc();
    a.live_ingest = desc != nullptr ? static_cast<std::int32_t>(desc->kind)
                                    : 0; // 无选中（防御）回退 NDI
    // 列表选中项 → ref（存储串）；手输文本 → currentText（NDI 源名/
    // sender 名本身即 ref，语义一致）
    const QVariant data = source_->currentData();
    a.live_source_name = data.isValid()
        ? data.toString().toStdString()
        : source_->currentText().toStdString();
    a.live_latency = latency_->currentIndex() == 1 ? 1 : 0;
}

// ---------------------------------------------------------------------------
// refreshFrom：模型 → 面板灌显示（QSignalBlocker 防回环；源下拉编辑中
// （有焦点）跳过——"正在编辑的字段用户说了算"）
// ---------------------------------------------------------------------------
void LiveClipSection::refreshFrom(const Asset& a) {
    if (a.kind != AssetKind::Live) return;
    if (source_ && source_->hasFocus()) return; // 用户正在挑源/键入
    const QSignalBlocker b0(ingest_);
    const QSignalBlocker b1(source_);
    const QSignalBlocker b2(latency_);
    // 接入方式：按注册表下标回选
    const auto& reg = ingestRegistry();
    for (std::size_t i = 0; i < reg.size(); ++i) {
        if (static_cast<std::int32_t>(reg[i].kind) == a.live_ingest) {
            ingest_->setCurrentIndex(static_cast<int>(i));
            break;
        }
    }
    // 源标识：优先按 ref 匹配列表项；无匹配则整串作手输文本
    const QString ref = QString::fromStdString(a.live_source_name);
    if (source_->currentData().toString() != ref &&
        source_->currentText() != ref) {
        const int idx = findIndexByData(source_, ref);
        if (idx >= 0) {
            source_->setCurrentIndex(idx);
        } else {
            source_->clear();
            if (!ref.isEmpty()) source_->addItem(ref, ref);
        }
    }
    latency_->setCurrentIndex(a.live_latency == 1 ? 1 : 0);
}

// ---------------------------------------------------------------------------
// onRefreshSources：当前接入方式的源列表重灌（保留当前选择；过滤再发布流）
// ---------------------------------------------------------------------------
void LiveClipSection::onRefreshSources() {
    // 当前值快照（ref 语义——见 applyTo 同款取值）
    const QVariant cur_data = source_->currentData();
    const QString   cur_text = source_->currentText();
    const QString   cur_ref = cur_data.isValid() ? cur_data.toString() : cur_text;

    source_->clear();
    // 同步枚举（≤300ms 阻塞——点击型操作可接受；未实现协议/无环境=空表）
    if (const IngestDesc* desc = currentDesc();
        desc != nullptr && desc->implemented && desc->list_sources != nullptr) {
        for (const auto& s : desc->list_sources(300)) {
            const QString qlabel = QString::fromStdString(s.label);
            if (isRepublishName(qlabel)) continue; // 自家 Hub 转发流不当采集源
            source_->addItem(qlabel, QString::fromStdString(s.ref));
        }
    }
    // 恢复当前值（按 ref 匹配列表；无匹配则保留为手输文本）
    const int idx = findIndexByData(source_, cur_ref);
    if (idx >= 0) {
        source_->setCurrentIndex(idx);
    } else if (!cur_ref.isEmpty()) {
        source_->addItem(cur_ref, cur_ref);
        source_->setCurrentIndex(source_->count() - 1);
    }
}

// ---------------------------------------------------------------------------
// 绑定素材定位：按 ctx.clip->asset_id 现查工作副本素材表（换绑安全——
// 每次现查不缓存；未绑定/类型不符返回 nullptr，写回与灌值统一防御）
// ---------------------------------------------------------------------------
const Asset* LiveClipSection::boundAsset() const {
    if (!ctx_.clip || !ctx_.scene || ctx_.clip->asset_id.empty()) return nullptr;
    return ctx_.scene->findAsset(ctx_.clip->asset_id);
}

Asset* LiveClipSection::writableAsset() {
    if (!ctx_.clip || !ctx_.scene || ctx_.clip->asset_id.empty()) return nullptr;
    for (auto& asset : ctx_.scene->assets)
        if (asset.id == ctx_.clip->asset_id) return &asset;
    return nullptr;
}

// ---------------------------------------------------------------------------
// 【ClipSectionInterface】宿主广播灌值：const ms::Clip* 经 void* 穿越 →
// 按其当前绑定现查素材 → refreshFrom（焦点保护 + 防回环见 refreshFrom）
// ---------------------------------------------------------------------------
void LiveClipSection::refreshFromClip(const void* clip) {
    auto* c = static_cast<const ms::Clip*>(clip);
    if (!c || !ctx_.scene || c->asset_id.empty()) return;
    const Asset* a = ctx_.scene->findAsset(c->asset_id);
    if (!a || a->kind != AssetKind::Live) return;
    refreshFrom(*a);
}

// ---------------------------------------------------------------------------
// onEdited：任一控件变更 → 直写素材 live_* 全字段 + on_edited 落定
//（原适配器 applyClipLiveEdit 职责内化，2026-11-13 片段插件化）
// ---------------------------------------------------------------------------
void LiveClipSection::onEdited() {
    Asset* a = writableAsset();
    if (!a || a->kind != AssetKind::Live) return; // 已删/未绑定：静默忽略

    applyTo(*a); // 控件值 → asset.live_* 全字段

    if (ctx_.on_edited) ctx_.on_edited(); // 宿主落定：sceneEdited + 广播
}

} // namespace ms
