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

#include <limits>

#include "live/LiveHub.hpp" // LiveHub（Ingest 注册表宿主）
#include "live/LiveIngest.hpp" // Ingest 注册表（接入方式/源枚举唯一数据源）
#include "live/MpegTsSender.hpp" // multicastUrlFor（组播 URL 确定性生成；
// LiveHub.hpp 刻意前向声明不 include 本头——2026-11-23 全量重编暴露
// C3861：此前 Live 插件未重编掩盖三天）

namespace ms {

namespace {

/// 自家再发布标记——源下拉过滤，防把转发流当采集源（自环：自己发自己收）。
/// NDI 分发退役（2026-11-21）后本机不再发布 ms-live 源；过滤保留为
/// 旧版本 Flow 实例仍在同一网络发布时的防御。
constexpr const char* kRepublishMark = "ms-live";

/// 帧同步目标延迟档位（μs）——display 端按"帧接收戳 + 本延迟"排程，
/// 多 display 跨机帧同步（LiveMediaSource 排程引擎，2026-11-21）。
/// 【2026-11-23 用户定案「必须同步」】监视档（0=最新帧直显）退役——
/// Live 的用途就是跨机分发+帧同步，只留同步档位；上限 500ms 在引擎
/// 缓冲容量（clamp [16,48] 帧 @30fps ≈ 1.6s）的安全域内。
struct SyncLatencyPreset {
    const char* label;
    std::int64_t us;
};
constexpr SyncLatencyPreset kSyncLatencyPresets[] = {
    {"40 ms",  40'000},
    {"80 ms",  80'000},
    {"120 ms", 120'000},
    {"200 ms", 200'000},
    {"500 ms", 500'000},
};
/// 存量工程兜底：帧同步值非法（≤0 = 旧"监视"档）一律归一到 120ms
constexpr std::int64_t kSyncLatencyDefaultUs = 120'000;

/// 下拉中按值就近选档（模型值非档位精确值时取差最小者——
/// 外部构造的 pb / 旧工程值也能回灌到合理档位）
int nearestSyncIndex(const QComboBox* box, std::int64_t us) {
    int best = 0;
    std::int64_t best_diff = std::numeric_limits<std::int64_t>::max();
    for (int i = 0; i < box->count(); ++i) {
        const std::int64_t v = box->itemData(i).toLongLong();
        const std::int64_t d = v > us ? v - us : us - v;
        if (d < best_diff) { best_diff = d; best = i; }
    }
    return best;
}

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

    // ---- 启用闸（两态：启用=采集+下发运行并锁定，停止=停采解锁）------------
    // asset.live_enabled 全链路生效：LiveHub::ensureAsset（采集+组播
    // 转发拆装）、LiveMediaSource::prepare（收流装配）、MediaScheduler
    // 指纹（启停即失效重建）。启用中锁定源编辑控件——运行中切源会析构
    // 重建连接中的源实例（接收/渲染线程竞态偶发崩溃）。
    enable_ = new QPushButton(QStringLiteral("启用"), box);
    enable_->setObjectName("liveEnable"); // 测试定位锚
    enable_->setCheckable(true);
    enable_->setToolTip(QStringLiteral(
        "启用：开始采集并组播下发（display 端有画面），同时锁定源选择。\n"
        "停止：结束采集与下发（display 占位），解锁后可切换源。"));
    lay->addWidget(enable_);

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

    // ---- 帧同步目标延迟（跨机帧同步排程；display 端消费）---------------------
    // 【2026-11-23 必须同步】仅同步档位（监视档退役）；延迟模式下拉同日
    // 退役——恒同步语义，不再暴露选项。
    auto* sync_row = new QHBoxLayout();
    auto* sync_label = new QLabel(QStringLiteral("帧同步"), box);
    sync_row->addWidget(sync_label);
    sync_latency_ = new QComboBox(box);
    sync_latency_->setObjectName("liveSyncLatency");
    sync_latency_->setToolTip(QStringLiteral(
        "按帧接收时间戳 + 本延迟排程显示：多台 display 选同一档即可跨机帧"
        "同步。建议 80~200ms。"));
    for (const auto& p : kSyncLatencyPresets)
        sync_latency_->addItem(QString::fromUtf8(p.label),
                               QVariant(static_cast<qlonglong>(p.us)));
    sync_latency_->setCurrentIndex(nearestSyncIndex(
        sync_latency_,
        asset.live_target_latency_us > 0 ? asset.live_target_latency_us
                                         : kSyncLatencyDefaultUs));
    sync_row->addWidget(sync_latency_, /*stretch=*/1);
    lay->addLayout(sync_row);

    // ---- 组播转发（恒启用——跨机画面分发唯一通道）---------------------------
    // 【2026-11-23 用户定案】组播不再可选：不勾选 = display 端在本机按名
    // 直收（跨机必黑屏），该分支是坑，直接消灭——live_url 恒生成（见
    // applyTo），此处仅展示确定性分配的组播地址（信息性，不可改）。
    mcast_url_ = new QLabel(box);
    mcast_url_->setObjectName("liveMcastUrl");
    mcast_url_->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    mcast_url_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lay->addWidget(mcast_url_);

    root->addWidget(box);

    // 启用状态按素材现值恢复（面板重建/重新选中后锁定不丢）——
    // 置于 connect 之前：setChecked 不触发写回
    syncEnableUi();

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
    connect(sync_latency_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) { onEdited(); });
    connect(refresh_, &QPushButton::clicked,
            this, &LiveClipSection::onRefreshSources);
    connect(enable_, &QPushButton::toggled,
            this, &LiveClipSection::onEnableToggled);
}

// ---------------------------------------------------------------------------
// currentEnabled：当前绑定素材是否启用中（读 asset.live_enabled——状态
// 唯一事实源，跨面板重建/重启/下发全程同源）
// ---------------------------------------------------------------------------
bool LiveClipSection::currentEnabled() const {
    const Asset* a = boundAsset();
    return a != nullptr && a->kind == AssetKind::Live && a->live_enabled;
}

// ---------------------------------------------------------------------------
// syncEnableUi：启用状态 → 按钮两态文案/样式 + 编辑控件可用性（幂等）。
// 落点：构造恢复 / refreshFromClip 广播 / onEnableToggled 切换。
// QSignalBlocker 防 setChecked 回环触发 onEnableToggled。
// ---------------------------------------------------------------------------
void LiveClipSection::syncEnableUi() {
    const bool on = currentEnabled();
    {
        const QSignalBlocker b(enable_);
        enable_->setChecked(on);
    }
    // 两态按钮：未启用「启用」（默认态）；启用后「停止」（绿底——
    // 运行中，按下面板即停止采集下发并解锁）
    enable_->setText(on ? QStringLiteral("停止") : QStringLiteral("启用"));
    enable_->setStyleSheet(on
        ? QStringLiteral("QPushButton { background-color: #2E7D32;"
                         " color: white; font-weight: bold; }")
        : QString());
    const bool editable = !on;
    ingest_->setEnabled(editable);
    source_->setEnabled(editable);
    refresh_->setEnabled(editable);
    sync_latency_->setEnabled(editable);
}

// ---------------------------------------------------------------------------
// onEnableToggled：「启用/停止」切换（运行闸）→ 写 asset.live_enabled +
// 全字段落定（启停经调度器指纹传播：Hub 拆/建收端与组播、display 拆/建
// 收流）+ 重铺锁定态。无绑定素材（未绑定/已删）：状态弹回。
// 刻意不走 onEdited（其含"启用中禁写"守卫，会拦掉本次合法写回）。
// ---------------------------------------------------------------------------
void LiveClipSection::onEnableToggled(bool on) {
    Asset* a = writableAsset();
    if (!a || a->kind != AssetKind::Live) {
        const QSignalBlocker b(enable_);
        enable_->setChecked(!on); // 弹回：无素材可写
        return;
    }
    applyTo(*a); // enable_ 已是用户点击后的新态 → live_enabled + 全字段
    syncEnableUi();
    if (ctx_.on_edited) ctx_.on_edited(); // 落定：sceneEdited 自动下发
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
    // 【2026-11-23 必须同步】延迟模式恒同步（低延迟档退役）；帧同步值
    // 从档位取（下拉已无监视档，恒 >0）
    a.live_latency = 1;
    a.live_target_latency_us = sync_latency_->currentData().toLongLong();
    // 组播转发恒启用（2026-11-23 用户定案）：live_url = 确定性组播地址
    // （multicastUrlFor；用户不经 UI 改）。跨机 display 唯一拿画面通道——
    // 源设备在 display 机器上不存在，按名直收必黑屏。
    a.live_url = multicastUrlFor(a.id);
    // 启用闸（2026-11-23）：按钮 = 捕获+下发唯一运行开关，随场景 pb
    // 分发（display 同步启停）；参与相等比较与调度器指纹。
    a.live_enabled = enable_->isChecked();
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
    const QSignalBlocker b3(sync_latency_);
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
    sync_latency_->setCurrentIndex(
        nearestSyncIndex(sync_latency_,
                         a.live_target_latency_us > 0
                             ? a.live_target_latency_us
                             : kSyncLatencyDefaultUs));
    // 组播地址恒展示（恒启用语义；applyTo 恒生成——此处按资产实际值显示，
    // 存量空值等下次 applyTo/下发归一化）
    mcast_url_->setText(QStringLiteral("组播地址: ") +
                        QString::fromStdString(a.live_url));
}

// ---------------------------------------------------------------------------
// onRefreshSources：当前接入方式的源列表重灌（保留当前选择；过滤再发布流）
// ---------------------------------------------------------------------------
void LiveClipSection::onRefreshSources() {
    // 【启用闸】运行中不重枚举：clear()/setCurrentIndex() 会触发
    // currentIndexChanged → onEdited 写回（等同切换源，启用中禁止）
    if (currentEnabled()) return;

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
// 绑定素材定位：按 clip->asset_id 现查工作副本素材表（换绑安全——
// 每次现查不缓存；未绑定/类型不符返回 nullptr，写回与灌值统一防御）
// 【2026-11-15 悬垂修复】clip 本体经 boundClip() 现查（vector 搬移安全）
// ---------------------------------------------------------------------------
Clip* LiveClipSection::boundClip() {
    if (ctx_.clip_lookup) {
        if (Clip* c = ctx_.clip_lookup()) return c;
    }
    return ctx_.clip;
}

const Clip* LiveClipSection::boundClip() const {
    if (ctx_.clip_lookup) {
        if (Clip* c = ctx_.clip_lookup()) return c;
    }
    return ctx_.clip;
}

const Asset* LiveClipSection::boundAsset() const {
    const Clip* c = boundClip();
    if (!c || !ctx_.scene || c->asset_id.empty()) return nullptr;
    return ctx_.scene->findAsset(c->asset_id);
}

Asset* LiveClipSection::writableAsset() {
    const Clip* c = boundClip();
    if (!c || !ctx_.scene || c->asset_id.empty()) return nullptr;
    for (auto& asset : ctx_.scene->assets)
        if (asset.id == c->asset_id) return &asset;
    return nullptr;
}

// ---------------------------------------------------------------------------
// 【ClipSectionInterface】宿主广播灌值：const ms::Clip* 经 void* 穿越 →
// 按其当前绑定现查素材 → refreshFrom（焦点保护 + 防回环见 refreshFrom）
// ---------------------------------------------------------------------------
void LiveClipSection::refreshFromClip(const ms::Clip* clip) {
    // 签名已类型化（P3-B），无需 static_cast 还原
    if (!clip || !ctx_.scene || clip->asset_id.empty()) return;
    const Asset* a = ctx_.scene->findAsset(clip->asset_id);
    if (!a || a->kind != AssetKind::Live) return;
    syncEnableUi(); // 启用状态跟素材走（面板重建/广播灌值后按素材现值恢复）
    refreshFrom(*a);
}

// ---------------------------------------------------------------------------
// onEdited：任一控件变更 → 直写素材 live_* 全字段 + on_edited 落定
//（原适配器 applyClipLiveEdit 职责内化，2026-11-13 片段插件化）
// ---------------------------------------------------------------------------
void LiveClipSection::onEdited() {
    Asset* a = writableAsset();
    if (!a || a->kind != AssetKind::Live) return; // 已删/未绑定：静默忽略

    // 【启用闸 2026-11-23】运行中禁止任何 live_* 写回（防御纵深：控件虽
    // 已置灰，信号竞态/程序化触发仍可能到达）——改 live_* 即改素材指纹，
    // 调度器会析构重建连接中的源实例（偶发崩溃根因）。合法启停写回走
    // onEnableToggled（直写 applyTo，不经本守卫）。
    if (currentEnabled()) return;

    applyTo(*a); // 控件值 → asset.live_* 全字段

    if (ctx_.on_edited) ctx_.on_edited(); // 宿主落定：sceneEdited + 广播
}

} // namespace ms
