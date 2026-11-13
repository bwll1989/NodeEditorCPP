// ============================================================================
//  SceneTreeWidget.hpp —— 场景树 dock 控件（第十迭代：快速选中被遮挡图元）
//
//  背景：画布上素材与 display 矩形常互相遮挡，被压在下面的图元点不中
//  （命中测试只取 z 最高者）。场景树把 全部屏幕 + 全部素材 + 全部轨道片段
//  列成树，点击节点即选中画布对应图元——与画布单击选中完全同一条选中链路。
//
//  结构（2026-11-05 起第一级 = scene 根，整树 = 当前场景的格式化展示；
//        双列：名称 | 信息）：
//    场景 · scene-xxx                [右键：同步媒体/下发场景/适应窗口/
//                                    放弃本地编辑；信息列=屏数/轨数/时长。
//                                    注：Scene 无"大小"语义（演出=屏幕
//                                    集合+时间轴），画布尺寸不在此展示]
//      ├─ 屏幕 (N)                   [右键：连接/断开此显示端、属性]
//      │   └─ scr-1 → player-1 ●在线 [kind=1, a=屏幕下标]  | 1920×1080 @(0,0)
//      └─ 视频轨 · 主轨 (K)          [轨道分组行，染类型色]
//          └─ clip-1                 [kind=4, a=轨道下标, b=片段下标] | 0:03.0–0:33.0
//
//  【2026-11-05 两级结构】第一级 = scene 根节点（右键 = 场景级四动作：
//    同步媒体/下发场景/适应窗口/放弃本地编辑——原顶部按钮行的最终归处）；
//    第二级 = 屏幕组/轨道组；第三级 = 屏幕/片段节点。
//
//  【场景树增强 2026-11-04】优化 + 功能丰富（本迭代）：
//    * 双列布局：信息列直读（片段时间区间/素材分辨率+引用数/屏幕几何），
//      悬停 tooltip 保留完整详情；
//    * rebuild 保留展开状态与滚动位置（原先每次全量重建强制全展开，
//      用户手动折叠后任何编辑都会被顶回去）；
//    * 播放联动：markPlayhead(t) 由 EditorWindow 低频驱动，活跃片段
//      （start ≤ t < start+duration）加 ▶ 前缀——不播放时树里也一眼
//      看到此刻"谁在台上"；
//    * 片段右键：复制片段 / 删除片段（2026-11-05 用户定案收敛——定位
//      播放头与属性两项移除，属性面板唯一入口 = timeline 双击）；
//    * 素材右键（已移除 2026-11-05：场景树不再展示素材分组）；
//    * 屏幕右键：连接/断开 + 属性。
//
//  与画布的双向同步：
//    树点击 ──selectionRequested──► EditorWindow ──► canvas_->highlight
//    画布选中 ──selectionChanged──► EditorWindow ──► syncSelection（回高亮，
//                                     blockSignals 防递归）
//  下标约定与 SceneCanvasWidget::Selection 同码（1=screen 4=clip）——
//  树不持有场景数据语义，重建只在内容变化时由 EditorWindow 驱动。
// ============================================================================

#pragma once

#include <QColor>
#include <QTreeWidget>

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "model/SceneModel.hpp"

namespace ms {

class SceneTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    /// display 运行状态表：player_id → 状态三元组
    /// rebuild/refreshDisplayStatuses 时用于节点后缀标注（●在线/（已连接）
    /// /（离线）/已停用）；右键菜单据此显示"连接/断开"。
    /// nullptr = 不显示状态（纯结构树）。
    struct DisplayState {
        bool online     = false; ///< TCP 在线且已完成注册（收到过注册帧）
        bool enabled    = true;  ///< false = 用户已停用（不连接）
        bool connected  = false; ///< TCP 在线但尚未注册（连接刚建立，
                                  ///<  等待注册帧/指派 id 往返完成）
    };
    using DisplayStates = std::map<std::string, DisplayState>;

    explicit SceneTreeWidget(QWidget* parent = nullptr);

    /// 全量重建（场景内容变化时由 EditorWindow 调用；nullptr = 清空）
    /// 保持画布选中同步由调用方随后 syncSelection 完成
    /// 【展开保持 2026-11-04】重建前记录各分组的展开态，重建后恢复
    /// （新分组默认展开）；滚动位置一并保留。语义变化：原先 expandAll()
    /// 会把用户手动折叠的分组在每次编辑（50ms 防抖全量重建）后顶开。
    /// \param states 显示端在线/启用状态（可空：不标注运行状态）
    void rebuild(const Scene* scene,
                 const DisplayStates* states = nullptr);

    /// 画布选中变化 → 树高亮对应节点（无匹配 = 清除选择；防递归）
    void syncSelection(int kind, int a, int b);

    /// 【播放联动 2026-11-04】按当前播放头标记活跃片段（▶ 前缀）。
    /// 由 EditorWindow 低频定时器（200ms UI 拍）驱动——树内部只遍历
    /// rebuild 时缓存的片段节点列表（非全树扫描），幂等（状态不变时
    /// 不改文本，避免无谓重绘）。
    void markPlayhead(std::int64_t t_us);

