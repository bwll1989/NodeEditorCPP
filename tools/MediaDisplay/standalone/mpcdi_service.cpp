// ============================================================================
// mpcdi_service —— MPCDI 校准包服务（收编自 MMC mpcdi_service.cpp；
// 投递目标 PlayerFacade → RenderThread::setMpcdiWarp，其余语义对齐）
// ============================================================================

#include "mpcdi_service.hpp"

#include "json_util.hpp"
#include "path_utf8.hpp"

#include "mpcdi/mpcdi_parse.hpp"
#include "mpcdi/mpcdi_to_warp.hpp"
#include "runtime/RenderThread.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace media_api {
namespace fs = std::filesystem;
namespace {

std::string sanitize_filename(std::string name) {
    for (char& c : name) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|') {
            c = '_';
        }
    }
    if (name.empty()) name = "package.mpcdi";
    return name;
}

bool ci_ends_with(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    for (size_t i = 0; i < suffix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[s.size() - suffix.size() + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i])))
            return false;
    }
    return true;
}

/**
 * 解析 JSON 字符串数组（如 layout.slots）；非数组段返回空。
 * null → 空串占位（mosaic 稀疏摆放依赖槽位对齐）。对齐 MMC 同名实现。
 */
void parse_string_array(const std::string& arr_text,
                        std::vector<std::string>& out) {
    size_t i = arr_text.find('[');
    if (i == std::string::npos) return;
    ++i;
    while (i < arr_text.size()) {
        while (i < arr_text.size() &&
               (arr_text[i] == ',' || arr_text[i] == ' ' ||
                arr_text[i] == '\t' || arr_text[i] == '\r' ||
                arr_text[i] == '\n')) {
            ++i;
        }
        if (i >= arr_text.size() || arr_text[i] == ']') break;
        if (arr_text.compare(i, 4, "null") == 0) {
            out.emplace_back();   // null → 空 cell（槽位占位）
            i += 4;
            continue;
        }
        if (arr_text[i] != '"') {
            ++i;                  // 其它非法 token：跳过（宽松处理）
            continue;
        }
        ++i;  // 跳过开引号
        std::string v;
        while (i < arr_text.size()) {
            char c = arr_text[i++];
            if (c == '\\' && i < arr_text.size()) {
                v.push_back(arr_text[i++]);
                continue;
            }
            if (c == '"') break;
            v.push_back(c);
        }
        out.push_back(v);
    }
}

/** region csv 拆分（去空白、保序去重）。 */
void parse_regions_csv(const std::string& csv, std::vector<std::string>& out) {
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
        if (!item.empty() &&
            std::find(out.begin(), out.end(), item) == out.end())
            out.push_back(item);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// 路径与装配
// ---------------------------------------------------------------------------
void MpcdiService::set_paths(const std::string& config_dir) {
    std::lock_guard<std::mutex> lock(mu_);
    // 兼容两种入参：目录（MMC 语义）或配置文件路径（本端 main 传
    // config_write_path=…/config.ini）。文件判定双保险（2026-11-13）：
    //   * is_regular_file——文件已存在时的权威判定；
    //   * .ini 后缀——文件缺位（首启/被误删）时按语义仍当文件（父目录
    //     落缓存）。否则 create_directories 会把"config.ini"当【目录】
    //     建出 config.ini/mpcdi_cache/，此后一切配置写回全部落盘失败
    //     （实证：display 首启无配置文件时 PUT settings 报落盘失败）。
    auto base = utf8_to_path(config_dir);
    std::error_code dec;
    const bool looks_like_file =
        !config_dir.empty() &&
        (fs::is_regular_file(base, dec) ||
         base.extension() == fs::path(".ini"));
    if (looks_like_file) {
        auto parent = base.parent_path();
        base = parent.empty() ? fs::path(".") : parent;
    }
    config_dir_ = path_to_utf8(base);
    cache_dir_ = path_to_utf8(base / "mpcdi_cache");
    std::error_code ec;
    fs::create_directories(utf8_to_path(cache_dir_) / "packages", ec);
    fs::create_directories(utf8_to_path(cache_dir_) / "active", ec);
    reload_active_locked();
}

void MpcdiService::set_render_thread(ms::RenderThread* rt) {
    std::lock_guard<std::mutex> lock(mu_);
    rt_ = rt;
}

std::string MpcdiService::package_dir(const std::string& id) const {
    return path_to_utf8(utf8_to_path(cache_dir_) / "packages" / id);
}

std::string MpcdiService::package_file(const std::string& id) const {
    auto dir = utf8_to_path(package_dir(id));
    if (!fs::exists(dir)) return {};
    for (auto& e : fs::directory_iterator(dir)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        for (char& c : ext)
            c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
        if (ext == ".mpcdi" || ext == ".zip") return path_to_utf8(e.path());
    }
    return {};
}

std::string MpcdiService::make_package_id(const std::string& /*filename*/) const {
    // 单槽存储：恒覆写同一包 id（对齐 MMC）
    return "current";
}

// ---------------------------------------------------------------------------
// 状态/包列表/region 目录/metadata（JSON 组装，语义对齐 MMC）
// ---------------------------------------------------------------------------
std::string MpcdiService::status_json() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::ostringstream oss;
    oss << "{\"enabled\":" << (enabled_ ? "true" : "false")
        << ",\"package_id\":"
        << (active_package_id_.empty()
                ? "null"
                : ("\"" + json_escape(active_package_id_) + "\""))
        << ",\"region\":"
        << (active_region_.empty()
                ? "null"
                : ("\"" + json_escape(active_region_) + "\""))
        << ",\"baker\":\"native\""
        << ",\"cache_dir\":\"" << json_escape(cache_dir_) << "\"";
    if (!last_manifest_json_.empty()) {
        oss << ",\"manifest\":" << last_manifest_json_;
    } else {
        oss << ",\"manifest\":null";
    }
    if (!last_session_json_.empty() && last_session_json_.front() == '{') {
        oss << ",\"session\":" << last_session_json_;
    } else {
        oss << ",\"session\":null";
    }
    oss << "}";
    return oss.str();
}

