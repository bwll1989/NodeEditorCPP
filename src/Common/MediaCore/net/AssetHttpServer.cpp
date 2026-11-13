// ============================================================================
//  AssetHttpServer.cpp —— 素材 HTTP 分发实现（POCO HTTPServer）
//
//  请求处理流程（GET /asset/<id>）：
//      1. 解析 asset_id → 查注册表（未命中 → 404）
//      2. 读 Range 头：
//         - 无 Range → 200 全量（Content-Length = 文件大小）
//         - "bytes=a-" 或 "bytes=a-b" → 206 Partial（Content-Range 响应头）
//         - 非法/越界 → 416
//      3. 流式发送：64KB 分块读文件写响应流——内存占用恒定
//         （大视频不整读进内存，设计 §5.5"分片 + 断点续传"）
//
//  CRC32：Poco::Checksum(TYPE_CRC32) 流式 update——与发送同样的分块
//  读法。【卡死修复 2026-11-03】CRC 从注册线程（GUI）移入后台线程计算：
//  registerAsset 立即返回，CRC 就绪前 Player 端按"空 checksum 只比长度"
//  降级；(路径,大小,mtime) 缓存使重复注册零重算。
// ============================================================================

#include "net/AssetHttpServer.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <vector>

#include <Poco/Checksum.h>
#include <Poco/Exception.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // MultiByteToWideChar（u8path 中文素材路径转换）
#endif

namespace ms {

using Poco::Net::HTTPResponse;

namespace {

// ---------------------------------------------------------------------------
// 分块大小：64KB（发送与 CRC 共用；POCO 网络栈下足够高效且内存恒定）
// ---------------------------------------------------------------------------
constexpr std::size_t kChunkSize = 64 * 1024;

/// UTF-8 窄串 → 宽字符 path（2026-11-12 中文素材实测修复）。
/// protobuf string 恒为 UTF-8；MSVC std::filesystem 窄字符构造按 ACP
/// （中文系统 GBK）隐式转换，中文文件名逐字节误解 → registerAsset 的
/// file_size/last_write_time 与 CRC/下载 ifstream 全部打不开。
/// 统一以宽字符 path 打开（Windows 端 UTF-8 → UTF-16 显式转换）。
std::filesystem::path u8path(const std::string& s) {
#ifdef _WIN32
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(),
                                      static_cast<int>(s.size()),
                                      nullptr, 0);
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                        w.data(), n);
    return std::filesystem::path(std::move(w));
#else
    return std::filesystem::path(s); // POSIX 窄字符即 UTF-8
#endif
}

/// 解析 Range 头 "bytes=a-" / "bytes=a-b" → 起始偏移。
/// \param range_header Range 请求头原文（空串 = 无 Range）
/// \param file_size    文件总大小
/// \param out_offset   解析出的起始偏移
/// \return 0=无 Range（全量）；1=有效 Range；-1=非法/越界（应回 416）
int parseRangeOffset(const std::string& range_header, std::uint64_t file_size,
                     std::uint64_t& out_offset) {
    if (range_header.empty()) return 0;
    // MVP 只支持单区间 "bytes=a-" / "bytes=a-b"（AssetReceiver 只发这种）
    static const std::string kPrefix = "bytes=";
    if (range_header.rfind(kPrefix, 0) != 0) return -1;
    const std::string spec = range_header.substr(kPrefix.size());
    const auto dash = spec.find('-');
    if (dash == std::string::npos) return -1;
    if (dash == 0) return -1; // 后缀形式 "bytes=-500"：MVP 不支持 → 416
    const std::string a = spec.substr(0, dash);
    if (a.empty()) return -1;
    unsigned long long start = 0;
    try {
        start = std::stoull(a);
    } catch (const std::exception&) {
        return -1;
    }
    if (start >= file_size) return -1; // 越界：文件已完整或错误
    out_offset = start;
    return 1;
}

} // 匿名命名空间

// ---------------------------------------------------------------------------
// 生命周期：CRC 后台线程随对象构造启动（与 HTTP 监听无关——测试可只注册
// 不启动监听）；析构/stop 统一停止。
// ---------------------------------------------------------------------------
AssetHttpServer::AssetHttpServer() {
    crc_worker_ = std::thread([this] { crcWorkerLoop(); });
}