    /// 【素材同步进度 2026-11-04】各显示端的素材拉取进度标注（⤓ 前缀）。
    /// display_id → 进度文案（"⤓ 同步 2/5 · 34%" / "✓ 已同步"）；无该
    /// display 的条目 = 清除标注（离线/停滞）。由 EditorWindow 200ms UI
    /// 拍驱动（EditorRuntime::assetSyncSnapshot 过滤拼接）；树内部按
    /// rebuild 缓存的屏幕节点列表更新，幂等（文案不变零重写）。
    void updateSyncProgress(const std::map<QString, QString>& sync_texts);

    /// 【连接状态轻量刷新 2026-11-06】按传入快照更新屏幕行的在线状态
    /// 后缀与颜色（"●在线"/"（离线）"等），不重建整树。由 EditorWindow
    /// 200ms UI 拍驱动（与状态栏"显示端=N/M"同拍）——连接状态变化不再
    /// 依赖编辑动作触发 rebuild 才可见（修复"display 已连接但树恒红"）。
    /// 同时刷新 display_states_（右键菜单连接/断开文案同源）。
    void refreshDisplayStatuses(const DisplayStates& states);

signals:
    /// 单击节点 → 请求画布选中对应图元（kind 1=screen 4=clip；同码画布）
    void selectionRequested(int kind, int a, int b);
    /// 双击节点 → 请求弹出属性对话框（与画布双击同路径）
    void propertiesRequested(int kind, int a, int b);
    /// 屏幕节点右键 → 单独连接/断开该 display（display_id = 绑定 player_id）
    /// EditorWindow 侧转 DisplayConnector::setEnabled；断开 = 立即断 TCP
    /// 且看门狗不再自动重连；连接 = 恢复自动连接
    void displayConnectRequested(const QString& display_id, bool connect);
    // 【2026-11-05 移除】素材节点右键三信号（删除/添加到画布/打开文件夹）
    // ——场景树不再展示素材分组，EditorWindow 侧槽已同步删除。
    // 【2026-11-05 移除】clipSeekRequested（片段右键"定位播放头到片段
    // 起点"菜单项一并删除——定位走时间轴拖播放头，用户定案收敛右键）。
    /// 【片段右键 2026-11-04】复制片段：同轨克隆（新 id 线性探测，
    /// start 顺延到原片段结束点），置脏 + 自动下发
    void clipDuplicateRequested(int track_idx, int clip_idx);
    /// 【片段右键 2026-11-04】删除片段（与画布右键删除同码：
    /// EditorWindow::onDeleteClipAt 统一落点）
    void clipDeleteRequested(int track_idx, int clip_idx);

    // ---- 【2026-11-05】全局动作四信号（原顶部按钮行迁入右键菜单）----
    // 任意位置（含空白区/分组行）右键均可见——语义与原按钮完全一致，
    // EditorWindow 侧接既有槽（onPublishScene/onSyncMedia/onReloadScene）。
    /// 下发场景：工作副本全量广播给所有显示端（含空绑定弹窗指引）
    void publishSceneRequested();
    /// 同步媒体：注册素材进分发服务 + 推清单给在线显示端（幂等）
    void syncMediaRequested();
    /// 放弃本地编辑：权威场景 → 工作副本（本地未下发修改丢弃）
    void reloadSceneRequested();
    /// 适应窗口：画布视图复位（整个 scene 铺满预览区）
    void fitViewRequested();
    // 【2026-11-13 移除】addDisplayRequested 信号（场景树不再提供添加显示
    // 端入口——单入口约定：画布"添加屏幕"自动连带登记；改绑走屏幕属性
    // 对话框"绑定"页）

protected:
    /// 右键菜单：屏幕节点（连接/断开、属性）、片段节点（复制/删除——
    /// 2026-11-05 定位与属性两项已移除）、场景级全局动作四项的功能入口
    void contextMenuEvent(QContextMenuEvent* e) override;

private:
    /// 最近一次 rebuild 时保存的显示端状态（右键菜单文案取值用）
    SceneTreeWidget::DisplayStates display_states_;

    /// 【展开保持】分组展开态：分组标识（":screens"/":assets"/"t-video-1"
    /// 等轨道 id）→ 是否展开；rebuild 前从树里收集、重建后恢复
    std::map<QString, bool> expanded_;

    /// 【播放联动】rebuild 时缓存的片段节点：(start_us, duration_us, 节点)
    /// —— markPlayhead 只遍历本列表；rebuild/clear 时重建/清空
    struct ClipNode {
        std::int64_t start_us  = 0;
        std::int64_t length_us = 0;
        QTreeWidgetItem* item  = nullptr;
        bool active            = false; // 上次标记状态（幂等判据）
    };
    std::vector<ClipNode> clip_nodes_;

    /// 【素材同步进度 2026-11-04】rebuild 时缓存的屏幕节点（display_id
    /// 非空才有标注价值）：基础文本（id→player+在线态，进度追加于其后）
    /// + 基础着色（进度清除时恢复）。
    struct ScreenNode {
        QString player_id;
        QString prefix;       ///< "scr-1 → player-1"（不含状态后缀）——
                              ///<  refreshDisplayStatuses 重组文本用
        QString base_text;    ///< prefix + 状态后缀（进度追加于其后）
        QString sync_suffix;  ///< 当前素材同步标注（updateSyncProgress 维护；
                              ///<  refreshDisplayStatuses 组合时要拼上）
        QColor  base_color;
        QTreeWidgetItem* item = nullptr;
    };
    std::vector<ScreenNode> screen_nodes_;
};

} // namespace ms
