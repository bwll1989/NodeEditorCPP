// ============================================================================
//  NodeController.hpp —— standalone 节点控制器（模式仲裁 + 切换编排 + 播放头）
//
//  定位（display×MMC 合并方案 v2.2，2026-11-11）：
//    display 的 standalone 模式下，NodeController 是"本机的 Editor"：
//    持多时间轴注册表（TimelineRegistry），把用户选中的时间轴装配产物
//    注入 PlayerRuntime 的场景槽并驱动其播放头——**同一 SceneModel、
//    同一渲染引擎链、同一状态机**（本机控制 = 立即锚点走 applyAnchorLocked，
//    零新状态机代码）。两种模式只是"任务下发的位置"不同：
//      sync 模式：Editor（TCP 17100）→ PlayerRuntime
//      standalone：NodeController（本机）→ PlayerRuntime
//
//  两级仲裁同语义"谁的内容进场景槽"（v2.2 定案）：
//    * 控制面级：Editor 在场全让位（M3 auto 模式实现；本类只管 standalone）；
//    * 时间轴级：多时间轴唯一活跃——启动 B = 停 A → 换场景槽 → 播放头
//      归零，场景替换一帧内完成（渲染线程经 sceneSerial 判新，引擎零感知）。
//
//  出点行为（loop / stop_position）：挂在 tick 包装里——PlayerRuntime
//  不管出点（sync 模式下出点语义由 Editor 决定），standalone 由本类按
//  活跃时间轴的编排属性执行：到点 → loop 回头重播 / 停止归零。
//
//  线程模型（M2 起）：play/pause/seek/stop 可从 HTTP 工作线程并发进入，
//  tick 留在 main 主线程——出点参数快照（loop_/stop_position_us_）经
//  param_mutex_ 保护（play/stop 写、tick 读，均为小整型赋值，锁开销可忽略）；
//  场景/播放头状态全部收敛在 PlayerRuntime::state_mutex_（方法组内部持锁），
//  注册表经自身 mutex_ 串行——本类不加"大锁"，仅守自有快照字段。
// ============================================================================

#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include "runtime/PlayerRuntime.hpp"
#include "standalone/TimelineRegistry.hpp"

namespace ms::standalone {

class NodeController {
public:
    NodeController(PlayerRuntime& player, TimelineRegistry& registry);

    // ---- 时间轴编排（M2 HTTP API 的直接后端）--------------------------------

    /// 时间轴注册表（HTTP CRUD / CLI 透传）
    TimelineRegistry& registry() { return registry_; }

    /// 启动指定时间轴：停当前 → 场景槽替换 → 播放头归零起播。
    /// \return false = 不存在 / 场景为空（err 说明）
    bool play(const std::string& id, std::string& err);

    /// 暂停（冻结于当前播放头）
    void pause();

    /// 重定位（状态保持：播中跳转续播 / 暂停中跳转冻结）
    void seek(std::int64_t to_us);

    /// 续播（从当前播放头重锚起播；Playing 态下重锚同刻幂等无副作用——
    /// localPlay 的立即锚点语义：session_start = now - 当前头）
    void resume() { player_.localPlay(playheadUs()); }

    /// 停止：播放头归零 + 清活跃（场景槽保留最后一帧场景——与受控模式
    /// STOP 后场景仍在同语义；下一次 play 换场景）
    void stop();

    /// 开机自动播放（v2.2 定案）：优先默认时间轴（registry.defaultId），
    /// 回退 fallback_id（CLI --timeline= / config.ini default_timeline）。
    /// \return false = 两者皆不可用（err 说明；节点保持待机不视为故障）
    bool playDefault(const std::string& fallback_id, std::string& err);

    // ---- 主循环节拍（main 每帧调用）------------------------------------------

    /// player.tick + 出点行为（loop 回头 / 停止归零）。
    /// 【顺序纪律】先推进再查出点——本拍推进越线的播放头下一拍才被
    /// 捕获（10ms 粒度），出点精度即主循环节拍，满足 MVP 语义。
    void tick(std::int64_t aligned_now_us);

    // ---- 状态观测（HTTP status / CLI）-----------------------------------------

    std::int64_t playheadUs() const { return player_.session_time_us(); }
    PlayState    state() const { return player_.state(); }
    bool         hasActiveTimeline() const { return !registry_.activeId().empty(); }

private:
    /// 活跃时间轴的出点（us；0 = 无出点控制——场景时长未知/无活跃）。
    /// play() 时快照，tick() 只读——避免每拍查表+解引用（param_mutex_ 保护）
    std::int64_t effectiveStopUs() const;

    /// 快照出点参数（play/stop 写入用；param_mutex_ 内赋值）
    void snapshotParams(bool loop, std::int64_t stop_us);

    PlayerRuntime&   player_;
    TimelineRegistry& registry_;

    /// 出点行为参数（play()/stop() 写、tick() 读——HTTP 线程与主线程并存）
    mutable std::mutex param_mutex_;
    bool         loop_             = false;
    std::int64_t stop_position_us_ = 0;
};

} // namespace ms::standalone
