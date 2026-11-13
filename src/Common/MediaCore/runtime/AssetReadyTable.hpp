// ============================================================================
//  AssetReadyTable.hpp —— display 素材就绪表持久化（2026-11-09）
//
//  【解决什么】PlayerRuntime::asset_done_（asset_id → {size, checksum}）
//  原为纯内存表，display 重启即清零——重启后首轮同步对每个已就绪文件重做
//  本地全文件 CRC（GB 级秒级/个）；若恰逢 editor 端 CRC 未就绪（场景/清单
//  带空 checksum，editor 先启 + display 立即上线的常态窗口），AssetReceiver
//  的幂等检查整体跳过，已就绪文件被全量重传——用户实测"重复同步媒体"根因。
//
//  现持久化到 <assets_root>/.asset_state.tsv（tmp + rename 原子覆盖），
//  重启后加载命中项：仅一次 stat（存在 + 大小一致）即视为就绪——零 CRC
//  零网络秒就绪（WATCHOUT 7 Asset Manager assetdb.json 持久库存的同型思路）。
//
//  格式：每行 `asset_id\tsize_bytes\tchecksum`（checksum 可为空；
//  asset_id 为引擎内部生成的 id，不含 \t 与换行）。
//
//  【共享根目录】同机多 display 实例共享 assets_root 时，本文件是共享
//  状态：各实例就绪记录天然一致（同素材同大小同校验和），原子 rename
//  last-writer-wins 无害。
//
//  【失败语义】就绪表是优化缓存：写失败静默（最坏退回"重启后重扫 CRC"
//  的旧行为），读失败得空表（等同首次运行）——绝不因缓存损坏阻断同步。
// ============================================================================

#pragma once

#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <utility>

#include "runtime/AssetPath.hpp" // u8Path：路径可含中文（assets_root 配置项）

namespace ms {

/// asset_id → (size_bytes, checksum)。"checksum" 为 "crc32:xxxxxxxx"
/// 或空串（下载时校验和未知——空值与任何非空值身份不同，见 PlayerRuntime）
using AssetReadyTable =
    std::map<std::string, std::pair<std::uint64_t, std::string>>;

/// 写就绪表（tmp + rename 原子覆盖；IO 失败静默返回 false）。
/// 【u8Path 必经】path = assets_root/.asset_state.tsv，assets_root 可配
/// 中文——窄串构造 path 遇非法 GBK 序列抛异常（拉取线程内 terminate）。
inline bool saveAssetReadyTable(const std::string& path,
                                const AssetReadyTable& table) {
    const std::string tmp = path + ".tmp";
    {
        std::ofstream f(ms::u8Path(tmp), std::ios::binary | std::ios::trunc);
        if (!f) return false;
        for (const auto& kv : table)
            f << kv.first << '\t' << kv.second.first << '\t'
              << kv.second.second << '\n';
        if (!f.good()) {
            f.close();
            std::error_code ec;
            std::filesystem::remove(ms::u8Path(tmp), ec);
            return false;
        }
    }
    std::error_code ec;
    std::filesystem::rename(ms::u8Path(tmp), ms::u8Path(path), ec); // MSVC rename 带覆盖语义
    if (ec) {
        std::error_code ec2;
        std::filesystem::remove(ms::u8Path(tmp), ec2);
        return false;
    }
    return true;
}

/// 读就绪表（文件缺失/损坏 → 空表；坏行跳过，不致命）
inline AssetReadyTable loadAssetReadyTable(const std::string& path) {
    AssetReadyTable out;
    std::ifstream f(ms::u8Path(path), std::ios::binary);
    if (!f) return out;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        // 三字段：id \t size \t checksum（checksum 可为空）
        const auto t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        const auto t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        const std::string id   = line.substr(0, t1);
        const std::string size = line.substr(t1 + 1, t2 - t1 - 1);
        const std::string ck   = line.substr(t2 + 1);
        if (id.empty()) continue;
        try {
            const auto sz = std::stoull(size);
            out.emplace(id, std::make_pair(sz, ck));
        } catch (const std::exception&) {
            continue; // 坏行（size 非数字）：跳过
        }
    }
    return out;
}

} // namespace ms
