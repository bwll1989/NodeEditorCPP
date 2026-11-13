// ============================================================================
//  AssetReceiver.hpp —— 素材拉取客户端（Player 侧，§5.5）
//
//  设计依据：《media-server-sync-design.md》§5.5 / §6：
//    * 素材文件走 HTTP 裸字节（不走 Protobuf）；Range 断点续传 + CRC 校验；
//    * 大视频流式落盘（64KB 分块），内存占用恒定；
//    * 断点续传：目标文件已存在部分字节 → 发 "Range: bytes=<已有>-"
//      请求剩余部分（206 续传）；服务端不支持/越界回 200 全量 → 覆盖重传；
//    * 幂等：文件已完整且 CRC 匹配 → 不发起网络请求（重启/重试零成本）。
//
//  校验和约定（与 Asset.checksum 字段一致）："<算法>:<hex>"，MVP 仅
//  "crc32:xxxxxxxx"（Poco::Checksum 与 Editor 端注册时同算法）。
// ============================================================================

#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

#include <Poco/Checksum.h>
#include <Poco/Net/HTTPClientSession.h>

namespace ms {

class AssetReceiver {
public:
    // -----------------------------------------------------------------------
    // 拉取结果
    // -----------------------------------------------------------------------
    enum class Result {
        Ok           = 0,  ///< 下载成功且 CRC 匹配（含幂等命中）
        AlreadyHave  = 1,  ///< 本地已完整且 CRC 匹配（零网络）
        CrcMismatch  = 2,  ///< 下载完成但校验不符（文件已删除待重试）
        NetworkError = 3,  ///< 连接/请求/响应失败（半成品保留供续传）
        NotFound     = 4,  ///< 服务端 404（素材未注册/ID 错）
        BadResponse  = 5,  ///< 响应协议异常（状态码/长度不符）
        Cancelled    = 6,  ///< 调用方取消（round7 S1-1；半成品保留供续传）
    };

    struct Stats {
        std::uint64_t bytes_written = 0;   ///< 本次实际传输字节（诊断）
        bool resumed = false;              ///< 是否发生断点续传
        bool idempotent = false;           ///< 是否幂等命中（零传输）
    };

    /// 分块进度回调（拉取线程上下文同步调用；64KB 每块一次——UI 端
    /// 自行快照节流）。\p bytes_total 为服务端报告的全量字节数（0 = 未知，
    /// UI 显示不定进度）。断点续传时 bytes_now 含续传前已有字节。
    using ChunkFn = std::function<void(std::uint64_t bytes_now,
                                       std::uint64_t bytes_total)>;

    /// 取消判定（拉取线程内高频轮询；返回 true = 尽快放弃本次拉取）。
    /// round7 S1-1 修复配套：recv 线程接管新清单前要 join 旧拉取线程，
    /// 旧线程若无取消通道，join 要等整个 GB 级下载结束（实测把 recv
    /// 线程冻结数十秒，期间下发的 SEEK/PAUSE/快照全部排队迟到）。
    using CancelFn = std::function<bool()>;

    AssetReceiver() = default;
    ~AssetReceiver() = default;

    /// 拉取素材到本地（阻塞至完成/失败/取消）。
    /// \param host/port    Editor 的 HTTP 分发地址
    /// \param asset_id     素材 ID（对应服务端注册表键）
    /// \param save_path    本地落盘路径（半成品允许存在——续传续写）
    /// \param checksum     期望校验和 "crc32:xxxxxxxx"（空串 = 只比长度）
    /// \param stats        可选：传输统计（诊断）
    /// \param chunk        可选：分块进度回调（见 ChunkFn；空 = 不上报）
    /// \param cancelled    可选：取消判定（每 64KB 分块后轮询；空 = 不可取消）
    /// \return Cancelled 或 NetworkError（半成品保留供续传）
    /// \note 连接超时 10s / 收发超时 30s（慢环境宽松；真实 LAN 秒级完成）
    Result fetch(const std::string& host, std::uint16_t port,
                 const std::string& asset_id, const std::string& save_path,
                 const std::string& checksum, Stats* stats = nullptr,
                 const ChunkFn& chunk = ChunkFn(),
                 const CancelFn& cancelled = CancelFn());

private:
    /// 计算文件 CRC32（流式分块）——与 Editor 端同算法同口径。
    /// 【path 版】窄 string 版在 MSVC 下按 ACP 解码，中文名静默 open
    /// 失败（CRC 恒空）甚至抛异常——调用方必须传 u8Path() 产物。
    static std::string fileCrc32Hex(const std::filesystem::path& path,
                                    std::uint64_t& out_size);
};

} // namespace ms
