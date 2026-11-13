// ============================================================================
//  MediaFactory.hpp —— 媒体源工厂 + IMediaSource 接口（§4.7 扩展点核心）
//
//  设计依据：《media-server-sync-design.md》§4.7 / §11
//    * 新增媒体类型 = 1 个 Protobuf 变体 + 1 个 IMediaSource 实现 + 1 行注册；
//      引擎 Core（TimelineEngine/ScreenMapper/Renderer）对具体媒体类型零感知，
//      全部通过本接口的多态产出交互（数据驱动）；
//    * MediaScheduler（Phase 1 后续任务）维护实例生命周期：按 asset_id 创建
//      并缓存 IMediaSource，逐帧调用其 produce(media_us) 获取纹理/采样。
//
//  测试策略：
//    MediaFactory 设计为【可实例化类】（非单例 hard-coded 状态），
//    单元测试可构造独立实例验证注册/查找/冲突语义；
//    全局默认实例 instance() 供运行期各处访问（注册发生在启动装配阶段）。
// ============================================================================

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "model/SceneModel.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// FrameView —— 媒体帧的通用视图（RGBA 行主序像素 / GPU 原生帧句柄）
//
// 设计文档 §5.6 解耦要点：Renderer/ScreenMapper 对媒体类型无感知，只消费
// 帧纹理。本结构是 Core 内统一的"帧纹理视图"（零拷贝：指向源实例内部的
// 像素缓冲，仅在同帧 produce→draw 期间有效）。
//
// 【解码线程化（2026-11-03）】解码移入驱动线程后，帧数据可能由解码线程
// 发布、渲染线程消费——keep 持有帧缓冲的共享所有权快照：渲染线程在
// draw 期间解码线程继续发布新帧也不悬垂（引用计数保活），零拷贝语义
// 保留（rgba 直接指向快照缓冲，无逐像素拷贝）。
//
// 【GPU 原生帧域（第③阶段 0 拷贝）】硬解帧不下载回内存时，帧以"原生
// 句柄"交付：domain = GpuCudaNv12，rgba 为空，native 持有解码帧引用
// （类型擦除的 shared_ptr——实际为 AVFrame 引用持有者，解码端构造；
// GL 后端经 CUDA-GL interop 消费）。CPU 域（CpuRgba）语义与旧版一致。
// ---------------------------------------------------------------------------
enum class FrameDomain : std::uint8_t {
    CpuRgba    = 0, ///< 系统内存 RGBA（行主序；既有语义）
    GpuCudaNv12 = 1, ///< 显存 NV12（CUDA 帧引用；GL 后端 interop 消费）
};

struct FrameView {
    int             width  = 0;   ///< 像素宽
    int             height = 0;   ///< 像素高
    std::uint32_t   stride = 0;   ///< 行字节数（= width * 4，无填充）
    const std::uint8_t* rgba = nullptr; ///< RGBA 数据指针（CPU 域；可空）
    /// 帧缓冲保活（类型擦除的共享快照）：渲染线程 draw 期间保持像素缓冲
    /// 存活；单线程同步路径为空（源实例自身存活已足够）
    std::shared_ptr<const void> keep;
    /// 帧域（CPU RGBA / GPU 原生）
    FrameDomain domain = FrameDomain::CpuRgba;
    /// GPU 原生帧句柄（GpuCudaNv12 域有效；shared_ptr 保活到绘制结束——
    /// 渲染线程 draw 完成后解码端缓冲可复用）
    std::shared_ptr<const void> native;

    /// 是否持有有效载荷（CPU 像素 或 GPU 原生帧二选一）
    bool valid() const noexcept {
        if (domain == FrameDomain::CpuRgba)
            return rgba != nullptr && width > 0 && height > 0;
        return native != nullptr && width > 0 && height > 0;
    }
};

