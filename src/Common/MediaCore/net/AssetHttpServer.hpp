// ============================================================================
//  AssetHttpServer.hpp —— 素材 HTTP 分发服务（Editor 侧 Distributor 底座）
//
//  设计依据：《media-server-sync-design.md》§6"协议与通道"：
//    * 素材文件【不走 Protobuf】——大视频经内嵌 HTTP server 的 Range 请求
//      传裸字节（分片 + 断点续传 + CRC 校验），避免被打包进内存（§6 表格
//      "素材文件走 HTTP 裸字节"）；
//    * Player 主动拉取（Editor 仅"内嵌 HTTP server，主动推送"指场景元数据
//      推送；文件本体按 §5.5 由 Player 按需拉取——MVP 取拉取模型）。
//
//  HTTP 接口（MVP，够 AssetReceiver 使用即可）：
//      GET  /asset/<asset_id>   下载素材（支持 Range: bytes=a- 断点续传）
//      HEAD /asset/<asset_id>   查询大小/校验和（响应头 X-Asset-Size /
//                               X-Asset-Crc32）
//  响应头约定：
//      Content-Length        文件（剩余）字节数
//      Content-Range         续传时 "bytes a-b/total"（206）
//      X-Asset-Size          文件总大小（字节）
//      X-Asset-Crc32         全文件 CRC32（十六进制，Player 校验用；
//                            后台计算未完成时不发此头）
//
//  安全模型：asset_id 只能命中注册表（registerAsset 显式登记），路径
//  不来自 HTTP 请求本身——天然免疫目录穿越；未注册 id 一律 404。
//
//  线程模型：HTTPServer 内部线程池（params.maxThreads）逐连接派发；
//  注册表用互斥量保护（registerAsset 可与请求并发）。
//
//  【卡死修复 2026-11-03】registerAsset 的全文件 CRC32 此前在调用线程
//  同步计算——Editor 下发补丁（GUI 线程）对大视频文件要读盘数秒，用户
//  实测"添加媒体后下发 patch 就卡死"。现改为：
//    * registerAsset 立即返回（只做存在性检查 + 登记条目，CRC 置
//      pending），CRC 计算转入后台线程；
//    * (路径, 大小, mtime) 缓存：同一文件重复注册零重算（反复 patch /
//      反复 start 都不再付 IO 成本）；
//    * CRC 未就绪期间：assetInfo 返回空 crc、HTTP 响应不带 X-Asset-Crc32
//      ——Player 端 AssetReceiver 对空 checksum 自动降级为"只比长度"
//      （AssetReceiver.hpp 约定），分发链路不受阻。
// ============================================================================

#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerParams.h>

namespace ms {

class AssetHttpServer {
public:
    // -----------------------------------------------------------------------
    // 素材注册表条目
    // -----------------------------------------------------------------------
    struct AssetEntry {
        std::string   file_path;      ///< 素材文件绝对/相对路径（只读服务）
        std::uint64_t size_bytes = 0; ///< 注册时 stat 的大小
        std::string   crc32_hex;      ///< 全文件 CRC32（"1a2b3c4d" 小写 hex；
                                      ///< 空串 = 后台计算中，Player 只比长度）
        bool          crc_pending = false; ///< true = CRC 后台计算中
        /// 注册时采集的文件 mtime（2026-11-04 推模型）：调用方（cmdSyncMedia /
        /// ensureAssetsRegistered）据此检测"同 id 同路径但文件已被外部替换"
        /// ——mtime/size 变化 → 重新注册重算 CRC，实现 WATCHOUT 式修改检测。
        std::filesystem::file_time_type mtime{};
    };

    AssetHttpServer();
    ~AssetHttpServer() { stop(); }
    AssetHttpServer(const AssetHttpServer&) = delete;
    AssetHttpServer& operator=(const AssetHttpServer&) = delete;

    /// 启动监听。port=0 时系统分配（单元测试）。
    /// \param max_threads HTTP 线程池上限（Player 数量个位数，4 足够）
    bool start(std::uint16_t port, int max_threads = 4);

    /// 停止（幂等）：关闭监听、停 CRC 后台线程并等待全部工作线程退出
    void stop();

    /// 实际监听端口（start(0) 后查询；未启动返回 0）
    std::uint16_t port() const;

    /// 注册素材：登记路径并【后台】计算 CRC32 与大小（立即返回，不阻塞
    /// 调用线程——大文件 CRC 数秒级，补丁下发路径在 GUI 线程）。
    /// 幂等缓存：同路径同大小同 mtime 的重复注册直接复用已算 CRC。
    /// 重复注册同 id 直接【覆盖】旧条目（支持补丁"同 asset_id 换文件"
    /// 场景重算 CRC；Player 端按场景下发的 checksum 感知变化）
    /// \return 是否成功（文件不存在/不可读返回 false）
    bool registerAsset(const std::string& asset_id, const std::string& file_path);

