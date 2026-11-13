// ============================================================================
//  AssetReceiver.cpp —— 素材拉取实现（断点续传 + CRC 校验）
//
//  主流程（fetch）：
//      0. 本地检查（两层幂等口径，2026-11-09 修订）：
//         期望 checksum 非空 → 本地 CRC 匹配即 AlreadyHave（零网络）；
//         期望 checksum 为空（editor CRC 未就绪窗口）→ HEAD 比服务端
//         大小（一致即 AlreadyHave；服务端带 CRC 时升级为本地 CRC 比对）
//         ——旧实现此窗口整体跳过幂等检查，已就绪文件被全量重传
//      1. HEAD /asset/<id> 拿 X-Asset-Size / X-Asset-Crc32
//         （失败降级：直接带 Range GET，靠响应码判断）
//      2. 本进程已有 part 临时文件 → GET Range: bytes=<已有>-
//         → 206 续写 / 200 覆盖重传 / 416 → 视为已完整转校验
//      3. 流式接收：64KB 分块追加写 part 临时文件
//      4. 校验：大小 + CRC32（不匹配删 part 返回 CrcMismatch）
//      5. 原子上位：part rename 覆盖 save_path（同盘 rename 原子）
//
//  【多实例并发隔离（2026-11-03）】同一台电脑开多个 media_display 且共享
//  assets_root 时，两个实例可能并发拉取同一素材。若直接写 save_path，
//  交错写会损坏彼此数据；改为各写各的 ".part-<pid>"（按进程隔离），
//  校验通过后 rename 覆盖目标——同盘 rename 原子，读者（媒体源/校验）
//  要么看到旧完整文件要么新完整文件，永远看不到半成品。
// ============================================================================

#include "net/AssetReceiver.hpp"

#include "runtime/AssetPath.hpp" // u8Path：UTF-8 窄串 → path（防 ACP 抛异常）

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <vector>

#include <Poco/Exception.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>

#include <chrono>
#include <thread>

#ifdef _WIN32
#include <process.h> // _getpid（part 临时文件按进程隔离命名）
#else
#include <unistd.h>  // getpid（POSIX 对应）
#endif