std::string MpcdiService::packages_json() const {
    std::lock_guard<std::mutex> lock(mu_);
    std::ostringstream oss;
    oss << "{\"packages\":[";
    bool first = true;
    auto root = utf8_to_path(cache_dir_) / "packages";
    std::error_code ec;
    if (fs::exists(root, ec)) {
        for (auto& e : fs::directory_iterator(root, ec)) {
            if (!e.is_directory()) continue;
            std::string id = path_to_utf8(e.path().filename());
            std::string file = package_file(id);
            if (file.empty()) continue;
            auto fsize = fs::file_size(utf8_to_path(file), ec);
            if (!first) oss << ",";
            first = false;
            oss << "{\"id\":\"" << json_escape(id) << "\","
                << "\"name\":\""
                << json_escape(path_to_utf8(utf8_to_path(file).filename()))
                << "\",\"path\":\"" << json_escape(file) << "\","
                << "\"size\":" << static_cast<long long>(fsize) << "}";
        }
    }
    oss << "]}";
    return oss.str();
}

bool MpcdiService::regions_json(const std::string& package_id,
                                std::string& out_json, std::string& err) {
    std::string pkg_path;
    {
        std::lock_guard<std::mutex> lock(mu_);
        pkg_path = package_file(package_id);
    }
    if (pkg_path.empty()) {
        err = "package not found";
        return false;
    }

    // 解包 + 解析 XML（zip/PFM 都不碰：region 目录只需 mpcdi.xml 结构）
    ms::MpcdiPackageData data;
    if (!ms::load_mpcdi_package(pkg_path, data, err)) return false;
    ms::MpcdiInfo info;
    if (!ms::parse_mpcdi_xml(data, info, err)) return false;

    std::ostringstream oss;
    oss << "{\"regions\":[";
    bool first = true;
    for (const auto& r : info.regions) {
        if (!first) oss << ",";
        first = false;
        oss << "{\"id\":\"" << json_escape(r.id) << "\""
            << ",\"key\":\"" << json_escape(r.key) << "\""
            << ",\"display_id\":\"" << json_escape(r.display_id) << "\""
            << ",\"buffer_id\":\"" << json_escape(r.buffer_id) << "\""
            << ",\"xResolution\":" << r.x_resolution
            << ",\"yResolution\":" << r.y_resolution
            << ",\"has_warp\":"
            << (r.geometry_warp_file.empty() ? "false" : "true") << "}";
    }
    oss << "]}";
    out_json = oss.str();
    return true;
}

