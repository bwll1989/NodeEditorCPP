// ============================================================================
//  SceneHistory.hpp —— 场景级撤销/重做历史（快照式，纯 C++ 值栈，无 Qt）
//
//  定位（设计文档 docs/时间轴画布撤销还原机制_设计方案_2026-11-26.md）：
//    权威数据 = ms::Scene 纯值结构（全链 operator== 齐全）——每条历史
//    记录 = {label, before, after} 三份场景快照。恢复 = 宿主把快照整体
//    重灌（loadSceneToModel 自带 *work_=scene + loading_ 屏蔽 + 全 UI
//    重建 + 发送快照重建），本类只管栈，不碰任何 UI。
//
//  两种收编形态（对应两类编辑节奏）：
//    * 会话型（拖拽）：beginEdit(按下) → endEdit(松手)——一次会话一条
//      历史；before 在 begin 时冻结一次。
//    * 离散型（删片段/属性面板/方向键……）：checkpoint(label, cur)——
//      写后调用；before 取「上一条历史之后的场景」（不变量：undo 栈顶
//      after 恒等于当前场景，见下）。
//
//  ★ 核心不变量：undo_.back().after == 当前场景（redo 侧对称：栈顶
//    before == 当前场景）。据此，离散型操作无需在写前插桩——写后
//    checkpoint 即可取到正确 before。该不变量由三方共同维护：
//      - 每次入栈 after = 当前场景（endEdit/checkpoint 实参）
//      - undo/redo 返回快照后，宿主恢复，栈随之搬移（takeUndo/takeRedo
//        把"当前场景"作为反向条目存好）
//      - 所有编辑路径都经过插桩（漏斗/会话/离散）——未插桩路径会破坏
//        不变量（表现为 undo 恢复到错误基线），新增编辑入口必须插桩。
//
//  合并窗口（kMergeMs）：同名离散操作在窗口内吸收进上一条（只更新
//    after，before 保持最初基线）——滑杆连发/方向键自动重复/连续删除
//    折叠为一条历史；跨窗口或异名 = 新条目。
//
//  恢复抑制：宿主执行 undo/redo 恢复期间 setActive(false)——恢复过程
//    （全量重灌触发的镜像回推/信号）不会把恢复动作当新编辑入栈。
// ============================================================================

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "model/SceneModel.hpp" // ms::Scene（值语义；operator== 全链）

namespace ms {

class SceneHistory {
public:
    static constexpr std::size_t kMaxDepth = 60; // 限深（防大工程内存失控）
    static constexpr int kMergeMs = 400;         // 同名离散操作合并窗口

    SceneHistory() = default;

    // ---- 基线与开关 ---------------------------------------------------------
    /// 工程（重）装载后调用：无历史时以当前场景为 undo 尽头的基线
    ///（第一条 checkpoint 的 before 来源）。in_edit_ 时拒绝（会话中不换基线）。
    void setBase(const Scene& s) {
        if (in_edit_) return;
        base_ = s;
    }
    /// 恢复期间关闭收编（undo/redo 重灌触发的回写不得入栈）。
    /// 关闭时强制丢弃未收口会话（恢复不允许发生在会话中途——宿主已挡）。
    void setActive(bool a) {
        active_ = a;
        if (!a) in_edit_ = false;
    }
    bool active() const { return active_; }
    /// 有未收口会话（宿主据此拒绝 undo/redo——拖拽中途恢复会悬挂借点）
    bool inEdit() const { return in_edit_; }

    // ---- 会话型 API（画布/时间轴拖拽）--------------------------------------
    /// 会话开始：立刻冻结 before 基线（拖拽全程只此一次深拷贝）。
    /// 嵌套防御：上一会话未收口则以 cur 收口（.live 编辑交叉时兜底）。
    void beginEdit(const std::string& label, const Scene& cur) {
        if (!active_) return;
        if (in_edit_) endEdit(cur);
        in_edit_ = true;
        label_   = label;
        pending_ = cur;
    }
    /// 会话收口：dedup（无变化丢弃）→ 合并窗口 → 入栈。
    /// 未开会的调用无害（in_edit_ 守卫）。
    void endEdit(const Scene& cur) {
        if (!active_ || !in_edit_) return;
        in_edit_ = false;
        commit(label_, pending_ ? *pending_ : cur, cur);
        pending_.reset();
    }

