// ============================================================================
//  NodeController.cpp —— standalone 节点控制器实现
//
//  切换编排顺序（v2.2 定案："启动 B = 停当前 → 换场景槽 → 播放头归零"）：
//    1. localStop()        —— 旧场景状态归位（Idle），播放头归零；
//    2. applyLocalScene()  —— 场景槽整体替换（serial++ → 渲染线程判新，
//                             下一帧即用新场景；媒体源按新素材表重建）；
//    3. setActive + 快照   —— 活跃登记 + 出点参数快照；
//    4. localPlay(0)       —— 立即锚点起播（session_start = now - 0）。
//  四步全在主线程串行完成，中间无 tick 插入——渲染线程只会观察到
//  "旧场景某帧 → 新场景第 0 帧"的一次性切换，无黑场无中间态。
// ============================================================================

#include "standalone/NodeController.hpp"

#include <cstdio>
#include <utility>

namespace ms::standalone {

NodeController::NodeController(PlayerRuntime& player,
                               TimelineRegistry& registry)
    : player_(player), registry_(registry) {}

// ---------------------------------------------------------------------------
// 出点解算
// ---------------------------------------------------------------------------
std::int64_t NodeController::effectiveStopUs() const {
    std::lock_guard<std::mutex> lock(param_mutex_);
    return stop_position_us_; // play() 时已快照（含"0 → 场景时长"换算）
}

void NodeController::snapshotParams(bool loop, std::int64_t stop_us) {
    std::lock_guard<std::mutex> lock(param_mutex_);
    loop_             = loop;
    stop_position_us_ = stop_us;
}

// ---------------------------------------------------------------------------
// 播放控制
// ---------------------------------------------------------------------------
bool NodeController::play(const std::string& id, std::string& err) {
    const auto entry = registry_.find(id);
    // 【2026-11-13 文案修正】原「时间轴不存在或未装配场景」把两种情况混成
    // 一句，且丢弃条目上现成的装配失败原因（note：暂无可播放片段/素材未
    // 就位/装配异常详情）——排查时误导（timeline 实际存在却报"不存在"）。
    // 保留「未装配场景」字样：网页 doPlay 按此字样附加引导提示（app.js）。
    if (entry == nullptr) {
        err = "时间轴不存在: " + id;
        return false;
    }
    if (entry->scene == nullptr) {
        err = "时间轴未装配场景，无法播放: " + id;
        if (!entry->note.empty()) err += " —— " + entry->note;
        return false;
    }

    // ---- 切换编排四步（见文件头注释）--------------------------------------
    player_.localStop(); // 1. 旧场景状态归位
    player_.applyLocalScene(entry->scene); // 2. 场景槽替换
    registry_.setActive(id);               // 3a. 活跃登记
    // 3b. 出点参数快照（stop_position 0 → 场景编排总时长）
    snapshotParams(entry->loop,
                   entry->stop_position_us > 0
                       ? entry->stop_position_us
                       : entry->scene->timeline.duration_us);
    player_.localPlay(0); // 4. 播放头归零起播

    std::printf("[standalone] 启动时间轴: id=%s name=%s 时长=%.2fs %s\n",
                id.c_str(), entry->name.c_str(),
                static_cast<double>(effectiveStopUs()) / 1e6,
                entry->loop ? "(循环)" : "");
    return true;
}

void NodeController::pause() { player_.localPause(); }

void NodeController::seek(std::int64_t to_us) { player_.localSeek(to_us); }

void NodeController::stop() {
    player_.localStop();
    registry_.clearActive();
    snapshotParams(false, 0);
    std::printf("[standalone] 停止播放\n");
}

bool NodeController::playDefault(const std::string& fallback_id,
                                 std::string& err) {
    // 优先默认时间轴（网页 /{id}/set-default 登记）；回退 CLI/ini 指定
    const std::string id = registry_.defaultId().empty() ? fallback_id
                                                         : registry_.defaultId();
    if (id.empty()) {
        err = "无默认时间轴（config.ini [standalone] default_timeline / "
              "--timeline= / 网页 set-default 任一设置后开机即播）";
        return false;
    }
    if (!play(id, err)) return false;
    // 【fixer-r45 S3-1r5】CLI/ini fallback 链播放成功后补登记运行态默认
    // ——此前只有网页 set-default 会写 defaultId()，重启后经 fallback
    // 自动播放时 defaultId() 仍为空 → 列表 API is_default 全 false（网页
    // "默认"徽标丢失；播放行为本就正确，此为登记闭环）。setDefault 单值
    // 覆盖，且 play() 已校验 id 存在，此处恒成功。
    registry_.setDefault(id);
    return true;
}

// ---------------------------------------------------------------------------
// 主循环节拍（出点行为）
// ---------------------------------------------------------------------------
void NodeController::tick(std::int64_t aligned_now_us) {
    player_.tick(aligned_now_us);

    // ---- 出点行为（仅播中且配置了出点；顺序纪律见头文件注释）--------------
    if (player_.state() != PlayState::Playing) return;
    bool loop = false;
    std::int64_t stop_us = 0;
    {
        std::lock_guard<std::mutex> lock(param_mutex_);
        loop = loop_;
        stop_us = stop_position_us_;
    }
    if (stop_us <= 0) return;
    const std::int64_t t = player_.session_time_us();
    if (t < stop_us) return;

    if (loop) {
        // 循环：回头重播（localPlay(0) 重锚——session_start = now，current
        // 从 0 重新推进；纠偏环在重锚后速率复位，无残余调速）
        player_.localPlay(0);
        std::printf("[standalone] 循环回头: t=%.2fs >= 出点 %.2fs\n",
                    static_cast<double>(t) / 1e6,
                    static_cast<double>(stop_us) / 1e6);
    } else {
        // 停止归零 + 清活跃（一次性——清 stop_position_ 防重复触发）
        player_.localStop();
        registry_.clearActive();
        snapshotParams(false, 0);
        std::printf("[standalone] 播到出点 %.2fs，停止\n",
                    static_cast<double>(stop_us) / 1e6);
    }
}

} // namespace ms::standalone
