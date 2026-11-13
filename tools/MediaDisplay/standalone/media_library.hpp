#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace media_api {

/** 媒体库中的一条文件记录。 */
struct MediaItem {
    std::string id;
    std::string name;
    std::string path;
    long long size = 0;
    long long mtime = 0;  // unix 秒，文件 last_write_time
    long long duration_us = 0;  // 容器时长（视频/音频，探测失败=0；图片恒 0）
    std::string kind;     // "video" | "image" | "audio"
};

struct UploadWriter {
    std::ofstream ofs;
    std::string desired_name;
    std::string temp_path;
    long long size = 0;
    bool open = false;
};

class MediaLibrary {
public:
    void set_root(const std::string& root);
    const std::string& root() const { return root_; }

    void refresh();
    size_t count() const;

    bool find(const std::string& id, MediaItem& out) const;
    bool remove(const std::string& id, std::string& err);

    bool begin_upload(const std::string& filename, UploadWriter& writer, std::string& err);
    bool write_upload_chunk(UploadWriter& writer, const char* data, size_t len, size_t max_bytes,
                            std::string& err);
    bool finish_upload(UploadWriter& writer, MediaItem& out, std::string& err);
    void abort_upload(UploadWriter& writer);

    std::string list_json(const std::string& query) const;
    std::string item_json(const MediaItem& item) const;

    static std::string detect_kind(const std::string& name);
    static bool is_allowed_media(const std::string& name);
    static std::string sanitize_filename(const std::string& name);

private:
    void scan_dir(const std::string& abs_dir, const std::string& rel_prefix);
    std::string unique_name(const std::string& desired) const;

    std::string root_;
    std::vector<MediaItem> items_;
    long long scanned_at_ = 0;
    mutable std::mutex mu_;
};

}  // namespace media_api