    // ---- 离散型 API（结构操作/属性漏斗/方向键）-----------------------------
    /// 一步式收编（写后调用）：before = 上一条历史之后的场景（核心不变量）。
    /// 开着的会话先以 cur 收口（离散动作打断拖拽会话的兜底）。
    void checkpoint(const std::string& label, const Scene& cur) {
        if (!active_) return;
        if (in_edit_) {
            endEdit(cur);
            return;
        }
        const Scene& before = undo_.empty() ? base_ : undo_.back().after;
        commit(label, before, cur);
    }

    // ---- 栈操作（宿主调用；返回要恢复的快照）--------------------------------
    bool canUndo() const { return active_ && !in_edit_ && !undo_.empty(); }
    bool canRedo() const { return active_ && !in_edit_ && !redo_.empty(); }
    /// \return 要恢复到的场景（undo 目标）；空 = 不可 undo
    std::optional<Scene> takeUndo(const Scene& cur) {
        if (!canUndo()) return std::nullopt;
        Entry e = std::move(undo_.back());
        undo_.pop_back();
        redo_.push_back(Entry{e.label, cur, e.after});
        return e.before;
    }
    /// \return 要恢复到的场景（redo 目标）；空 = 不可 redo
    std::optional<Scene> takeRedo(const Scene& cur) {
        if (!canRedo()) return std::nullopt;
        Entry e = std::move(redo_.back());
        redo_.pop_back();
        undo_.push_back(Entry{e.label, cur, e.after});
        if (undo_.size() > kMaxDepth) undo_.erase(undo_.begin());
        return e.after;
    }
    /// 下一动作的描述文案（菜单/状态栏："撤销：移动片段"）；空栈 = 空串
    const std::string& undoLabel() const {
        static const std::string kEmpty;
        return undo_.empty() ? kEmpty : undo_.back().label;
    }
    const std::string& redoLabel() const {
        static const std::string kEmpty;
        return redo_.empty() ? kEmpty : redo_.back().label;
    }

private:
    struct Entry {
        std::string label;
        Scene before; ///< 操作前场景（undo 恢复目标）
        Scene after;  ///< 操作后场景（= 入栈时刻当前场景，redo 恢复目标）
    };

    /// 收编内核：无变化丢弃 → 合并窗口吸收 → 入栈（清 redo + 限深）
    void commit(const std::string& label, const Scene& before,
                const Scene& after) {
        if (before == after) return; // 无实际变化（点击未拖/改回原值）不入栈
        const auto now = std::chrono::steady_clock::now();
        const auto since_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  now - last_commit_)
                                  .count();
        if (!undo_.empty() && undo_.back().label == label &&
            since_ms < kMergeMs) {
            undo_.back().after = after; // 吸收：before 保持最初基线
            redo_.clear();
            last_commit_ = now;
            return;
        }
        undo_.push_back(Entry{label, before, after});
        if (undo_.size() > kMaxDepth) undo_.erase(undo_.begin());
        redo_.clear();
        last_commit_ = now;
    }

    std::vector<Entry> undo_, redo_;
    std::optional<Scene> pending_; ///< 会话 before 基线（begin 冻结）
    std::string label_;            ///< 进行中会话的描述
    Scene base_;                   ///< 无历史时的 undo 尽头（工程装载基线）
    bool in_edit_ = false;
    bool active_  = true;
    std::chrono::steady_clock::time_point last_commit_{
        std::chrono::steady_clock::time_point::min()}; ///< 合并窗口基准
};

} // namespace ms