bool MpcdiService::metadata_json(const std::string& package_id,
                                 std::string& out_json, std::string& err) {
    std::string pkg_path;
    {
        std::lock_guard<std::mutex> lock(mu_);
        pkg_path = package_file(package_id);
    }
    if (pkg_path.empty()) {
        err = "package not found";
        return false;
    }

    ms::MpcdiPackageData data;
    if (!ms::load_mpcdi_package(pkg_path, data, err)) return false;
    ms::MpcdiInfo info;
    if (!ms::parse_mpcdi_xml(data, info, err)) return false;

    auto fmt_f = [](float v) {
        std::ostringstream s;
        s.precision(4);
        s << std::fixed << v;
        return s.str();
    };

    std::ostringstream oss;
    oss << "{\"profile\":\"" << json_escape(info.profile) << "\""
        << ",\"version\":\"" << json_escape(info.version) << "\""
        << ",\"buffers\":[";
    for (size_t i = 0; i < info.buffers.size(); ++i) {
        if (i) oss << ",";
        oss << "{\"id\":\"" << json_escape(info.buffers[i].id) << "\""
            << ",\"width\":" << info.buffers[i].x_resolution
            << ",\"height\":" << info.buffers[i].y_resolution << "}";
    }
    oss << "],\"regions\":[";
    for (size_t i = 0; i < info.regions.size(); ++i) {
        const auto& r = info.regions[i];
        if (i) oss << ",";

        // 匹配 buffer 级分辨率（XML buffer 属性；viewer "buffer" 行）
        const ms::MpcdiBufferInfo* buf = nullptr;
        for (const auto& b : info.buffers) {
            if (b.id == r.buffer_id) {
                buf = &b;
                break;
            }
        }

        oss << "{\"id\":\"" << json_escape(r.id) << "\""
            << ",\"key\":\"" << json_escape(r.key) << "\""
            << ",\"buffer_id\":\"" << json_escape(r.buffer_id) << "\"";
        if (buf) {
            oss << ",\"buffer\":{\"id\":\"" << json_escape(buf->id) << "\""
                << ",\"width\":" << buf->x_resolution
                << ",\"height\":" << buf->y_resolution << "}";
        } else {
            oss << ",\"buffer\":null";
        }
        oss << ",\"width\":" << r.x_resolution
            << ",\"height\":" << r.y_resolution
            << ",\"rect\":{\"x\":" << fmt_f(r.x) << ",\"y\":" << fmt_f(r.y)
            << ",\"x_size\":" << fmt_f(r.x_size)
            << ",\"y_size\":" << fmt_f(r.y_size) << "}";

        // warp 统计：全分辨率 PFM 扫描（NaN 单独计数，对齐 viewer）
        bool warp_ok = false;
        if (!r.geometry_warp_file.empty()) {
            if (const auto* pfm = data.find(r.geometry_warp_file, r.buffer_id)) {
                ms::MpcdiWarpStats st;
                std::string werr;
                if (ms::compute_warp_stats(*pfm, st, werr)) {
                    warp_ok = true;
                    oss << ",\"warp\":{\"available\":true"
                        << ",\"width\":" << st.width
                        << ",\"height\":" << st.height
                        << ",\"min_u\":" << fmt_f(st.min_u)
                        << ",\"max_u\":" << fmt_f(st.max_u)
                        << ",\"min_v\":" << fmt_f(st.min_v)
                        << ",\"max_v\":" << fmt_f(st.max_v)
                        << ",\"nan_count\":" << st.nan_count
                        << ",\"total_texels\":" << st.total_texels
                        << ",\"normalized\":"
                        << (st.looks_normalized ? "true" : "false") << "}";
                }
            }
        }
        if (!warp_ok) oss << ",\"warp\":{\"available\":false}";

        // alpha（blend）尺寸：PNG IHDR 提取（不解码整图）
        bool alpha_ok = false;
        if (!r.alpha_map.empty()) {
            if (const auto* png = data.find(r.alpha_map, r.buffer_id)) {
                int aw = 0, ah = 0;
                if (ms::png_header_size(*png, aw, ah) && aw > 0 && ah > 0) {
                    alpha_ok = true;
                    oss << ",\"alpha\":{\"available\":true"
                        << ",\"width\":" << aw << ",\"height\":" << ah
                        << ",\"gamma\":"
                        << (r.alpha_gamma > 0.0f ? fmt_f(r.alpha_gamma)
                                                 : std::string("null"))
                        << "}";
                }
            }
        }
        if (!alpha_ok) oss << ",\"alpha\":{\"available\":false}";

        oss << "}";
    }
    oss << "]}";
    out_json = oss.str();
    return true;
}

