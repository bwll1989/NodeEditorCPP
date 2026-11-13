// ============================================================================
//  ScreenWarpPanel.hpp —— 屏幕输出变形编辑面板（2026-11-11 从 ScreenWarpDialog
//  抽取的可内嵌主体；WATCHOUT「几何结构」页语义）
//
//  WATCHOUT 式输出变形：把本机输出画面按可拖拽的控制点网格贴到物理
//  屏幕（投影仪堆叠错位 / 弧形幕 / 异形 LED 的几何校正场景）。
//
//  与对话框的关系：
//    * 本面板 = 原对话框的全部编辑主体（模式/密度/重置/网格编辑区/提示）；
//      ScreenWarpDialog 退化为薄壳（panel + Ok/Cancel），行为不变，
//      test_screen_warp_dialog 继续经薄壳验证（控件 objectName 不变）；
//    * ScreenPropertyDialog 的「几何结构」页直接内嵌本面板（用户需求：
//      不要弹窗）——编辑中实时变化经 setLiveApply 回调转发宿主。
//
//  交互约定（与原对话框同源）：
//    * 构造预填当前 warp（无变形时以 4×4 恒等网格作为编辑起点，但模式下
//      拉仍为"无变形"——不写回用户未动过的状态）；
//    * 模式二选一：无变形（直出）/ 手动网格（拖拽编辑）；MPCDI 项禁用
//      占位（枚举先立位，本轮不实装）；
//    * 编辑区自绘：实线 = 变形网格（节点位置的分片双线性曲面）、
//      虚线 = 恒等参考；拖拽节点即变形；密度变更 = 双线性保形重采样；
//    * setLiveApply：每次变化（拖拽/密度/重置/切模式）实时回调当前状态。
//  融合（blending / alpha mask）属同族后续能力，本面板暂不涉及。
// ============================================================================

#pragma once

#include <QWidget>

#include <functional>

class QComboBox;
class QSpinBox;
class QPushButton;

namespace ms {

struct ScreenWarp; // 前置声明（须与 SceneModel 的 struct 声明一致——
                   // class/struct 混用会让 MSVC 改名不同 → LNK2019）

class ScreenWarpPanel : public QWidget {
    Q_OBJECT
public:
    /// \param initial 当前 warp（预填；无变形时以 4×4 恒等网格起步）
    /// \param parent  Qt 父控件
    explicit ScreenWarpPanel(const ScreenWarp& initial,
                             QWidget* parent = nullptr);
    ~ScreenWarpPanel() override;

    /// 把编辑状态写回 warp（applyTo 语义；属性框 OK 时调用）
    void applyTo(ScreenWarp& w) const;

    /// 【实时预览】注入实时应用回调——编辑过程中（拖拽节点/改密度/重置/
    /// 切模式）每次变化立即以当前编辑状态调用回调，由宿主写回场景并走
    /// 自动下发链路，display 端拖动即见变形。空 = 不实时应用。
    void setLiveApply(std::function<void(const ScreenWarp&)> fn) {
        live_apply_ = std::move(fn);
    }

private slots:
    /// 模式切换：无变形时禁用密度/编辑区，手动网格时启用
    void onModeChanged(int index);
    /// 密度（列×行）变更：双线性重采样旧节点位置到新网格（保形）
    void onDensityChanged();
    /// 重置：当前密度下的恒等网格（全部节点回位）
    void onReset();

private:
    /// 当前编辑状态 → ScreenWarp（applyTo/实时回调共用同一构造）
    ScreenWarp current() const;
    /// 把当前编辑状态经 live_apply_ 实时发出（未注入则空操作）
    void emitLive();

    QComboBox*   mode_  = nullptr; ///< 变形模式（无/手动网格/MPCDI 禁用）
    QSpinBox*    cols_  = nullptr; ///< 网格列数（节点数，2..16）
    QSpinBox*    rows_  = nullptr; ///< 网格行数（同上）
    QPushButton* reset_btn_ = nullptr; ///< 恒等重置

    /// 实时应用回调（宿主注入；拖拽/密度/重置/模式变化时触发）
    std::function<void(const ScreenWarp&)> live_apply_;

    /// 自绘拖拽编辑区（实现见 .cpp；无 Q_OBJECT——纯绘制/鼠标交互，
    /// 通知经 std::function 回调，免 moc 依赖）
    class WarpEditArea;      ///< 嵌套前置声明（定义在 .cpp）
    WarpEditArea* edit_ = nullptr;
};

} // namespace ms
