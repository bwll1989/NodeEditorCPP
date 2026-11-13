// ============================================================================
//  HwAccel.hpp —— 视频硬解后端配置（跨平台；FFmpeg hwaccel 抽象）
//
//  设计依据：《hw-decode-implementation-plan.md》第①阶段
//    * 解码层统一的"硬解优先、软解兜底"策略：本结构只描述【意图】，
//      实际是否启用由 FfmpegMediaSource::prepare 运行时探测决定——
//      探测/创建设备/打开解码器任一步失败即静默回退软解（与既有
//      行为逐位一致），保证无独显/旧驱动/远程会话机器照常运行；
//    * 后端映射（FFmpeg 语义）：
//        D3D11VA    → AV_PIX_FMT_D3D11   （Windows 全厂商；默认推荐）
//        Cuda       → AV_PIX_FMT_CUDA    （NVIDIA NVDEC；唯一可与
//                     OpenGL 直接互操作的后端——cudaGraphicsGLRegisterImage，
//                     第③阶段 0 拷贝依赖它）
//        Vaapi      → AV_PIX_FMT_VAAPI   （Linux Intel/AMD）
//        MediaCodec → AV_PIX_FMT_MEDIACODEC（Android）
//        Auto       → 按平台探测链逐个尝试（Windows: D3D11VA→CUDA）
//        Soft       → 强制软解（诊断/对比基准）
//    * 【默认值分层】库层默认 Soft（现有测试/Editor 预览行为完全不变）；
//      display 端装配层从 config.ini 读，产品默认 D3D11VA（配置与库
//      默认分离——测试无需感知硬解环境差异）。
//
//  零依赖：纯值类型，无 FFmpeg 头（类型/枚举仅 .cpp 层使用）。
// ============================================================================

#pragma once

#include <atomic>
#include <memory>
#include <string>

namespace ms {

// ---------------------------------------------------------------------------
// 硬解后端类型（配置文件/命令行的取值集合）
// ---------------------------------------------------------------------------
enum class HwAccelType {
    Soft       = 0, ///< 强制软解（诊断基准；库层默认）
    Auto       = 1, ///< 平台探测链（Windows: D3D11VA→CUDA→Soft）
    D3D11VA    = 2, ///< Windows D3D11VA（全厂商；产品默认推荐）
    Cuda       = 3, ///< NVIDIA NVDEC/CUDA（可 GL 0 拷贝互操作）
    Vaapi      = 4, ///< Linux VAAPI（Intel/AMD）
    MediaCodec = 5, ///< Android MediaCodec
};

// ---------------------------------------------------------------------------
// HwAccelConfig —— 硬解配置值对象（随装配链下传到每个媒体源实例）
// ---------------------------------------------------------------------------
struct HwAccelConfig {
    HwAccelType type = HwAccelType::Soft; ///< 后端意图（运行时探测后生效）

    /// 第③阶段 0 拷贝交付开关：true = 解码线程直接发布 GPU 原生帧句柄
    /// （AVFrame ref，帧留在显存），由 GL 后端经 CUDA-GL interop 消费；
    /// false = transfer 回系统内存（NV12→sws→RGBA，既有管线）。
    /// 仅 "GL 渲染后端 + Cuda 硬解" 组合有意义；装配层负责保持一致性。
    bool gpu_delivery = false;

    /// 【运行时协商旗标】GPU 帧交付的自适应回退开关（③阶段三档回退链的
    /// 关键件）——解码端逐帧检查，渲染端初始化后回写：
    ///   * nullptr（默认）：禁用 GPU 帧交付（始终走下载路径——既有行为，
    ///     测试/Editor 预览零影响）；
    ///   * 非 null：*旗标 == true 时发布 GpuCudaNv12 原生帧；GL 后端
    ///     interop 探测失败（无 N 卡/工具链缺失/注册失败）时置 false →
    ///     解码端下一帧起自动回退 av_hwframe_transfer_data 下载路径，
    ///     全程无黑屏（首帧竞态窗口 < 1s，渲染端占位跳过自愈）。
    /// shared_ptr 携带：装配层创建一次，解码源与渲染后端各持一份副本。
    std::shared_ptr<std::atomic_bool> gpu_delivery_ok;

    /// 解析配置字符串（大小写不敏感；未知值回落 Soft 并由调用方提示）
    /// 支持：soft / auto / d3d11va / cuda|nvdec / vaapi / mediacodec
    static HwAccelType parseType(const std::string& name);

    /// 类型名（日志/诊断用；与 parseType 可逆的规范拼写）
    static const char* typeName(HwAccelType t);
};

} // namespace ms