// ---------------------------------------------------------------------------
// IMediaSource —— 媒体源统一接口（所有媒体类型的抽象）
//
// 生命周期约定：
//   * 一个实例对应一个素材（Asset）在某个 Player 上的播放上下文；
//   * prepare() 在播放前调用（打开文件/建立解码上下文/预热），失败可重试；
//   * produce(media_us) 逐帧调用：给定素材内部时间，返回该时刻的呈现载荷
//     （Phase 1 中由 FfmpegMediaSource 返回解码帧；此处接口先行，实现后续补）；
//   * release() 释放底层资源（解码器/文件句柄/显存）。
//
// 注意：接口方法均带媒体类型无关语义——上层不判断"这是视频还是文本"，
// 只按时间取"当前帧"，这正是工厂模式带来的 Core 零改动扩展性。
// ---------------------------------------------------------------------------
class IMediaSource {
public:
    virtual ~IMediaSource() = default;

    /// 本源对应的媒体类型（工厂注册键；测试用于类型断言）
    virtual AssetKind kind() const noexcept = 0;

    /// 播放前准备：打开素材、建立解码/渲染上下文
    /// \return false 表示准备失败（调用方可重试或上报错误）
    virtual bool prepare(const Asset& asset) = 0;

    /// 取指定素材内部时间（us）的呈现载荷
    /// \return false 表示该时刻无有效载荷（如尚未解码出帧）
    virtual bool produce(std::int64_t media_us) = 0;

    /// 取当前帧的通用视图（produce 成功后调用；渲染管线消费）
    /// \return false = 本源无像素帧（如音频/未就绪）；视图仅本帧有效
    virtual bool frameView(FrameView& out) const {
        (void)out;
        return false; // 默认无像素帧（音频源等；视频/图片源覆写）
    }

    /// 释放全部底层资源（可随后重新 prepare 复用实例）
    virtual void release() = 0;
};

// ---------------------------------------------------------------------------
// MediaFactory —— 注册表：AssetKind → 构造器
//
// 线程安全：注册（写）与创建（读）均持锁。注册发生在启动装配阶段（低频），
// 创建发生在 Player 收到 Scene 时（低频）；逐帧 produce 不经过本类，无热点。
// ---------------------------------------------------------------------------
class MediaFactory {
public:
    // 默认构造公开：允许构造隔离实例（单元测试/多装配域）。
    // 禁止拷贝（注册表含互斥量），允许移动语义由编译器默认规则处理。
    MediaFactory() = default;
    MediaFactory(const MediaFactory&) = delete;
    MediaFactory& operator=(const MediaFactory&) = delete;

    /// 构造器签名：输入素材元数据，产出对应的媒体源实例
    using Creator = std::function<std::unique_ptr<IMediaSource>(const Asset&)>;

    /// 注册某类型的构造器
    /// \return false 表示该类型已注册过（重复注册被拒绝——避免装配期
    ///         静默覆盖导致难以排查的行为差异；需要替换时请显式 unregister）
    bool registerCreator(AssetKind kind, Creator creator) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto key = static_cast<std::uint32_t>(kind);
        if (creators_.count(key) != 0) return false; // 已存在 → 拒绝
        creators_.emplace(key, std::move(creator));
        return true;
    }

    /// 注销（供测试与运行期重装配使用）
    bool unregisterCreator(AssetKind kind) {
        std::lock_guard<std::mutex> lock(mutex_);
        return creators_.erase(static_cast<std::uint32_t>(kind)) > 0;
    }

    /// 按素材种类创建媒体源实例
    /// \return nullptr 表示该类型未注册（含 Unknown）——调用方（MediaScheduler）
    ///         应记日志并跳过该素材，不得让整个播放失败（单素材故障隔离）
    std::unique_ptr<IMediaSource> create(const Asset& asset) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = creators_.find(static_cast<std::uint32_t>(asset.kind));
        if (it == creators_.end()) return nullptr;
        return it->second(asset); // 委托构造器创建实例
    }

    /// 查询某类型是否已注册（诊断/测试用）
    bool hasCreator(AssetKind kind) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return creators_.count(static_cast<std::uint32_t>(kind)) != 0;
    }

    /// 全局默认工厂（运行期装配入口；测试请构造局部实例以隔离状态）
    static MediaFactory& instance() {
        static MediaFactory factory;
        return factory;
    }

private:
    std::unordered_map<std::uint32_t, Creator> creators_; ///< kind → 构造器
    mutable std::mutex mutex_; ///< 保护注册表的并发访问
};

} // namespace ms
