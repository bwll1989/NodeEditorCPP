// ============================================================================
//  ClipPropertyPanel 实现 —— 片段自定义属性区（媒体文件绑定）
//
//  【2026-11-06 方案 A 用户拍板】画布变换区（X/Y/Z/宽/高/旋转/不透明度）
//  整体退役——七属性编辑并入 TweenSection 合并区（appendSection 追加，
//  值恒显示播放头处最终结果 + 双模直写），消除"同一属性两处显示/两个值"
//  的混乱。本面板保留：媒体文件（导入绑定 + 文件名展示）+ 时长联动。
//  宽高自动回填随迁至适配器 applyClipPanelMerge 的素材变化分支（绑定
//  新素材时未设尺寸自动补素材原始宽高——原 adoptImportedAsset 语义）。
//
//  编辑域自 ClipPropertyDialog 提炼，交互 = 即时生效（控件变更即经
//  applier 写回），与库通用面板（ClipTimePanel spinbox 即改即生效）
//  的交互语义对齐——附加进组合面板后体验与原版 QtTimeLine 一致。
// ============================================================================

#include "ClipPropertyPanel.hpp"

#include <cstdint>

#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "model/SceneModel.hpp"

namespace ms {

ClipPropertyPanel::ClipPropertyPanel(const Clip& clip,
                                     MediaLibrary::Category media_category,
                                     const QString& binding_name,
                                     QWidget* parent)
    : QTabWidget(parent),
      current_asset_id_(clip.asset_id),
      current_binding_name_(binding_name),
      snapshot_(clip),
      effective_duration_us_(clip.duration_us) {
    // 【2026-11-13 三页式改版（用户定案）】本面板即 QTabWidget 页容器：
    //   第 1 页「通用」= 媒体文件区（库通用面板的时间属性/片段属性由
    //   BaseClass initPropertyWidget 识别 Tab 结构后插入本页开头）；
    //   第 2 页「动画」/第 3 页专属区由适配器经 appendSection 追加。
    general_page_ = new QWidget(this);
    auto* lay = new QVBoxLayout(general_page_);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->setSpacing(6);

    // ---- 媒体文件（2026-11-12 用户定案：文件选择经媒体库 SelectorComboBox，
    // 替换原 QFileDialog 任意路径选择——素材统一走媒体库管理）--------------
    // 【2026-11-13 布局重构】两行结构：行 1 = 媒体库选择器（内嵌展开钮），
    // 行 2 = 绑定详情（asset id 可复制）。旧实现 asset_label_ 未加入布局，
    // QLabel 脱管飘在 QGroupBox 左上角与标题重叠（用户报"媒体文件挤在一起"）。
    // 候选 = 媒体库对应分类的文件名列表（适配器按片段类型传分类）；当前
    // 绑定文件名灌入选择器显示。
    auto* asset_box = new QGroupBox("媒体文件", this);
    asset_box->setFlat(true); // 带文字的分割线（2026-11-13 用户定案：四区块统一去框）
    media_box_ = asset_box; // 存成员：Live 片段经 setMediaSectionVisible 隐藏
    auto* asset_lay = new QVBoxLayout(asset_box);
    asset_lay->setContentsMargins(8, 2, 8, 6);
    asset_lay->setSpacing(4);
    media_selector_ = new SelectorComboBox(media_category, asset_box);
    media_selector_->setObjectName("clipPickMedia"); // 测试定位锚（沿用原锚）
    media_selector_->setAllowCustomInput(false); // 绑定必须来自媒体库候选
    media_selector_->setToolTip("从媒体库选择文件绑定到本片段"
                                "（候选为媒体库当前分类下的文件）");
    if (!current_binding_name_.isEmpty())
        media_selector_->setCurrentValue(current_binding_name_);
    asset_lay->addWidget(media_selector_, /*stretch=*/1);
    asset_label_ = new QLabel(asset_box);
    asset_label_->setObjectName("clipAssetLabel"); // 测试定位锚（与对话框同锚）
    asset_label_->setTextInteractionFlags(Qt::TextSelectableByMouse); // 可复制
    asset_label_->setStyleSheet("color:#888; font-size:11px;");
    updateAssetLabel(); // 预填当前媒体绑定详情（快照 asset_id；文件名由选择器展示）
    asset_lay->addWidget(asset_label_);
    connect(media_selector_, &SelectorComboBox::selectionChanged,
            this, &ClipPropertyPanel::onMediaSelected);
    lay->addWidget(asset_box);
    lay->addStretch(1); // 页内容不足时余量沉底（时间属性等插入后仍在尾部）
    addTab(general_page_, tr("通用")); // 第 1 页（this 即页容器）

    // 【画布变换区已退役（2026-11-06 方案 A）】七属性编辑并入 TweenSection
    // 合并区（适配器 makeClipPropertyPanel 统一追加）——此处不再创建任何
    // transform 控件；预填/信号连接/applyTo/refreshFrom 的 transform 部分
    // 一并移除。
}

// ---------------------------------------------------------------------------
// 附加一页（2026-11-13 三页式改版）：追加顺序即页序（适配器先「动画」后
// 专属）。所有权随页转移（addTab 重父化，随面板销毁）。
// ---------------------------------------------------------------------------
void ClipPropertyPanel::appendSection(QWidget* section, const QString& title) {
    if (!section) return;
    addTab(section, title);
}

// ---------------------------------------------------------------------------
// 媒体绑定区可见性（2026-11-09 Live 片段隐藏——与实况区功能重复）。
// QGroupBox 隐藏时 QVBoxLayout 自动回收占位（无 retainSize），下方追加区
// （实况区/文本区/变换区）随之上移补位；默认可见，其他类型行为不变。
// ---------------------------------------------------------------------------
void ClipPropertyPanel::setMediaSectionVisible(bool visible) {
    if (media_box_) media_box_->setVisible(visible);
}

// ---------------------------------------------------------------------------
// 面板脏字段 → 片段（合并语义：只覆盖 dirty_mask_ 置位字段；c 的其余字段
// 保持调用方填入的当前值——适配器传入的是工作副本最新值，防过期快照把
// 通用面板/拖拽的修改覆盖回退）。变换字段已迁 TweenSection（方案 A）。
// ---------------------------------------------------------------------------
void ClipPropertyPanel::applyTo(Clip& c) const {
    if (dirty_mask_ & kAssetId)  c.asset_id = current_asset_id_;
    if (dirty_mask_ & kDuration) c.duration_us = effective_duration_us_;
    // start_us/transform 不在本面板编辑域（前者归库通用面板 ClipTimePanel，
    // 变换七属性归 TweenSection 合并区——2026-11-06 方案 A），透传不动
}

// ---------------------------------------------------------------------------
// 采用一个已导入素材为本片段的绑定（onPickMediaFile 导入成功后的公共落点；
// 独立公开供测试/未来拖拽绑定复用）：
//   * 更新当前绑定（id + 展示名）→ 标脏 kAssetId；
//   * 视频/音频素材联动媒体真实时长（kDuration）；图片/文本无时长不联动；
//   * 【宽高联动已迁出（2026-11-06 方案 A）】视频/图片素材的原始宽高
//     回填由适配器 applyClipPanelMerge 在素材变化分支处理（写回后统一
//     落 transform，合并区经广播灌值显示）——本面板已无宽高控件。
//   * 随后统一走 onEdited 即时写回（面板语义：控件变更即生效）。
// ---------------------------------------------------------------------------
void ClipPropertyPanel::adoptImportedAsset(const Asset& a) {
    current_asset_id_    = a.id;
    current_binding_name_ = QFileInfo(QString::fromStdString(a.path)).fileName();
    dirty_mask_ |= kAssetId;
    if (a.duration_us > 0) { // 视频/音频联动媒体时长；图片/文本保持原值
        effective_duration_us_ = a.duration_us; // 面板无时长编辑控件（归通用
        dirty_mask_ |= kDuration;               // 面板），联动值经此透传写回
    }
    {   // 选择器显示跟随绑定（选中项本身已触发本链路，此灌值为幂等保护）
        const QSignalBlocker bs(media_selector_);
        media_selector_->setCurrentValue(current_binding_name_);
    }
    updateAssetLabel();
    onEdited(kAssetId);
}

// ---------------------------------------------------------------------------
// 媒体库选择器变更（2026-11-12 用户定案）：候选文件名 → allFiles() 反查
// 绝对路径 → 导入钩子（类型校验/入表）→ adoptImportedAsset（绑定+联动+写回）。
// 反查失败（库刚刷新/手动清空）不动作，选择器回显当前绑定。
// ---------------------------------------------------------------------------
void ClipPropertyPanel::onMediaSelected(const QString& file_name) {
    if (!importer_) return; // 未接导入能力（防御）
    if (file_name.isEmpty()) return;
    const QString path = resolveLibraryPath(file_name);
    if (path.isEmpty()) {
        // 候选过期（媒体库刷新后文件被移除等）：提示并回显当前绑定
        QMessageBox::warning(this, "指定媒体文件",
                             QString("媒体库中未找到文件：%1").arg(file_name));
        const QSignalBlocker bs(media_selector_);
        media_selector_->setCurrentValue(current_binding_name_);
        return;
    }
    std::string err;
    Asset a = importer_(path, err);
    if (a.id.empty()) {
        QMessageBox::warning(this, "指定媒体文件",
                             QString("导入失败：%1")
                                 .arg(QString::fromStdString(err)));
        const QSignalBlocker bs(media_selector_);
        media_selector_->setCurrentValue(current_binding_name_);
        return;
    }
    adoptImportedAsset(a); // 导入成功 → 绑定 + 时长联动 + 即时写回
}

// ---------------------------------------------------------------------------
// 媒体库文件名 → 绝对路径：allFiles()（路径列表）按文件名匹配
// （GestureNode resolveLibraryPath 同模式；命中多路时取第一个）
// ---------------------------------------------------------------------------
QString ClipPropertyPanel::resolveLibraryPath(const QString& file_name) const {
    const auto files = MediaLibrary::instance()->allFiles();
    for (const QString& p : files)
        if (QFileInfo(p).fileName() == file_name)
            return p;
    return {};
}

// ---------------------------------------------------------------------------
// 【双向同步·模型→面板向】外部变化灌显示（适配器经注册表广播调用，见
// QtTimelineModelAdapter::refreshClipPanels）。变换控件已迁合并区（方案 A），
// 仅剩媒体绑定展示。
// ---------------------------------------------------------------------------
void ClipPropertyPanel::refreshFrom(const Clip& c,
                                    const QString& binding_name) {
    // 媒体绑定展示：无条件跟工作副本（选择器/标签只读灌值无输入态；用户
    // 刚导入绑定的场景写回已先行发生，这里刷出的是同一值，无视觉扰动）
    current_asset_id_     = c.asset_id;
    current_binding_name_ = binding_name;
    {
        const QSignalBlocker bs(media_selector_); // 灌值不触发 selectionChanged
        media_selector_->setCurrentValue(binding_name);
    }
    updateAssetLabel();
    // 生效时长透传字段同步（面板无时长编辑控件；值经 applyTo 的 kDuration
    // 脏位或工作副本打底——这里保持与工作副本一致即可）
    effective_duration_us_ = c.duration_us;
}

// ---------------------------------------------------------------------------
// 只读绑定标签刷新：asset_id + 展示名 → 文本（超长中间省略，完整内容放
// 悬停提示）。未绑定时显示引导文案（与对话框同交互）。
// ---------------------------------------------------------------------------
void ClipPropertyPanel::updateAssetLabel() {
    if (current_asset_id_.empty()) {
        asset_label_->setText("（未绑定——从上方媒体库选择文件）");
        asset_label_->setToolTip({});
        return;
    }
    // 文件名已由媒体库选择器展示，本行补 id 详情（可复制）
    const QString id = QString::fromStdString(current_asset_id_);
    asset_label_->setText(fontMetrics().elidedText(id, Qt::ElideMiddle, 260));
    asset_label_->setToolTip(id);
}

// ---------------------------------------------------------------------------
// 任一编辑控件变更 → 标脏 + applier 写回（未注入回调时忽略）
// 【无参写回】写回实现（applyClipPanelMerge）从工作副本取当前值 + 面板
// dirty 字段合并——此处不再构造完整 Clip（原深拷贝快照被实现完全忽略，
// 每次控件变更白拷一次）。
// ---------------------------------------------------------------------------
void ClipPropertyPanel::onEdited(std::uint32_t field_bit) {
    dirty_mask_ |= field_bit;
    if (!applier_) return;
    applier_();
}

} // namespace ms