void AssetHttpServer::stop() {
    // ---- 1. 先停 CRC 后台线程（幂等；队列中未算完的 CRC 直接丢弃）----
    {
        std::lock_guard<std::mutex> lock(mutex_);
        crc_quit_ = true;
    }
    crc_cv_.notify_all();
    if (crc_worker_.joinable()) crc_worker_.join();

    // ---- 2. 停 HTTP 监听（未启动过则跳过）----
    if (!server_) return;
    server_->stop(); // 内部 join 工作线程
    server_.reset(); // 析构 ServerSocket
    port_ = 0;
}

std::uint16_t AssetHttpServer::port() const {
    return server_ ? server_->port() : 0;
}

// ---------------------------------------------------------------------------
// registerAsset —— 登记素材（立即返回；CRC 后台计算）
//
// 【卡死修复】此前在本调用线程同步做全文件 CRC32（大视频数秒级读盘），
// 而"同步媒体"（GUI 线程，原 cmdPatch 时代）对每个新增素材都会走到这里
// ——用户实测"添加媒体后下发就卡死"。现流程：
//   1. 文件存在性检查 + 大小/mtime 采集（快）；
//   2. 缓存命中（同路径+大小+mtime）→ 直接复用已算 CRC，零重算；
//   3. 否则登记条目（crc 空 + pending）并入队后台任务，立即返回。
// CRC 未就绪期间 assetInfo 返回空 crc → 场景广播的 checksum 为空 →
// Player 端 AssetReceiver 对空 checksum 自动降级"只比长度"（约定见
// AssetReceiver.hpp），分发不受阻。
// ---------------------------------------------------------------------------
bool AssetHttpServer::registerAsset(const std::string& asset_id,
                                    const std::string& file_path) {
    // ---- 1. 文件校验（存在 + 可读 + 大小）----
    // 【u8path 2026-11-12】窄串直传按 ACP 转换，中文文件名必失败
    std::error_code ec;
    const auto fsp = u8path(file_path);
    const auto size = std::filesystem::file_size(fsp, ec);
    if (ec) return false; // 不存在/不可读（与旧 ifstream 行为一致）
    const auto mtime = std::filesystem::last_write_time(fsp, ec);
    if (ec) return false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        // ---- 2. 缓存命中：同一文件（路径+大小+mtime 均同）复用 CRC ----
        // 反复下发补丁/重启会话对未变更素材零 IO 成本。
        const auto cit = crc_cache_.find(file_path);
        if (cit != crc_cache_.end() && cit->second.size == size &&
            cit->second.mtime == mtime) {
            assets_[asset_id] = Entry{file_path, size, cit->second.hex, false, mtime};
            return true;
        }
        // ---- 3. 登记待算条目并入队（覆盖语义：同 id 换文件照常重算）----
        assets_[asset_id] = Entry{file_path, size, "", true, mtime};
        crc_queue_.push_back(CrcTask{asset_id, file_path, size, mtime});
    }
    crc_cv_.notify_all();
    return true;
}

