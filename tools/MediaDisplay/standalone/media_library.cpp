#include "media_library.hpp"

#include "json_util.hpp"
#include "media/FfmpegMediaSource.hpp"
#include "path_utf8.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>

namespace fs = std::filesystem;

namespace media_api {

void MediaLibrary::set_root(const std::string& root) {
    std::lock_guard<std::mutex> lock(mu_);
    root_ = root;
    std::error_code ec;
    fs::create_directories(utf8_to_path(root_), ec);
}

void MediaLibrary::scan_dir(const std::string& abs_dir, const std::string& rel_prefix) {
    std::error_code ec;
    const fs::path dir = utf8_to_path(abs_dir);
    for (auto it = fs::directory_iterator(dir, ec); !ec && it != fs::directory_iterator(); it.increment(ec)) {
        const auto& entry = *it;
        const auto name = path_to_utf8(entry.path().filename());
        if (name.empty() || name[0] == '.') continue;
        if (entry.is_directory(ec)) {
            const std::string child_rel = rel_prefix.empty() ? name : (rel_prefix + "/" + name);
            scan_dir(path_to_utf8(entry.path()), child_rel);
            continue;
        }
        if (!entry.is_regular_file(ec)) continue;
        if (!is_allowed_media(name)) continue;

        MediaItem item;
        item.name = name;
        item.id = rel_prefix.empty() ? name : (rel_prefix + "/" + name);
        std::replace(item.id.begin(), item.id.end(), '\\', '/');
        item.path = path_to_utf8(entry.path());
        item.size = static_cast<long long>(entry.file_size(ec));
        {
            // 使用文件真实 mtime（C++17：用 clock 差值映射到 system_clock）。
            auto ft = entry.last_write_time(ec);
            if (!ec) {
                const auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now() +
                        (ft - fs::file_time_type::clock::now()));
                item.mtime = static_cast<long long>(sctp.time_since_epoch().count());
            } else {
                item.mtime = now_unix();
            }
        }
        item.kind = detect_kind(name);
        // 容器时长探测（视频/音频；图片恒 0）：网页"点击素材追加片段"直接
        // 以文件长度建 clip（需求：添加文件后以文件长度为准）。探测只读容器
        // 头（avformat_open_input + find_stream_info，不解码），失败静默 0，
        // 由 SceneAssembler 装配期兜底（文档=编辑态真相不受影响）。
        item.duration_us = (item.kind == "image")
            ? 0
            : std::max<long long>(0, ms::FfmpegMediaSource::probeDurationUs(item.path));
        items_.push_back(std::move(item));
    }
}

void MediaLibrary::refresh() {
    std::lock_guard<std::mutex> lock(mu_);
    items_.clear();
    if (root_.empty()) {
        scanned_at_ = now_unix();
        return;
    }
    std::error_code ec;
    fs::create_directories(utf8_to_path(root_), ec);
    scan_dir(root_, "");
    std::sort(items_.begin(), items_.end(), [](const MediaItem& a, const MediaItem& b) {
        return a.id < b.id;
    });
    scanned_at_ = now_unix();
}

bool MediaLibrary::find(const std::string& id, MediaItem& out) const {
    std::lock_guard<std::mutex> lock(mu_);
    for (const auto& item : items_) {
        if (item.id == id) {
            out = item;
            return true;
        }
    }
    return false;
}

bool MediaLibrary::remove(const std::string& id, std::string& err) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = std::find_if(items_.begin(), items_.end(), [&](const MediaItem& m) { return m.id == id; });
    if (it == items_.end()) {
        err = "media not found";
        return false;
    }
    std::error_code ec;
    fs::path root_abs = fs::weakly_canonical(utf8_to_path(root_), ec);
    if (ec) root_abs = fs::absolute(utf8_to_path(root_), ec);
    fs::path file_abs = fs::weakly_canonical(utf8_to_path(it->path), ec);
    if (ec) file_abs = fs::absolute(utf8_to_path(it->path), ec);
    auto root_s = to_lower(path_to_utf8(root_abs));
    auto file_s = to_lower(path_to_utf8(file_abs));
    std::replace(root_s.begin(), root_s.end(), '\\', '/');
    std::replace(file_s.begin(), file_s.end(), '\\', '/');
    while (!root_s.empty() && root_s.back() == '/') root_s.pop_back();
    if (file_s != root_s && file_s.rfind(root_s + "/", 0) != 0) {
        err = "path denied";
        return false;
    }
    ec.clear();
    if (!fs::remove(file_abs, ec)) {
        err = "failed to delete file";
        return false;
    }
    items_.erase(it);
    return true;
}

