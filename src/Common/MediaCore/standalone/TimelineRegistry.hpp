// ============================================================================
//  TimelineRegistry.hpp —— standalone 多时间轴注册表（播放列表）
//
//  定位（display×MMC 合并方案 v2.2，2026-11-11）：
//    display 的 standalone 模式 = 本机简易时间轴控制器。用户（经 M2 网页
//    或 M5 工程文件）编排多个时间轴，每个时间轴是一份装配好的 Scene
//    （= 一个"播放列表"条目）；同一时刻至多一个活跃——"启动 B = 停 A →
//    换场景槽 → 播放头归零"，场景替换一帧内完成，渲染引擎零感知
//    （NodeController 负责这套切换编排，本表只管登记与仲裁）。
//
//  职责边界（单一）：
//    * 条目 CRUD（装配好的 Scene 以 shared_ptr 持有在此；装配在
//      SceneAssembler，播放切换在 NodeController）
//    * 唯一活跃仲裁（时间轴级"谁的内容进场景槽"——控制面级仲裁
//      在 NodeController：Editor 在场全让位，v2.2 两级仲裁同语义）
//    * 默认时间轴登记（开机自动播放的恢复目标；v2.2 用户定案
//      "可以选中某一个时间轴，然后开机时自动进入播放"）
//  不负责：播放控制（NodeController）、HTTP API（M2）、持久化（M5——
//  现为内存态，进程重启后经 config.ini [standalone] default_timeline
//  指向的工程文件重建）。
//
//  线程模型：一把 mutex 串行化——M2 起 HTTP 工作线程并发调 CRUD，
//  NodeController 主循环读活跃条目。对外返回值快照 / shared_ptr 免拷贝，
//  不泄漏内部引用；shared_ptr<const TimelineEntry> 保证调用方持有期间
//  条目数据不被并发 remove/update 破坏（读方最长持有一个 tick 粒度）。
// ============================================================================

#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "model/SceneModel.hpp"

namespace ms::standalone {

// ---------------------------------------------------------------------------
// 时间轴条目 —— 播放列表的一项
// ---------------------------------------------------------------------------
struct TimelineEntry {
    std::string id;    ///< 稳定标识（URL 路径 / default_timeline 登记键；
                       ///< 空 id 由 add() 生成 "tl-N" 递增）
    std::string name;  ///< 人类可读名（网页列表显示）
    /// 装配好的场景（SceneAssembler 产出；渲染引擎消费的同一 SceneModel）
    std::shared_ptr<const Scene> scene;
    bool loop = false; ///< 播到出点后回头重播（loop 循环语义）
    /// 出点（us）：播到此位置触发 loop 回头或停止。
    /// 0 = 用 scene->timeline.duration_us（片段编排自动推算的总时长）
    std::int64_t stop_position_us = 0;
    /// 装配状态说明（round3 S2）：非空 = 条目当前不可直接播放（scene 为
    /// 空——装配失败/素材未就位），内容为失败原因首行。列表接口原样
    /// 下发网页展示（保留可见性：装配失败的时间轴重启后不再被静默
    /// 丢弃，修好素材或改编排保存即可恢复）。空 = 正常可播。
    std::string note;
};

// ---------------------------------------------------------------------------
// 注册表
// ---------------------------------------------------------------------------
class TimelineRegistry {
public:
    TimelineRegistry() = default;
    TimelineRegistry(const TimelineRegistry&) = delete;
    TimelineRegistry& operator=(const TimelineRegistry&) = delete;

    // ---- CRUD --------------------------------------------------------------

    /// 新增条目。\param entry.id 为空时生成 "tl-N" 递增 id；非空时要求
    /// 不与既有 id 冲突。
    /// \return 生效 id；空串 = id 冲突（调用方决定改名/报错）
    std::string add(TimelineEntry entry);

    /// 按 id 整体替换条目（场景/名称/loop/出点任一变更都走这里——
    /// 时间轴内容以"不可变快照"为纪律，不做局部字段写）。
    /// \return false = id 不存在
    bool update(const TimelineEntry& entry);

    /// 删除条目。活跃/默认登记命中被删 id 时自动复位（注册表自洽，
    /// NodeController 无需先查再删）。
    /// \return false = id 不存在
    bool remove(const std::string& id);

    /// 全量快照（保创建序；值拷贝——列表展示用，量级个位数~几十）
    std::vector<TimelineEntry> list() const;

    /// 按 id 查找（不存在返回空指针；shared_ptr 免拷贝且防悬垂）
    std::shared_ptr<const TimelineEntry> find(const std::string& id) const;

    std::size_t size() const;

    // ---- 唯一活跃仲裁（"谁的内容进场景槽"·时间轴级）--------------------------

    /// 设活跃时间轴。\return false = id 不存在（活跃登记不变）。
    /// 不校验"是否已切换"——同 id 重复 set 幂等，NodeController 据此
    /// 决定是否真正执行场景替换。
    bool setActive(const std::string& id);

    /// 清除活跃（停播时调用；无活跃 = 场景槽保持现状但不再续播）
    void clearActive();

    /// 当前活跃条目（无活跃返回空指针）
    std::shared_ptr<const TimelineEntry> active() const;

    /// 当前活跃 id（空 = 无活跃；HTTP status 报文用）
    std::string activeId() const;

    // ---- 默认时间轴（开机自动播放的恢复目标）--------------------------------

    /// 登记默认时间轴（网页 /{id}/set-default；持久化到 config.ini 在 M5）。
    /// \return false = id 不存在
    bool setDefault(const std::string& id);

    /// 清除默认登记
    void clearDefault();

    /// 默认时间轴 id（空 = 未设置；开机/Editor 断开时 NodeController
    /// 据此恢复自动播放）
    std::string defaultId() const;

    /// 默认时间轴条目（未设置或条目已被删返回空指针）
    std::shared_ptr<const TimelineEntry> defaultEntry() const;

private:
    /// 锁内线性查找（条目量级小；调用方持锁）
    std::shared_ptr<const TimelineEntry> findByIdLocked(
        const std::string& id) const;

    mutable std::mutex mutex_;
    /// 全部条目（创建序；shared_ptr 保证外部持有的快照不被删除悬空）
    std::vector<std::shared_ptr<const TimelineEntry>> entries_;
    std::string active_id_;   ///< 活跃条目 id（空 = 无活跃）
    std::string default_id_;  ///< 默认条目 id（空 = 未设置）
    int next_seq_ = 0;        ///< 自动 id 生成序号（单调递增，允许有洞）
};

} // namespace ms::standalone
