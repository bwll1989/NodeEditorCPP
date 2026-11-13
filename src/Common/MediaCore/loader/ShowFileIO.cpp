// ============================================================================
//  ShowFileIO.cpp —— .show 项目文件序列化/反序列化实现（第五迭代）
//  【2026-11-04 JSON 化】保存 = protobuf 官方 JSON 映射（美化 + snake_case +
//  零值打印，可读可手编可 git diff）；加载 = 双格式自适应（JSON 新格式 /
//  MSSHOW 魔数 pb 旧格式），映射与校验链路完全复用。
// ============================================================================

#include "loader/ShowFileIO.hpp"

#include <cstdio>
#include <filesystem>
#include <unordered_set>

#include <google/protobuf/util/json_util.h>

#include "loader/SceneLoader.hpp"
#include "runtime/AssetPath.hpp" // u8Path：.show 路径可含中文（UTF-8 协议约定）

namespace ms {

// 注意：不使用 using 引入 ms_proto::DisplayEntry/ShowDocument——
// ms 命名空间内已有同名内存模型类型（DisplayEntry/ShowDocument），
// using 声明会与之冲突（C2874）；一律 ms_proto:: 全限定。

namespace {

// ---------------------------------------------------------------------------
// JSON 打印选项（保存格式契约，三处缺一影响可读性/兼容性）：
//   * add_whitespace                     → 美化缩进（人可读 + git diff 友好）
//   * preserve_proto_field_names         → 保留 proto 原始 snake_case 字段名
//                                          （默认 camelCase 会让手编文件的人
//                                          对照 proto 文件时困惑）
//   * always_print_fields_with_no_presence → 显式字段（port/enabled 等）
//                                          即使为默认值也打印——手编者
//                                          能看到全部可调项，不留猜谜
// ---------------------------------------------------------------------------
google::protobuf::util::JsonPrintOptions jsonPrintOptions() {
    google::protobuf::util::JsonPrintOptions opt;
    opt.add_whitespace = true;
    opt.preserve_proto_field_names = true;
    opt.always_print_fields_with_no_presence = true;
    return opt;
}

} // namespace

// ---------------------------------------------------------------------------
// pb ↔ 内存模型
// ---------------------------------------------------------------------------
ms_proto::ShowDocument ShowFileIO::toProto(const ShowDocument& doc) {
    ms_proto::ShowDocument pb;
    // Scene 部分直接复用 SceneLoader 的映射（单一事实来源）
    *pb.mutable_scene() = SceneLoader::toProto(doc.scene);
    for (const auto& d : doc.displays) {
        auto* pd  = pb.add_displays();
        pd->set_display_id(d.display_id);
        pd->set_host(d.host);
        pd->set_port(d.port);
        pd->set_enabled(d.enabled);
        pd->set_pairing_code(d.pairing_code);
    }
    return pb;
}

std::optional<ShowDocument> ShowFileIO::fromProto(const ms_proto::ShowDocument& pb,
                                                  std::string& err) {
    ShowDocument doc;
    // Scene：走 SceneLoader::fromProto —— 引用完整性校验（clip→asset、
    // 轨道类型约束等）与 .scene 同标准，fail-fast
    auto scene = SceneLoader::fromProto(pb.scene(), err);
    if (!scene) return std::nullopt;
    doc.scene = std::move(*scene);

    std::unordered_set<std::string> seen_ids;
    for (const auto& pd : pb.displays()) {
        DisplayEntry d;
        d.display_id = pd.display_id();
        d.host       = pd.host();
        // 【port 值域】int32 → uint16 静默截断会把 70000 变成 4464——
        // display 连去错误端口且无任何报错；越界显式拒绝（0 允许：未配置）
        if (pd.port() < 0 || pd.port() > 65535) {
            err = "显示端 '" + d.display_id + "' 端口越界 (port=" +
                  std::to_string(pd.port()) + ")";
            return std::nullopt;
        }
        // 【id 校验】空 id 无法定位 connector 槽位；重复 id 使条目歧义
        if (d.display_id.empty()) {
            err = "display_id 为空的显示端条目（载荷损坏）";
            return std::nullopt;
        }
        if (!seen_ids.insert(d.display_id).second) {
            err = "display_id 重复 '" + d.display_id + "'（载荷损坏）";
            return std::nullopt;
        }
        d.port    = static_cast<std::uint16_t>(pd.port());
        d.enabled = pd.enabled();
        d.pairing_code = pd.pairing_code(); // 配对码（空 = 不认证）
        doc.displays.push_back(std::move(d));
    }
    return doc;
}

// ---------------------------------------------------------------------------
// 文件 IO
// ---------------------------------------------------------------------------
bool ShowFileIO::toJsonText(const ShowDocument& doc, std::string& out,
                            std::string& err) {
    const ms_proto::ShowDocument pb = toProto(doc);
    const auto status = google::protobuf::util::MessageToJsonString(
        pb, &out, jsonPrintOptions());
    if (!status.ok()) {
        err = "ShowDocument JSON 序列化失败: " + std::string(status.message());
        return false;
    }
    return true;
}

std::optional<ShowDocument> ShowFileIO::fromJsonText(const std::string& text,
                                                     std::string& err) {
    // ignore_unknown_fields = 向前兼容（未来版本新增字段不炸旧读取）
    google::protobuf::util::JsonParseOptions popt;
    popt.ignore_unknown_fields = true;
    ms_proto::ShowDocument pb;
    const auto status =
        google::protobuf::util::JsonStringToMessage(text, &pb, popt);
    if (!status.ok()) {
        err = "不是有效的 .show 项目文件（JSON 解析失败） [" +
              std::string(status.message()) + "]";
        return std::nullopt;
    }
    return fromProto(pb, err); // SceneLoader::fromProto 引用完整性校验
}

bool ShowFileIO::saveToFile(const ShowDocument& doc, const std::string& path,
                            std::string& err) {
    // 序列化（锁无关：ShowDocument 为值语义，调用方保证无并发写）
    // 【JSON 格式 2026-11-04】pb → 官方 JSON 映射（选项见 jsonPrintOptions），
    // 产出 UTF-8 文本；MessageToJsonString 失败（非法字段名/映射冲突）极罕见，
    // 一律报错不落盘。（M5：文本级逻辑收口 toJsonText——单一事实来源）
    std::string payload;
    if (!toJsonText(doc, payload, err)) return false;

    // 目录不存在则创建（保存到新子目录的友好性；失败时报错）
    // 【u8Path 必经】path 来自 Qt（UTF-8）；窄串构造按 ACP(GBK) 误解码，
    // 非法 GBK 序列直接抛（2026-11-13 与素材链同源加固）
    std::error_code ec;
    const auto parent = ms::u8Path(path).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            err = "创建目录失败: " + path + " (" + ec.message() + ")";
            return false;
        }
    }

    // 【原子替换】先写同目录临时文件，全部落盘成功后 rename 覆盖目标——
    // 直接写目标时半途崩溃/断电会留下半截 .show（载入端虽能靠 JSON/魔数
    // 解析拦截，但用户的上一版存档已毁）；同盘 rename 为原子操作。
    const std::string tmp = path + ".tmp";