// ---------------------------------------------------------------------------
// crcWorkerLoop —— 后台 CRC 计算线程：取任务 → 锁外分块读文件 → 回填
// ---------------------------------------------------------------------------
void AssetHttpServer::crcWorkerLoop() {
    for (;;) {
        CrcTask task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            // 有任务/退出即醒；定时醒一次兜底信号丢失
            crc_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
                return crc_quit_ || !crc_queue_.empty();
            });
            if (crc_quit_) return;
            if (crc_queue_.empty()) continue;
            task = crc_queue_.front();
            crc_queue_.pop_front();
        }

        // ---- 锁外计算：全文件 CRC32（流式分块，内存恒定）----
        std::ifstream f(u8path(task.path), std::ios::binary);
        std::string hex;
        if (!f) {
            // 【2026-11-13 修正】打开失败（注册与计算间被删/被独占）留痕——
            // 原先静默丢弃导致该条目 crc_pending 永久滞留且排障无据
            std::fprintf(stderr, "[asset] CRC 打开失败（跳过本任务）: %s\n",
                         task.path.c_str());
        } else {
            f.seekg(0, std::ios::end);
            // 以当前磁盘实况为准（注册与计算间文件可能被替换）；
            // 【2026-11-13 修正】tellg 失败(-1) 不回填——垃圾值会污染
            // assetInfo 尺寸并导致 Player 长度幂等比对永不命中
            const auto raw_sz = f.tellg();
            if (raw_sz < 0) {
                std::fprintf(stderr,
                             "[asset] CRC tellg 失败（跳过本任务）: %s\n",
                             task.path.c_str());
            } else {
            const auto sz = static_cast<std::uint64_t>(raw_sz);
            f.seekg(0, std::ios::beg);
            Poco::Checksum ck(Poco::Checksum::TYPE_CRC32);
            std::vector<char> buf(kChunkSize);
            while (f) {
                f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
                const auto got = f.gcount();
                if (got > 0) ck.update(buf.data(), got);
            }
            char hexbuf[16];
            std::snprintf(hexbuf, sizeof(hexbuf), "%08x",
                          static_cast<unsigned>(ck.checksum()));
            hex = hexbuf;
            // 回填条目（仅当该 id 仍指向本任务文件——期间可能被重新注册
            // 覆盖为别的文件，此时结果作废、由新任务回填）+ 写缓存
            std::lock_guard<std::mutex> lock(mutex_);
            const auto it = assets_.find(task.asset_id);
            if (it != assets_.end() && it->second.path == task.path) {
                it->second.crc_hex     = hex;
                it->second.crc_pending = false;
                it->second.size        = sz; // 以磁盘实况为准
                it->second.mtime       = task.mtime;
            }
            crc_cache_[task.path] = CrcCache{sz, task.mtime, hex};
            }
        }

        // ---- 持久化 + 就绪回调（锁外做 IO；仅真算出 CRC 时）----
        // 缓存文件：原子重写全量（KB 级，逐个 CRC 完成时一次）
        // on_crc_ready：Editor 订阅做"CRC 就绪自动补推清单"（防抖在
        // 订阅方）——显示端身份表从"空 checksum"一次对齐到真值
        if (!hex.empty()) {
            std::map<std::string, CrcCache> snap;
            std::string cpath;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                cpath = cache_path_;
                snap  = crc_cache_;
            }
            if (!cpath.empty()) saveCacheFile(cpath, snap);
            if (on_crc_ready) on_crc_ready();
        }
    }
}

std::size_t AssetHttpServer::asset_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return assets_.size();
}

bool AssetHttpServer::assetInfo(const std::string& asset_id,
                                AssetEntry& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = assets_.find(asset_id);
    if (it == assets_.end()) return false;
    out.file_path = it->second.path;
    out.size_bytes = it->second.size;
    out.crc32_hex = it->second.crc_hex;
    out.crc_pending = it->second.crc_pending;
    out.mtime = it->second.mtime;
    return true;
}