// ---------------------------------------------------------------------------
// 上传三段（对齐 MMC：begin 建临时文件 → HTTP 层流式写 → finish 落位）
// ---------------------------------------------------------------------------
bool MpcdiService::upload_begin(const std::string& filename,
                                std::string& temp_path, std::string& err) {
    std::lock_guard<std::mutex> lock(mu_);
    if (cache_dir_.empty()) {
        err = "mpcdi cache not configured";
        return false;
    }
    auto tmp_dir = utf8_to_path(cache_dir_) / "tmp";
    std::error_code ec;
    fs::create_directories(tmp_dir, ec);
    auto name = sanitize_filename(filename);
    auto path =
        tmp_dir / (std::to_string(std::chrono::steady_clock::now()
                                      .time_since_epoch()
                                      .count()) +
                   "_" + name);
    temp_path = path_to_utf8(path);
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs) {
        err = "cannot create temp file";
        return false;
    }
    return true;
}

void MpcdiService::discard_upload_temp(const std::string& temp_path) const {
    if (temp_path.empty()) return;
    std::error_code ec;
    fs::remove(utf8_to_path(temp_path), ec);
}

bool MpcdiService::upload_finish(const std::string& temp_path,
                                 const std::string& filename,
                                 std::string& package_id, std::string& err) {
    std::lock_guard<std::mutex> lock(mu_);
    auto src = utf8_to_path(temp_path);
    if (!fs::exists(src)) {
        err = "temp upload missing";
        return false;
    }
    // [S2] zip 内容校验（round3）：垃圾字节在落位前拦截——原实现只看
    // 扩展名，垃圾包 200 覆盖 current 单槽后校验拖到 enable 才报错，
    // 既有可用包已被顶掉。此处用与 enable 同源的装载/解析链探测 temp
    // 文件：zip 魔数与结构 + mpcdi.xml 可解析，二者缺一即拒绝（temp
    // 保留由调用方 discard，current 不受影响）。
    {
        ms::MpcdiPackageData probe;
        ms::MpcdiInfo probe_info;
        std::string perr;
        if (!ms::load_mpcdi_package(temp_path, probe, perr) ||
            !ms::parse_mpcdi_xml(probe, probe_info, perr)) {
            err = "not a valid mpcdi package: " + perr;
            return false;
        }
    }
    package_id = make_package_id(filename);
    auto packages_root = utf8_to_path(cache_dir_) / "packages";
    auto dest_dir = utf8_to_path(package_dir(package_id));
    std::error_code ec;
    // 磁盘上只保一个 .mpcdi：清掉其它包槽，再替换 current（单槽语义）
    if (fs::exists(packages_root, ec)) {
        for (auto& e : fs::directory_iterator(packages_root, ec)) {
            if (!e.is_directory()) continue;
            if (e.path() == dest_dir) continue;
            fs::remove_all(e.path(), ec);
        }
    }
    fs::remove_all(dest_dir, ec);
    fs::create_directories(dest_dir, ec);
    auto safe = sanitize_filename(filename);
    if (!ci_ends_with(safe, ".mpcdi") && !ci_ends_with(safe, ".zip"))
        safe += ".mpcdi";
    auto dest = dest_dir / safe;
    ec.clear();
    fs::rename(src, dest, ec);
    if (ec) {
        fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
        fs::remove(src, ec);
        if (ec) {
            err = "failed to store package: " + ec.message();
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// enable/disable —— 构建 + 投递 + 会话落盘
// ---------------------------------------------------------------------------
bool MpcdiService::enable(const std::string& package_id,
                          const std::string& regions_csv,
                          const std::string& layout_json,
                          const std::string& options_json, std::string& err) {
    std::string pkg_path;
    ms::RenderThread* rt = nullptr;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (!rt_) {
            err = "render thread not ready";
            return false;
        }
        if (cache_dir_.empty()) {
            err = "mpcdi cache not configured";
            return false;
        }
        rt = rt_;
        pkg_path = package_file(package_id);
    }
    if (pkg_path.empty()) {
        err = "package not found: " + package_id;
        return false;
    }

    // ---- 1. 解析布局（对齐 web 上报的 layout 结构）----
    ms::MpcdiBuildOptions opt;
    opt.layout.mode = "grid";  // 默认：region 顺序自动网格
    std::string layout_norm =
        layout_json.empty() ? std::string("{\"mode\":\"grid\"}") : layout_json;
    if (!layout_json.empty()) {
        std::string mode;
        if (extract_json_value(layout_json, "mode", mode) && !mode.empty()) {
            if (mode.front() == '"')
                mode = mode.substr(1, mode.size() - 2);
            opt.layout.mode = mode;
        }
        double num = 0;
        if (extract_number_field(layout_json, "cols", num) && num > 0)
            opt.layout.cols = static_cast<int>(num);
        if (extract_number_field(layout_json, "rows", num) && num > 0)
            opt.layout.rows = static_cast<int>(num);
        std::string slots;
        if (extract_json_value(layout_json, "slots", slots) &&
            slots.find('[') != std::string::npos) {
            parse_string_array(slots, opt.layout.slots);
        }
    }

    // ---- 2. region 顺序：mosaic slots 优先（行优先去重），否则 csv ----
    if (!opt.layout.slots.empty()) {
        opt.regions.clear();
        for (const auto& s : opt.layout.slots) {
            if (s.empty()) continue;
            if (std::find(opt.regions.begin(), opt.regions.end(), s) !=
                opt.regions.end())
                continue;
            opt.regions.push_back(s);
        }
    } else {
        opt.regions.clear();
        parse_regions_csv(regions_csv, opt.regions);
    }
    if (opt.regions.empty()) {
        err = "no regions selected";
        return false;
    }

    // ---- 3. 解析渲染选项（键集对齐 v26 bake 语义）----
    if (!options_json.empty()) {
        bool b = false;
        if (extract_bool_field(options_json, "alpha_linearize", b))
            opt.alpha_linearize = b;
        if (extract_bool_field(options_json, "force_absolute", b))
            opt.force_absolute = b;
        if (extract_bool_field(options_json, "flip_y", b)) opt.flip_y = b;
        if (extract_bool_field(options_json, "flip_screen_y", b))
            opt.flip_screen_y = b;
        if (extract_bool_field(options_json, "region_local", b))
            opt.region_local = b;
        if (extract_bool_field(options_json, "blend_enabled", b))
            opt.blend_enabled = b;
        // bounds_overlay：解析保留但不消费（渲染端 mesh 边缘绿带未实现）
    }

    // ---- 4. 构建（zip 解包 + PFM 降采样 + 顶点网格总装；耗时，锁外）----
    ms::MpcdiGeometry geom;
    if (!ms::build_mpcdi_geometry(pkg_path, opt, geom, err)) return false;

    // ---- 5. 转 WarpSpec(mode=2) 投递（跨线程交接；渲染端内部去重）----
    auto ws = std::make_shared<const ms::WarpSpec>(ms::mpcdi_to_warp(geom));
    if (!ws->active()) {
        err = "built geometry inactive (empty mesh?)";
        return false;
    }
    rt->setMpcdiWarp(std::shared_ptr<const ms::WarpSpec>(ws));

    // ---- 6. 组装 manifest（output = 画布尺寸，前端状态栏显示用）----
    std::ostringstream man;
    man << "{\"package_id\":\"" << json_escape(package_id) << "\""
        << ",\"layout\":" << layout_norm
        << ",\"options\":"
        << (options_json.empty() ? "{}" : options_json)
        << ",\"output\":{\"width\":" << geom.cols * geom.cell_w
        << ",\"height\":" << geom.rows * geom.cell_h << "}}";
    const std::string manifest = man.str();

    // ---- 7. 落盘会话（重启自动恢复）----
    {
        std::lock_guard<std::mutex> lock(mu_);
        enabled_ = true;
        active_package_id_ = package_id;
        std::string csv;
        for (size_t i = 0; i < opt.regions.size(); ++i) {
            if (i) csv += ",";
            csv += opt.regions[i];
        }
        active_region_ = csv;
        active_layout_json_ = layout_norm;
        active_options_json_ =
            options_json.empty() ? std::string("{}") : options_json;
        last_manifest_json_ = manifest;
        {
            auto out_path = utf8_to_path(cache_dir_) / "active";
            std::error_code ec;
            fs::create_directories(out_path, ec);
            std::ofstream mfs(out_path / "manifest.json",
                              std::ios::binary | std::ios::trunc);
            mfs << manifest;
        }
        write_session_locked(true);
    }
    return true;
}

bool MpcdiService::disable(std::string& err) {
    // 先落盘关闭状态（避免 restore 误恢复），再撤几何。
    ms::RenderThread* rt = nullptr;
    {
        std::lock_guard<std::mutex> lock(mu_);
        rt = rt_;
        enabled_ = false;
        write_session_locked(false);
    }
    if (!rt) {
        err = "render thread not ready";
        return false;
    }
    rt->setMpcdiWarp(nullptr);   // 撤销 → 渲染端回落场景 screen warp
    return true;
}

}  // namespace media_api
