// ============================================================================
// mpcdi_service_session —— MPCDI 会话持久化（active/*.json 落盘 + 启动恢复）。
//
// 职责单一（收编自 MMC mpcdi_service_session.cpp，语义逐条对齐）：
//   reload_active_locked（启动装载 active 目录）、write_session_locked
//  （enable/disable 落盘会话）、restore_on_startup（重启后重走构建路径
//   恢复几何）。包管理/几何构建在 mpcdi_service.cpp。
// ============================================================================

#include "mpcdi_service.hpp"

#include "json_util.hpp"
#include "path_utf8.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace media_api {
namespace fs = std::filesystem;
namespace {

bool read_entire_file(const fs::path& path, std::string& out) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) return false;
    std::ostringstream oss;
    oss << ifs.rdbuf();
    out = oss.str();
    return true;
}

/** "r1,r2" → "\"r1\",\"r2\""（写 selected_regions 数组用；空串返回空）。 */
std::string csv_to_json_array(const std::string& csv) {
    std::string out;
    size_t start = 0;
    while (start <= csv.size()) {
        size_t comma = csv.find(',', start);
        std::string item = csv.substr(
            start, comma == std::string::npos ? std::string::npos
                                              : comma - start);
        while (!item.empty() &&
               std::isspace(static_cast<unsigned char>(item.front())))
            item.erase(item.begin());
        while (!item.empty() &&
               std::isspace(static_cast<unsigned char>(item.back())))
            item.pop_back();
        if (!item.empty()) {
            if (!out.empty()) out += ",";
            out += "\"" + json_escape(item) + "\"";
        }
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return out;
}

}  // namespace

void MpcdiService::reload_active_locked() {
    enabled_ = false;
    active_package_id_.clear();
    active_region_.clear();
    active_layout_json_.clear();
    active_options_json_.clear();
    last_manifest_json_.clear();
    last_session_json_.clear();
    if (cache_dir_.empty()) return;

    auto active = utf8_to_path(cache_dir_) / "active";

    std::string man;
    if (read_entire_file(active / "manifest.json", man))
        last_manifest_json_ = man;

    std::string sess;
    if (read_entire_file(active / "session.json", sess)) {
        last_session_json_ = sess;
        extract_string_field(sess, "package_id", active_package_id_);
        extract_string_field(sess, "region", active_region_);
        extract_bool_field(sess, "enabled", enabled_);
        extract_json_value(sess, "layout", active_layout_json_);
        extract_json_value(sess, "options", active_options_json_);
    }

    // state.json 兜底（旧版/session 缺字段时）
    std::string st;
    if (read_entire_file(active / "state.json", st)) {
        std::string pid, reg;
        bool en = false;
        if (extract_string_field(st, "package_id", pid) &&
            active_package_id_.empty()) {
            active_package_id_ = pid;
        }
        if (extract_string_field(st, "region", reg) && active_region_.empty()) {
            active_region_ = reg;
        }
        if (extract_bool_field(st, "enabled", en) && !enabled_) enabled_ = en;
    }
}

void MpcdiService::write_session_locked(bool enabled) {
    auto out_path = utf8_to_path(cache_dir_) / "active";
    std::error_code ec;
    fs::create_directories(out_path, ec);

    // session.json：restore_on_startup 的唯一权威来源。
    std::ostringstream sess;
    sess << "{\"enabled\":" << (enabled ? "true" : "false")
         << ",\"package_id\":\"" << json_escape(active_package_id_) << "\""
         << ",\"region\":\"" << json_escape(active_region_) << "\""
         << ",\"baker\":\"native\"";
    if (!active_region_.empty()) {
        sess << ",\"selected_regions\":[" << csv_to_json_array(active_region_)
             << "]";
    }
    if (!active_layout_json_.empty())
        sess << ",\"layout\":" << active_layout_json_;
    if (!active_options_json_.empty())
        sess << ",\"options\":" << active_options_json_;
    sess << "}";
    last_session_json_ = sess.str();
    {
        std::ofstream ofs(out_path / "session.json",
                          std::ios::binary | std::ios::trunc);
        ofs << last_session_json_;
    }

    // state.json：简版冗余（与 v26 文件结构兼容，便于排查）。
    std::ofstream state(out_path / "state.json",
                        std::ios::binary | std::ios::trunc);
    state << "{\"enabled\":" << (enabled ? "true" : "false")
          << ",\"package_id\":\"" << json_escape(active_package_id_) << "\""
          << ",\"region\":\"" << json_escape(active_region_) << "\""
          << ",\"baker\":\"native\"}";
}

bool MpcdiService::restore_on_startup(std::string& err) {
    // 锁内只拷贝会话参数，enable 内部会自行加锁（避免重入死锁）。
    bool enabled = false;
    std::string package_id, region, layout, options;
    {
        std::lock_guard<std::mutex> lock(mu_);
        enabled = enabled_;
        package_id = active_package_id_;
        region = active_region_;
        layout = active_layout_json_;
        options = active_options_json_;
    }
    if (!enabled) {
        err = "mpcdi not enabled in session";
        return false;
    }
    if (package_id.empty() || region.empty()) {
        err = "session missing package/region";
        return false;
    }
    // 重走构建路径：包文件仍在 cache/packages/current 下。
    return enable(package_id, region, layout, options, err);
}

}  // namespace media_api