    /// 已注册素材数（诊断/测试断言）
    std::size_t asset_count() const;

    /// 查询素材条目（未注册返回 false；CRC 可能为空=后台计算中）
    bool assetInfo(const std::string& asset_id, AssetEntry& out) const;

    // -----------------------------------------------------------------------
    // CRC 缓存持久化 + 就绪回调（2026-11-09，"重复同步媒体"修复链）
    // -----------------------------------------------------------------------
    /// 启用 CRC 缓存持久化：【调用时立即加载】磁盘缓存（同路径+大小+mtime
    /// 命中即复用，editor 重启后零重算——场景/清单立刻带真值 checksum，
    /// 显示端不再因"空 checksum 窗口"重传已就绪文件）；之后每次后台算完
    /// 一个 CRC 即原子重写缓存文件（tmp + rename）。惯例路径
    /// <assets_root>/.crc_cache.tsv；空串 = 禁用（默认）。应在首次
    /// registerAsset 之前调用（加载只并入内存表没有的键，晚调不破坏正确性）。
    /// 格式：每行 `path\tsize\tmtime_ticks\tcrc_hex`（mtime 为
    /// file_time_type 时钟 tick 数，仅同平台同运行时自比——表示法变化时
    /// 缓存失配自动重算，安全退化）。
    void enableCacheFile(const std::string& path);
    /// 新 CRC 后台就绪回调（每算完一个非空 CRC 触发一次；【CRC 后台线程
    /// 上下文】订阅方自行防抖/线程安全。缓存命中路径不触发——只有新算的
    /// 才是"状态变化"。空 = 不回调。应在 registerAsset 之前设置，运行期
    /// 只读）。Editor 端订阅它做"CRC 就绪自动补推清单"（防抖见
    /// EditorRuntime::crcPushLoop）。
    std::function<void()> on_crc_ready;

private:
    /// 单条目索引（锁内返回拷贝，避免回调/请求侧持锁做 IO）
    struct Entry {
        std::string   path;
        std::uint64_t size = 0;
        std::string   crc_hex;
        bool          crc_pending = false; ///< CRC 后台计算中
        std::filesystem::file_time_type mtime{}; ///< 注册时的文件 mtime
    };
    /// CRC 缓存条目：文件身份（路径+大小+mtime）→ 已算校验和
    struct CrcCache {
        std::uint64_t                   size = 0;
        std::filesystem::file_time_type mtime{};
        std::string                     hex;
    };
    /// 后台 CRC 任务（asset_id 仅用于回填定位；path 为计算对象）
    struct CrcTask {
        std::string                     asset_id;
        std::string                     path;
        std::uint64_t                   size = 0;
        std::filesystem::file_time_type mtime{};
    };

    /// CRC 后台线程主循环：取任务 → 锁外分块算 CRC → 回填条目+缓存
    void crcWorkerLoop();

    /// 缓存文件读写（enableCacheFile/crcWorkerLoop 专用；见公开注释）
    static void saveCacheFile(const std::string& path,
                              const std::map<std::string, CrcCache>& cache);
    static std::map<std::string, CrcCache>
    loadCacheFile(const std::string& path);

    mutable std::mutex mutex_;
    std::map<std::string, Entry> assets_;   ///< asset_id → 文件信息
    std::map<std::string, CrcCache> crc_cache_; ///< 路径 → 文件身份→CRC
    /// CRC 缓存持久化路径（enableCacheFile 设置；空 = 禁用）。
    /// 写于 CRC 后台线程（saveCacheFile），读于任意注册线程——两者都在
    /// mutex_ 临界区内或只读，无竞争。
    std::string cache_path_;

    // ---- 后台 CRC 线程（构造启动，析构/stop 停止）----
    std::mutex              crc_mutex_;
    std::condition_variable crc_cv_;
    bool                    crc_quit_ = false;
    std::deque<CrcTask>     crc_queue_;
    std::thread             crc_worker_;

    std::unique_ptr<Poco::Net::HTTPServer> server_;
    std::uint16_t port_ = 0;

    // HandlerFactory / RequestHandler 需要访问注册表——声明为友元
    // （注意：C++ 友元【不传递】给嵌套类，两个类都须显式声明）
    friend class AssetHandlerFactory;
    friend class AssetHandler;
};

} // namespace ms