#ifdef _WIN32
    std::FILE* f = _wfopen(ms::u8Path(tmp).c_str(), L"wb"); // 宽字符打开：中文名不误解码
#else
    std::FILE* f = std::fopen(tmp.c_str(), "wb"); // POSIX 窄字符即 UTF-8
#endif
    if (!f) {
        err = "无法打开临时文件写入: " + tmp;
        return false;
    }
    if (std::fwrite(payload.data(), 1, payload.size(), f) != payload.size() ||
        std::fclose(f) != 0) {
        err = "写入临时文件失败: " + tmp;
        std::error_code rm_ec;
        std::filesystem::remove(ms::u8Path(tmp), rm_ec); // 清理残件（尽力而为）
        return false;
    }
    std::error_code rec;
    std::filesystem::rename(ms::u8Path(tmp), ms::u8Path(path), rec); // MSVC 实现带覆盖语义
    if (rec) {
        err = "临时文件替换目标失败: " + path + " (" + rec.message() + ")";
        std::error_code rm_ec2;
        std::filesystem::remove(ms::u8Path(tmp), rm_ec2);
        return false;
    }
    return true;
}

std::optional<ShowDocument> ShowFileIO::loadFromFile(const std::string& path,
                                                     std::string& err) {
    // ---- 整读文件（.show 量级为几十 KB～几 MB，一次读入足够）----
    // 【u8Path 必经】同 saveToFile（中文名宽字符打开，防 ACP 误解码/抛异常）
#ifdef _WIN32
    std::FILE* f = _wfopen(ms::u8Path(path).c_str(), L"rb");
#else
    std::FILE* f = std::fopen(path.c_str(), "rb");
#endif
    if (!f) {
        err = "无法打开文件: " + path;
        return std::nullopt;
    }
    std::string bytes;
    char buf[64 * 1024];
    std::size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
        bytes.append(buf, n);
    const bool read_err = std::ferror(f) != 0;
    std::fclose(f);
    if (read_err) {
        err = "读取文件失败: " + path;
        return std::nullopt;
    }

    // ---- 格式自适应：旧版二进制（魔数）优先，否则按 JSON 解析 ------------
    if (bytes.size() >= sizeof(kMagic) &&
        std::memcmp(bytes.data(), kMagic, sizeof(kMagic)) == 0) {
        // 旧版格式：魔数后跟 ShowDocument pb 二进制（兼容历史项目文件）
        ms_proto::ShowDocument pb;
        if (!pb.ParseFromArray(bytes.data() + sizeof(kMagic),
                               static_cast<int>(bytes.size() -
                                                sizeof(kMagic)))) {
            err = ".show 内容损坏（旧版二进制格式 protobuf 解析失败）: " + path;
            return std::nullopt;
        }
        return fromProto(pb, err);
    }
    // 新版 JSON：文本内容直接映射为 ShowDocument。
    // 误选 .scene（pb 二进制）/ 随机文件在此以解析失败拦截（fail-fast）。
    //（M5：解析收口 fromJsonText——单一事实来源）
    return fromJsonText(bytes, err);
}

} // namespace ms
