// ============================================================================
//  ClockProvider.hpp —— 对齐时钟读取（全场共享 NTP 对齐时钟的消费端，§4.4）
//
//  设计依据：《media-server-sync-design.md》§4.4 / §5.4 / §11
//    * 所有节点（含 Editor）从"已 NTP 校准的系统时钟"读取 aligned_now，
//      本地计算播放头：session_time = aligned_now - session_start（§4.4）；
//    * 时钟【对齐本身交给系统 NTP 客户端】（chrony/ntpd，零自研，§11），
//      本组件只负责两件事：
//        (1) 复合时钟读法：用 monotonic 时钟推进 + NTP offset 修正——
//            即使系统墙钟被 NTP step 或用户手动改动，读取值也平滑连续；
//            （这对应设计评审补丁："墙上时钟跳变风险 → monotonic + offset"）
//        (2) offset 注入点：Phase 2 中由外部进程（SNTP 探测/系统 NTP 状态）
//            或 SyncClient（软重锚）设置 offsetUs()，业务其余部分零改动。
//
//  复合时钟公式：
//      aligned_now(t) = anchor_wall + (mono_now - anchor_mono) + offset
//    其中 anchor 为构造时记录的 (墙钟, 单调钟) 锚点对；
//    offset 为 NTP 修正量（wall 与"真准时钟"的偏差），运行期可更新。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>

namespace ms {

class ClockProvider {
public:
    /// 构造时记录 (墙钟, 单调钟) 锚点对——之后所有读取都从锚点经单调钟推进，
    /// 墙钟后续的 step 跳变不再影响读取值（见 .cpp 实现）
    ClockProvider() noexcept;

    // ---- NTP offset 管理 ---------------------------------------------------

    /// 设置 NTP 修正量（us）：aligned = 系统墙钟 + offset。
    /// 正值表示本地墙钟比 NTP 基准慢（需要加）。线程安全（原子写）。
    void setOffsetUs(std::int64_t offset_us) noexcept;

    /// 当前生效的修正量（us）——诊断/日志用
    std::int64_t offsetUs() const noexcept;

    // ---- 时钟读取 ----------------------------------------------------------

    /// 对齐时钟当前值（us，Unix 纪元微秒）：
    /// 等价于"NTP 校准后的墙钟"，但由单调钟平滑推进，抗 step 跳变。
    std::int64_t alignedNowUs() const noexcept;

    /// 原始系统墙钟（us）——仅诊断对比用，播放头计算禁止直接使用
    std::int64_t wallNowUs() const noexcept;

    // ---- 播放头公式（全场统一，§4.4）---------------------------------------

    /// session_time = aligned_now - session_start（全场同式，天然一致）
    /// 独立成静态函数：作为唯一公式入口，防各处手写产生单位/符号偏差
    static std::int64_t sessionTimeUs(std::int64_t aligned_now_us,
                                      std::int64_t session_start_us) noexcept {
        return aligned_now_us - session_start_us;
    }

private:
    std::int64_t wallNowRawUs() const noexcept;  ///< 原始系统墙钟（us）
    std::int64_t monoNowRawUs() const noexcept;  ///< 原始单调钟（us，steady）

    std::int64_t anchor_wall_us_;      ///< 构造时的墙钟锚点
    std::int64_t anchor_mono_us_;      ///< 构造时的单调钟锚点
    std::atomic<std::int64_t> offset_us_; ///< NTP 修正量（us），原子读写
};

} // namespace ms