namespace ms {

using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;

namespace {
constexpr std::size_t kChunkSize = 64 * 1024;
} // namespace

// ---------------------------------------------------------------------------
// 文件 CRC32（流式分块；与 AssetHttpServer::registerAsset 同算法）
// ---------------------------------------------------------------------------
std::string AssetReceiver::fileCrc32Hex(const std::filesystem::path& path,
                                        std::uint64_t& out_size) {
    std::ifstream f(path, std::ios::binary); // path 重载：无 ACP 转换坑
    if (!f) {
        out_size = 0;
        return {};
    }
    f.seekg(0, std::ios::end);
    out_size = static_cast<std::uint64_t>(f.tellg());
    f.seekg(0, std::ios::beg);

    Poco::Checksum ck(Poco::Checksum::TYPE_CRC32);
    std::vector<char> buf(kChunkSize);
    while (f) {
        f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        const auto got = f.gcount();
        if (got > 0) ck.update(buf.data(), got);
    }
    char hex[16];
    std::snprintf(hex, sizeof(hex), "%08x", static_cast<unsigned>(ck.checksum()));
    return hex;
}

// ---------------------------------------------------------------------------
// 主流程
// ---------------------------------------------------------------------------
AssetReceiver::Result AssetReceiver::fetch(const std::string& host,
                                           std::uint16_t port,
                                           const std::string& asset_id,
                                           const std::string& save_path,
                                           const std::string& checksum,
                                           Stats* stats,
                                           const ChunkFn& chunk,
                                           const CancelFn& cancelled) {
    Stats local;
    if (stats) *stats = Stats{};

    /// 取消轮询收口（round7 S1-1）：下载入口/分块循环处检查——命中即
    /// 保留 part 半成品返回（下次 fetch 从 part 尾续传）。GB 级文件
    /// 下载中途被新清单接管时，join 旧线程不必等整个下载结束。
    auto is_cancelled = [&cancelled]() {
        return static_cast<bool>(cancelled) && cancelled();
    };
    if (is_cancelled()) return Result::Cancelled;

    // ======================================================================
    // 0. 下载临时文件路径（".part-<pid>"，按进程隔离；幂等检查也要用）
    // ======================================================================
    // 同机多 display 共享 assets_root：各实例写各自的 part，互不干扰；
    // PID 缺省 id 也含 PID，不重名。save_path 与 part 同目录（rename 才
    // 能保证同盘原子）。
    // 【2026-11-13 中文素材崩溃修复】save/part 全程走 filesystem::path
    // （u8Path 按 UTF-8 解码）——原窄 string 直传 file_size/ifstream/
    // ofstream 时，MSVC 按 ACP(GBK) 隐式构造 path：中文文件名轻则打不开
    //（CRC 恒空），重则非法 GBK 序列抛 std::system_error——而 file_size
    // 的隐式 path 构造发生在 try 块之外，异常逃逸 worker 线程即
    // std::terminate 全进程静默死亡（「同步媒体」崩溃根因，探针实证）。
#if defined(_WIN32)
    const int pid = _getpid();
#else
    const int pid = static_cast<int>(::getpid());
#endif
    const std::filesystem::path save = ms::u8Path(save_path);
    std::filesystem::path part = save;
#if defined(_WIN32)
    part += L".part-" + std::to_wstring(pid);
#else
    part += std::string(".part-") + std::to_string(pid);
#endif
    // 诊断打印用窄串（仅控制台显示，无 fs 语义；UTF-8 字节直出）
    const std::string part_dbg = save_path + ".part-" + std::to_string(pid);

    // ======================================================================
    // 0. 幂等检查（两层口径，2026-11-09 修订——"重复同步媒体"根因②）
    //    期望 checksum 非空（常态）：本地 CRC 直接比对——匹配即零网络。
    //    期望 checksum 为空（editor 端 CRC 后台未就绪窗口，推模型的常态
    //    瞬态）：旧实现整体跳过幂等检查 → 已完整文件也被 GET 全量重传
    //    （editor 先启 + display 立即上线后重启显示端必现）。现改为 HEAD
    //    服务端口径：X-Asset-Size 与本地一致即视为已就绪（与校验段"空
    //    checksum 只比长度"降级口径自洽）；服务端已带 X-Asset-Crc32 时
    //    升级为本地 CRC 比对（口径更强，内容真不一致则落入下载修复）。
    //    【信任边界】同大小不同内容在"双端都无 CRC"窗口内不可辨——真值
    //    清单到达后身份不符触发重拉自愈（与降级校验同一权衡）。
    //    【stat 先行】本地现状只 stat 不读内容——CRC 仅在需要比对时才算
    //    （旧实现无条件全文件扫描，空 checksum 时白付 GB 级读盘）。
    // ======================================================================
    std::uint64_t local_size = 0;
    {
        std::error_code sec;
        const auto sz = std::filesystem::file_size(save, sec);
        if (!sec) local_size = static_cast<std::uint64_t>(sz);
    }

    /// 幂等命中收口：清 part 残件 + 置统计（见上方"顺手清场"注释）
    auto mark_already_have = [&]() {
        std::error_code rem_ec;
        std::filesystem::remove(part, rem_ec);
        if (stats) {
            stats->idempotent = true;
            stats->bytes_written = 0;
        }
        return Result::AlreadyHave;
    };

    if (local_size > 0 && !checksum.empty()) {
        // ---- 口径 A：期望 checksum 可比 → 本地 CRC 直接裁决 ----
        const std::string local_hex = fileCrc32Hex(save, local_size);
        // 期望值可能带 "crc32:" 前缀（Asset.checksum 约定）——兼容两种形态
        if (checksum == local_hex || checksum == "crc32:" + local_hex)
            return mark_already_have();
    } else if (local_size > 0) {
        // ---- 口径 B：期望 checksum 为空 → HEAD 比对服务端口径 ----
        // 独立会话（失败不污染主 GET 会话状态）；HEAD 无响应体
        try {
            Poco::Net::HTTPClientSession hs(host, port);
            hs.setTimeout(Poco::Timespan(10 * 1'000'000));
            hs.setReceiveTimeout(Poco::Timespan(30 * 1'000'000));
            HTTPRequest head(HTTPRequest::HTTP_HEAD, "/asset/" + asset_id);
            hs.sendRequest(head);
            HTTPResponse hresp;
            hs.receiveResponse(hresp);
            if (hresp.getStatus() == HTTPResponse::HTTP_OK) {
                std::uint64_t server_size = 0;
                const std::string szs = hresp.get("X-Asset-Size", "");
                try {
                    if (!szs.empty()) server_size = std::stoull(szs);
                } catch (const std::exception&) {
                    server_size = 0; // 坏头按未知大小降级
                }
                const std::string server_crc = hresp.get("X-Asset-Crc32", "");
                if (server_size != 0 && server_size == local_size) {
                    if (server_crc.empty())
                        return mark_already_have(); // 服务端也无 CRC：只比长度
                    // 服务端有 CRC：升级为本地 CRC 比对（内容裁决）
                    const std::string local_hex =
                        fileCrc32Hex(save, local_size);
                    if (server_crc == local_hex) return mark_already_have();
                    // CRC 不符 = 内容真不一致：落入下方 GET 全量重传修复
                }
            }
        } catch (const Poco::Exception&) {
            // HEAD 失败（网络抖动/素材尚未注册）：不预判，走 GET 流程
        }
    }

    // ======================================================================
    // 1. 建立会话（超时宽松，兼容慢环境）
    // ======================================================================
    // 【单位警示】Poco::Timespan 单参构造的参数是【微秒】——此处必须写
    // 秒数 ×1e6（曾误写 Timespan(10) = 10 微秒，跨机部署分块间隔稍大即
    // 超时重试；同机回环数据已在内核缓冲侥幸掩盖）
    Poco::Net::HTTPClientSession session(host, port);
    session.setTimeout(Poco::Timespan(10 * 1'000'000));          // 连接 10s
    session.setKeepAliveTimeout(Poco::Timespan(30 * 1'000'000)); // 慢环境容忍 30s
    // 【2026-11-13 收紧 60s→15s】读超时是"分块间不活跃上限"而非总时长，
    // 正常拉取（含 3.3GB 流式）不受影响；收紧的意义：服务端半死（幽灵
    // socket / 端口劫持）时，拉取线程与"场景重入 join"（PlayerRuntime
    // fetchAssetsAsync 作废旧拉取线程）的卡顿上限从 60s 降到 15s。
    session.setReceiveTimeout(Poco::Timespan(15 * 1'000'000));   // 大文件读超时 15s

    // ======================================================================
    // 2. 续传决策：本进程上次中断的 part 文件 → Range 续传；否则全量
    // ======================================================================
    // 【口径变化（多实例隔离配套）】续传基准从 save_path 改为 part 文件：
    //   * save_path 只在"完整落盘"后出现（rename 产物）；不完整的
    //     save_path 只可能是旧版本程序的遗留残件，内容不可信——直接
    //     全量重下（rename 覆盖），不再基于它续传；
    //   * part 文件是本进程自己的下载现场（网络异常时特意保留），从
    //     它的文件尾续传。这里只取大小（CRC 校验收完再统一做，省一次
    //     全文件扫描）。
    std::uint64_t part_size = 0;
    {
        std::ifstream pf(part, std::ios::binary);
        if (pf) {
            pf.seekg(0, std::ios::end);
            part_size = static_cast<std::uint64_t>(pf.tellg());
        }
    }
    // 两参构造：version 默认 HTTPMessage::HTTP_1_1
    HTTPRequest get(HTTPRequest::HTTP_GET, "/asset/" + asset_id);
    std::uint64_t resume_from = 0;
    bool have_partial = false;
    if (part_size > 0) {
        // 半成品：从 part 文件尾续传（目标：不重下已传字节）
        // 若服务端不支持 Range 会回 200 全量 → 覆盖写 part
        have_partial = true;
        resume_from = part_size;
        get.set("Range", "bytes=" + std::to_string(part_size) + "-");
    }

    try {
        // ---- 落盘父目录保障：save_path 可能含子目录（如 a.path =
        // "media/demo.mp4"）——统一在此创建，调用方无需记住该约定 ----
        {
            const std::filesystem::path parent = save.parent_path();
            if (!parent.empty()) {
                std::error_code dsec;
                std::filesystem::create_directories(parent, dsec);
                // 失败不打断：后续 out.open 失败会给出 NetworkError
            }
        }

        session.sendRequest(get);
        HTTPResponse resp;
        std::istream& body = session.receiveResponse(resp);
        const auto status = resp.getStatus();

        // ---- 404：素材未注册 ----
        if (status == HTTPResponse::HTTP_NOT_FOUND) return Result::NotFound;

        // ---- 416：服务端认为越界 → 本地可能已完整，转校验 ----
        if (status == HTTPResponse::HTTP_REQUESTED_RANGE_NOT_SATISFIABLE) {
            have_partial = false; // 按"已完整"处理，落校验流程
        }

        // ---- 读取服务端口径（X-Asset-* 头）----
        std::uint64_t server_size = 0;
        std::string  server_crc;
        {
            const auto& hdrs = resp;
            const std::string sz = hdrs.get("X-Asset-Size", "");
            if (!sz.empty()) {
                // std::stoull 抛 std 异常（非 Poco::Exception）——外层 catch
                // 接不住，逃逸出线程函数直接 terminate 整个进程。坏头
                // （代理篡改/版本不符）按"未知大小"降级，不崩
                try {
                    server_size = std::stoull(sz);
                } catch (const std::exception&) {
                    server_size = 0;
                }
            }
            server_crc = hdrs.get("X-Asset-Crc32", "");
        }

        // ---- 状态码分支：206 续写 part / 200 覆盖 part / 其他异常 ----
        std::ofstream out;
        if (status == HTTPResponse::HTTP_PARTIAL_CONTENT && have_partial) {
            // 断点续传：追加写 part（本进程上次下载现场之后接续）
            out.open(part, std::ios::binary | std::ios::app);
            if (stats) stats->resumed = true;
        } else if (status == HTTPResponse::HTTP_OK) {
            // 全量（首次 / 服务端不支持 Range / save_path 残件不可信）：
            // 覆盖写 part
            out.open(part, std::ios::binary | std::ios::trunc);
            resume_from = 0;
            if (stats) stats->resumed = false;
        } else if (status == HTTPResponse::HTTP_REQUESTED_RANGE_NOT_SATISFIABLE) {
            // part 可能已完整（Range 越界）→ 落校验（是否真完整由 CRC 裁决）
        } else {
            return Result::BadResponse;
        }
        if (!out && status != HTTPResponse::HTTP_REQUESTED_RANGE_NOT_SATISFIABLE) {
            std::fprintf(stderr, "[asset] 本地临时文件打开失败: %s\n",
                         part_dbg.c_str());
            return Result::NetworkError; // 本地文件打不开
        }

        // ---- 流式接收：分块追加，内存恒定 ----
        // 【进度上报】bytes_now = 续传前已有字节 + 本次已写（206 续传时
        // resume_from 非零；200 覆盖写时已归零）；bytes_total = 服务端
        // 全量大小（X-Asset-Size 头，0 = 未知 → UI 走不定进度样式）。
        if (out) {
            std::vector<char> buf(kChunkSize);
            while (body) {
                body.read(buf.data(), static_cast<std::streamsize>(buf.size()));
                const auto got = body.gcount();
                if (got > 0) {
                    out.write(buf.data(), static_cast<std::streamsize>(got));
                    local.bytes_written += static_cast<std::uint64_t>(got);
                    if (chunk)
                        chunk(resume_from + local.bytes_written, server_size);
                }
                // 取消检查（每 64KB 一次，函数调用开销可忽略）：命中即刻
                // 放弃——part 保留供续传（与 HTTP 异常同语义）
                if (is_cancelled()) {
                    out.close();
                    return Result::Cancelled;
                }
            }
            out.close();
        }

        // ---- 3. 完整性校验：校验 part（通过后才 rename 上位）----
        std::uint64_t final_size = 0;
        const std::string final_hex = fileCrc32Hex(part, final_size);
        // 期望值带算法前缀（Asset.checksum 约定 "crc32:hex"）→ 拼齐再比；
        // 兼容不带前缀的期望（直接比 hex）。
        // 【口径归一（联测排障修复）】服务端 X-Asset-Crc32 头是【裸 hex】
        // （注册表存 hex），与本地 full_crc（带前缀）直接比较永远不等
        // → "场景未带 checksum + 服务端有头" 组合必判 CrcMismatch（既有
        // 缺陷，单测全带非空 checksum 未覆盖此分支）。统一去前缀比 hex。
        const std::string final_full = "crc32:" + final_hex;
        const bool crc_ok =
            checksum.empty()
                ? (server_crc.empty() ||
                   server_crc == final_hex ||
                   server_crc == final_full)
                : (checksum == final_full || checksum == final_hex);
        if (!crc_ok) {
            // CRC 不符说明内容损坏：删除 part 半成品（续传无意义，重传
            // 修正）；save_path 不受影响（多实例并发下旧文件仍完好）
            // 【诊断留痕】期望/实际 CRC + 期望口径（场景/服务端）+ 落盘
            // 大小——联测期 CRC 排障唯一线索（内容不一致必然有源头）
            std::fprintf(stderr,
                         "[asset] CRC 不符: %s\n  期望(checksum参数)=%s\n"
                         "  实际=%s  server头=%s\n  落盘=%lluB server_size=%lluB\n",
                         part_dbg.c_str(), checksum.c_str(),
                         final_full.c_str(),
                         server_crc.c_str(),
                         static_cast<unsigned long long>(final_size),
                         static_cast<unsigned long long>(server_size));
            {
                std::error_code rem_ec;
                std::filesystem::remove(part, rem_ec);
            }
            return Result::CrcMismatch;
        }
        if (server_size != 0 && final_size != server_size) {
            std::error_code rem_ec;
            std::filesystem::remove(part, rem_ec); // 大小不符：part 不可信，删
            return Result::BadResponse;
        }

        // ---- 4. 原子上位：part → save_path（同盘 rename，MSVC 带覆盖语义）----
        // 目标被占用（如旧文件正被播放器读取）时 rename 失败：保留 part，
        // 下次 fetch 重试（幂等检查/续传都能接续）。rename 失败不损坏任何
        // 一侧文件——这正是原子替换的意义。
        //
        // 【锁冲突自愈（2026-11-05 黑屏联测实证）】重复分发同名素材的常态
        // 场景：本进程媒体源（FFmpeg avio 持续解码）持有旧文件句柄 →
        // MoveFileEx(REPLACE) 对"无 FILE_SHARE_DELETE 的读句柄"必报拒绝
        // 访问 → 素材状态机卡"未就绪" → 渲染层恒占位黑屏（联测三次抓屏
        // 全黑 + "落盘改名失败 拒绝访问" 实证）。而此时目标内容与 part
        // 完全一致（同一素材同一版本重复下载）——CRC 相同即视为上位成功
        // （删 part 清场）；不一致（真换了文件）才保留 part 等锁释放重试。
        // 另加 3×150ms 短重试，吃掉杀软扫描/索引器的瞬时锁。
        {
            bool renamed = false;
            std::error_code rsec;
            for (int i = 0; i < 4 && !renamed; ++i) {
                if (i > 0)
                    std::this_thread::sleep_for(std::chrono::milliseconds(150));
                rsec.clear();
                std::filesystem::rename(part, save, rsec);
                renamed = !rsec;
            }
            if (!renamed) {
                // 目标内容与 part 一致 = 本地已是最新版本（重复分发场景）
                // → 直接视为上位成功，不再让素材状态机卡死
                std::uint64_t dst_size = 0;
                const std::string dst_hex = fileCrc32Hex(save, dst_size);
                if (dst_size == final_size && dst_hex == final_hex) {
                    std::error_code rem_ec;
                    std::filesystem::remove(part, rem_ec); // 清场：part 无保留价值
                    std::fprintf(stderr,
                                 "[asset] 目标被占用但内容与下载一致，"
                                 "视为已上位: %s\n", save_path.c_str());
                } else {
                    std::fprintf(stderr,
                                 "[asset] 落盘改名失败: %s → %s (%s)\n",
                                 part_dbg.c_str(), save_path.c_str(),
                                 rsec.message().c_str());
                    return Result::NetworkError;
                }
            }
        }

        if (stats) {
            stats->bytes_written = local.bytes_written;
            stats->idempotent = false;
        }
        return Result::Ok;

    } catch (const Poco::Exception& e) {
        // 网络/超时异常：part 保留（下次 fetch 从 part 尾续传）
        std::fprintf(stderr, "[asset] HTTP 异常: %s\n", e.displayText().c_str());
        return Result::NetworkError;
    }
}

} // namespace ms