// ---------------------------------------------------------------------------
// CRC 缓存持久化（2026-11-09）：路径 → {size, mtime, hex} 四字段 TSV。
// 磁盘形态 = 跨会话的 crc_cache_：editor 重启后 enableCacheFile 立即恢复，
// 场景/清单从第一帧起就带真值 checksum——显示端不再因"空 checksum 窗口"
// 重传已就绪文件（用户实测"editor 先启 + display 立即上线 → 重复同步"
// 根因①）。mtime 以 file_time_type 时钟 tick 原值存取（同平台同运行时
// 自比；表示法变化 → 失配自动重算，安全退化）。
// ---------------------------------------------------------------------------
void AssetHttpServer::enableCacheFile(const std::string& path) {
    if (path.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    cache_path_ = path;
    // 只并入内存表没有的键：本会话已算/已命中的条目是权威（磁盘文件可能
    // 滞后）——merge 语义恰好只搬移左侧缺失的键
    auto loaded = loadCacheFile(path);
    crc_cache_.merge(loaded);
}

void AssetHttpServer::saveCacheFile(
    const std::string& path, const std::map<std::string, CrcCache>& cache) {
    const std::string tmp = path + ".tmp";
    {
        std::ofstream f(u8path(tmp), std::ios::binary | std::ios::trunc);
        if (!f) return;
        for (const auto& kv : cache) {
            f << kv.first << '\t' << kv.second.size << '\t'
              << kv.second.mtime.time_since_epoch().count() << '\t'
              << kv.second.hex << '\n';
        }
        if (!f.good()) {
            f.close();
            std::error_code ec;
            std::filesystem::remove(u8path(tmp), ec);
            return;
        }
    }
    std::error_code ec;
    std::filesystem::rename(u8path(tmp), u8path(path), ec); // MSVC rename 带覆盖语义
    if (ec) {
        std::error_code ec2;
        std::filesystem::remove(u8path(tmp), ec2);
    }
}

std::map<std::string, AssetHttpServer::CrcCache>
AssetHttpServer::loadCacheFile(const std::string& path) {
    std::map<std::string, CrcCache> out;
    std::ifstream f(u8path(path), std::ios::binary);
    if (!f) return out;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        // 四字段：path \t size \t mtime_ticks \t crc_hex（path 不含 \t）
        const auto t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        const auto t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        const auto t3 = line.find('\t', t2 + 1);
        if (t3 == std::string::npos) continue;
        const std::string p     = line.substr(0, t1);
        const std::string size  = line.substr(t1 + 1, t2 - t1 - 1);
        const std::string ticks = line.substr(t2 + 1, t3 - t2 - 1);
        const std::string hex   = line.substr(t3 + 1);
        if (p.empty() || hex.empty()) continue;
        try {
            CrcCache c;
            c.size  = std::stoull(size);
            c.mtime = std::filesystem::file_time_type{
                std::filesystem::file_time_type::duration{std::stoll(ticks)}};
            c.hex   = hex;
            out.emplace(p, std::move(c));
        } catch (const std::exception&) {
            continue; // 坏行跳过（缓存性质，不致命）
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// HandlerFactory / RequestHandler（定义于 .cpp，经友元访问注册表）
// ---------------------------------------------------------------------------
class AssetHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    explicit AssetHandlerFactory(AssetHttpServer& owner) : owner_(owner) {}

    Poco::Net::HTTPRequestHandler* createRequestHandler(
        const Poco::Net::HTTPServerRequest& /*req*/) override {
        return new AssetHandler(owner_);
    }

private:
    // -----------------------------------------------------------------------
    // 单请求处理器：GET/HEAD /asset/<id>（Range → 206）
    // -----------------------------------------------------------------------
    class AssetHandler : public Poco::Net::HTTPRequestHandler {
    public:
        explicit AssetHandler(AssetHttpServer& owner) : owner_(owner) {}

        void handleRequest(Poco::Net::HTTPServerRequest& req,
                           Poco::Net::HTTPServerResponse& resp) override {            // ---- 1. 路由：仅 /asset/<id> ----
            const std::string& uri = req.getURI();
            static const std::string kRoute = "/asset/";
            if (uri.rfind(kRoute, 0) != 0) {
                resp.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
                resp.setContentLength(0);
                resp.send();
                return;
            }
            const std::string asset_id = uri.substr(kRoute.size());
            // 去查询串（未来扩展 "?v=..." 时不受影响）
            const auto qmark = asset_id.find('?');
            const std::string id =
                (qmark == std::string::npos) ? asset_id : asset_id.substr(0, qmark);

            // ---- 2. 查注册表（快照拷贝，IO 在锁外做）----
            AssetHttpServer::Entry e;
            {
                std::lock_guard<std::mutex> lock(owner_.mutex_);
                const auto it = owner_.assets_.find(id);
                if (it == owner_.assets_.end()) {
                    resp.setStatusAndReason(HTTPResponse::HTTP_NOT_FOUND);
                    resp.setContentLength(0);
                    resp.send();
                    return;
                }
                e = it->second;
            }

            // ---- 3. 打开文件（注册表信息可能滞后于磁盘——fail-safe）----
            std::ifstream f(u8path(e.path), std::ios::binary);
            if (!f) {
                resp.setStatusAndReason(
                    HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
                resp.setContentLength(0);
                resp.send();
                return;
            }
            f.seekg(0, std::ios::end);
            const auto fsize = static_cast<std::uint64_t>(f.tellg());

            // ---- 4. 解析 Range ----
            std::uint64_t offset = 0;
            const int r = parseRangeOffset(req.get("Range", ""), fsize, offset);
            if (r < 0) { // 非法/越界
                resp.setStatusAndReason(
                    HTTPResponse::HTTP_REQUESTED_RANGE_NOT_SATISFIABLE);
                resp.set("Content-Range", "bytes */" + std::to_string(fsize));
                resp.setContentLength(0);
                resp.send();
                return;
            }

            // ---- 5. 响应头（公共：大小与校验和都发全文件口径）----
            if (r == 1) {
                resp.setStatusAndReason(HTTPResponse::HTTP_PARTIAL_CONTENT);
                // Content-Range: bytes a-(fsize-1)/fsize（到文件尾；MVP 单区间）
                resp.set("Content-Range", "bytes " + std::to_string(offset) +
                                              "-" +
                                              std::to_string(fsize - 1) + "/" +
                                              std::to_string(fsize));
            } else {
                resp.setStatusAndReason(HTTPResponse::HTTP_OK);
            }
            resp.setContentType("application/octet-stream");
            resp.set("X-Asset-Size", std::to_string(fsize));
            // CRC 后台计算未就绪时不发此头——Player 端按"无 checksum 只比
            // 长度"降级（AssetReceiver 约定），分发链路不受阻
            if (!e.crc_hex.empty()) resp.set("X-Asset-Crc32", e.crc_hex);
            resp.setContentLength(static_cast<std::int64_t>(fsize - offset));
            resp.setKeepAlive(true);

            if (req.getMethod() == Poco::Net::HTTPRequest::HTTP_HEAD) {
                resp.send(); // HEAD：只要头
                return;
            }

            // ---- 6. 流式发送：seek 到偏移，64KB 分块写响应流 ----
            std::ostream& out = resp.send();
            f.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
            std::vector<char> buf(kChunkSize);
            while (f && out) {
                f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
                const auto got = f.gcount();
                if (got > 0)
                    out.write(buf.data(), static_cast<std::streamsize>(got));
            }
            // out 析构时 flush；写失败由客户端侧长度/CRC 校验兜底
        }

    private:
        AssetHttpServer& owner_; // 服务本体（经友元访问注册表）
    };

    AssetHttpServer& owner_;
};

// ---------------------------------------------------------------------------
// 生命周期（start 放在 HandlerFactory 定义之后——需要其完整类型）
// ---------------------------------------------------------------------------
bool AssetHttpServer::start(std::uint16_t port, int max_threads) {
    if (server_) return true; // 幂等

    auto params = new Poco::Net::HTTPServerParams;
    params->setMaxThreads(max_threads);
    params->setMaxQueued(32);                        // 积压上限（Player 个位数）
    // 【单位警示】Timespan 单参构造参数是微秒——15s/30s 需显式 ×1e6
    params->setKeepAliveTimeout(Poco::Timespan(15 * 1'000'000)); // 慢环境容忍
    params->setTimeout(Poco::Timespan(30 * 1'000'000));          // 读超时（同上）

    // HandlerFactory 捕获 this：引用注册表（本对象生命周期覆盖 server_）
    Poco::Net::HTTPRequestHandlerFactory::Ptr factory(
        new AssetHandlerFactory(*this));
    try {
        // 便捷构造：内部创建 ServerSocket 绑定任意地址 + TCPServer 线程池
        server_ = std::make_unique<Poco::Net::HTTPServer>(
            factory, port, Poco::Net::HTTPServerParams::Ptr(params));
        server_->start();
    } catch (const Poco::Exception&) {
        server_.reset();
        return false;
    }
    port_ = server_->port(); // 便捷构造下 HTTPServer::port() 返回实际端口
    return true;
}

} // namespace ms