std::string MediaLibrary::unique_name(const std::string& desired) const {
    if (std::none_of(items_.begin(), items_.end(), [&](const MediaItem& m) { return m.name == desired; })) {
        return desired;
    }
    auto dot = desired.find_last_of('.');
    std::string stem = dot == std::string::npos ? desired : desired.substr(0, dot);
    std::string ext = dot == std::string::npos ? "" : desired.substr(dot);
    for (int i = 1; i < 10000; ++i) {
        std::string cand = stem + "_" + std::to_string(i) + ext;
        if (std::none_of(items_.begin(), items_.end(), [&](const MediaItem& m) { return m.name == cand; })) {
            return cand;
        }
    }
    return stem + "_dup" + ext;
}

bool MediaLibrary::begin_upload(const std::string& filename, UploadWriter& writer, std::string& err) {
    abort_upload(writer);
    std::string clean = sanitize_filename(filename);
    if (clean.empty() || !is_allowed_media(clean)) {
        err = "unsupported or invalid filename";
        return false;
    }
    if (clean.find("..") != std::string::npos || clean.find('/') != std::string::npos ||
        clean.find('\\') != std::string::npos) {
        err = "path denied";
        return false;
    }

    std::lock_guard<std::mutex> lock(mu_);
    std::error_code ec;
    fs::create_directories(utf8_to_path(root_), ec);

    const auto stamp = now_unix();
    const std::string tmp_name = ".upload_" + std::to_string(stamp) + "_" +
                                 std::to_string(reinterpret_cast<uintptr_t>(&writer)) + ".part";
    fs::path tmp = utf8_to_path(root_) / utf8_to_path(tmp_name);
    writer.desired_name = clean;
    writer.temp_path = path_to_utf8(tmp);
    writer.size = 0;
#ifdef _WIN32
    writer.ofs.open(tmp, std::ios::binary | std::ios::trunc);
#else
    writer.ofs.open(writer.temp_path, std::ios::binary | std::ios::trunc);
#endif
    if (!writer.ofs) {
        err = "failed to write file";
        writer.temp_path.clear();
        return false;
    }
    writer.open = true;
    return true;
}

bool MediaLibrary::write_upload_chunk(UploadWriter& writer, const char* data, size_t len,
                                      size_t max_bytes, std::string& err) {
    if (!writer.open || !writer.ofs) {
        err = "upload not started";
        return false;
    }
    if (len == 0) return true;
    // max_bytes == SIZE_MAX means unlimited
    if (max_bytes != (std::numeric_limits<size_t>::max)() &&
        static_cast<size_t>(writer.size) + len > max_bytes) {
        err = "upload too large";
        return false;
    }
    writer.ofs.write(data, static_cast<std::streamsize>(len));
    if (!writer.ofs) {
        err = "failed to write file";
        return false;
    }
    writer.size += static_cast<long long>(len);
    return true;
}

bool MediaLibrary::finish_upload(UploadWriter& writer, MediaItem& out, std::string& err) {
    if (!writer.open) {
        err = "upload not started";
        return false;
    }
    writer.ofs.flush();
    writer.ofs.close();
    writer.open = false;

    if (writer.size <= 0) {
        abort_upload(writer);
        err = "empty upload";
        return false;
    }

    std::lock_guard<std::mutex> lock(mu_);
    std::string final_name = unique_name(writer.desired_name);
    fs::path dest = utf8_to_path(root_) / utf8_to_path(final_name);
    fs::path tmp = utf8_to_path(writer.temp_path);
    std::error_code ec;
    fs::rename(tmp, dest, ec);
    if (ec) {
        // Cross-device fallback: copy then remove.
        ec.clear();
        fs::copy_file(tmp, dest, fs::copy_options::overwrite_existing, ec);
        fs::remove(tmp);
        if (ec) {
            err = "failed to finalize upload";
            abort_upload(writer);
            return false;
        }
    }
    writer.temp_path.clear();

    MediaItem item;
    item.id = final_name;
    item.name = final_name;
    item.path = path_to_utf8(dest);
    item.size = writer.size;
    item.mtime = now_unix();
    item.kind = detect_kind(final_name);
    item.duration_us = (item.kind == "image")
        ? 0
        : std::max<long long>(0, ms::FfmpegMediaSource::probeDurationUs(item.path));
    items_.push_back(item);
    std::sort(items_.begin(), items_.end(), [](const MediaItem& a, const MediaItem& b) { return a.id < b.id; });
    out = item;
    writer = UploadWriter{};
    return true;
}

void MediaLibrary::abort_upload(UploadWriter& writer) {
    if (writer.open && writer.ofs) {
        writer.ofs.close();
    }
    writer.open = false;
    if (!writer.temp_path.empty()) {
        std::error_code ec;
        fs::remove(utf8_to_path(writer.temp_path), ec);
        writer.temp_path.clear();
    }
    writer.size = 0;
    writer.desired_name.clear();
}

std::string MediaLibrary::item_json(const MediaItem& item) const {
    std::ostringstream oss;
    oss << "{\"id\":\"" << json_escape(item.id) << "\","
        << "\"name\":\"" << json_escape(item.name) << "\","
        << "\"path\":\"" << json_escape(item.path) << "\","
        << "\"size\":" << item.size << ","
        << "\"mtime\":" << item.mtime << ","
        << "\"duration_us\":" << item.duration_us << ","
        << "\"kind\":\"" << json_escape(item.kind) << "\"}";
    return oss.str();
}

std::string MediaLibrary::list_json(const std::string& query) const {
    std::lock_guard<std::mutex> lock(mu_);
    std::string q = to_lower(query);
    std::ostringstream items;
    items << "[";
    bool first = true;
    int count = 0;
    for (const auto& item : items_) {
        if (!q.empty()) {
            if (to_lower(item.id).find(q) == std::string::npos &&
                to_lower(item.name).find(q) == std::string::npos) {
                continue;
            }
        }
        if (!first) items << ",";
        first = false;
        items << item_json(item);
        ++count;
    }
    items << "]";

    std::ostringstream oss;
    oss << "{\"root\":\"" << json_escape(root_) << "\","
        << "\"scanned_at\":" << scanned_at_ << ","
        << "\"count\":" << count << ","
        << "\"items\":" << items.str() << "}";
    return oss.str();
}

size_t MediaLibrary::count() const {
    std::lock_guard<std::mutex> lock(mu_);
    return items_.size();
}

std::string MediaLibrary::detect_kind(const std::string& name) {
    static const char* images[] = {".jpg", ".jpeg", ".png", ".webp", ".bmp", ".gif",
                                   ".tif", ".tiff", ".jxl", ".avif"};
    for (auto ext : images) {
        if (ends_with_ci(name, ext)) return "image";
    }
    // T7 补测：音频文件走 kind="audio"（原版只有 video/image，纯音频进不了库）
    static const char* audios[] = {".mp3", ".wav", ".flac", ".aac", ".m4a",
                                   ".ogg", ".opus", ".wma"};
    for (auto ext : audios) {
        if (ends_with_ci(name, ext)) return "audio";
    }
    return "video";
}

bool MediaLibrary::is_allowed_media(const std::string& name) {
    static const char* exts[] = {".mp4",  ".mkv",  ".webm", ".avi",  ".mov",  ".m4v",  ".ts",
                                 ".m2ts", ".wmv",  ".flv",  ".jpg",  ".jpeg", ".png",  ".webp",
                                 ".bmp",  ".gif",  ".tif",  ".tiff", ".jxl",  ".avif",
                                 // T7 补测：音频扩展名入库（内核本身支持纯音频播放）
                                 ".mp3",  ".wav",  ".flac", ".aac",  ".m4a",  ".ogg",
                                 ".opus", ".wma"};
    for (auto ext : exts) {
        if (ends_with_ci(name, ext)) return true;
    }
    return false;
}

std::string MediaLibrary::sanitize_filename(const std::string& name) {
    std::string base = basename_of(name);
    std::string out;
    for (unsigned char c : base) {
        if (c < 32) continue;
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '\"' || c == '<' || c == '>' ||
            c == '|') {
            out.push_back('_');
        } else {
            out.push_back(static_cast<char>(c));
        }
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '.')) out.pop_back();
    return out;
}

}  // namespace media_api
