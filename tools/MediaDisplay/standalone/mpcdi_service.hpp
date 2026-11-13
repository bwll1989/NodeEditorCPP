#pragma once

// ============================================================================
// MpcdiService —— MPCDI 校准包存储 + 顶点网格构建 + 会话持久化
// （display×MMC 合并 M4 收编自 MMC media_api::MpcdiService；2026-11-11）
//
// 语义对齐 MMC（黄金参考）：
//   * 上传存储 / 包管理 / 会话落盘结构一致：单槽 "current" +
//     mpcdi_cache/active/{session.json,state.json,manifest.json}。
//   * enable() 本地构建：build_mpcdi_geometry 生成顶点网格 → mpcdi_to_warp
//     转 WarpSpec(mode=2) → RenderThread::setMpcdiWarp 投递（HTTP 线程 →
//     渲染线程锁内交接；渲染端内部比对去重）。
//   * 几何独立于媒体：换片 / 渲染器重建都不需要重新挂接（渲染线程每帧
//     仲裁注入，MPCDI active 优先于场景 screen 手动网格）。
//   * disable() 落盘关闭态后撤几何；restore_on_startup 重走 enable 的
//     构建路径（包文件仍在 cache/packages/current）。
//
// MMC 差异（有意为之）：
//   * 投递目标 PlayerFacade::set_output_geometry → RenderThread::
//     setMpcdiWarp（MediaServer 两段式呈现模型，region 拼接进单网格）。
//   * bounds_overlay 选项保留解析但不消费（渲染端 mesh 边缘绿带未实现，
//     第一阶段不做——选项原样透传进会话，后续接入零协议变更）。
// ============================================================================

#include <mutex>
#include <string>
#include <vector>

namespace ms {
class RenderThread;
}

namespace media_api {

class MpcdiService {
public:
    void set_paths(const std::string& config_dir);
    void set_render_thread(ms::RenderThread* rt);

    std::string status_json() const;
    std::string packages_json() const;

    /**
     * 包 region 目录（网页排布 UI 用）：
     * [{"id","key","display_id","buffer_id","xResolution","yResolution",
     *   "has_warp"}...]。失败返回 false 并填 err。
     */
    bool regions_json(const std::string& package_id, std::string& out_json,
                      std::string& err);

    /**
     * 包 metadata 面板数据（复刻 mpcdi-viewer METADATA 区）：
     * {profile, version, buffers:[...], regions:[{id,key,buffer_id,buffer,
     * width,height,rect,warp:{...},alpha:{...}}...]}。
     * 失败返回 false 并填 err。
     */
    bool metadata_json(const std::string& package_id, std::string& out_json,
                       std::string& err);

    bool upload_begin(const std::string& filename, std::string& temp_path,
                      std::string& err);
    bool upload_finish(const std::string& temp_path,
                       const std::string& filename,
                       std::string& package_id, std::string& err);
    /** 丢弃上传临时文件（超限/失败时调用，避免 tmp 泄漏）。 */
    void discard_upload_temp(const std::string& temp_path) const;

    /**
     * 启用变形融合：
     *   package_id   已上传包 id（当前实现为单槽 "current"）
     *   regions_csv  参与拼接的 region（逗号分隔 id/key）；layout.slots
     *                非空时以其为准（行优先去重）
     *   layout_json  {"mode":"grid|mosaic|...","cols":N,"rows":N,
     *                 "slots":["k1","","k2",...]}（可空）
     *   options_json {"alpha_linearize":bool,"force_absolute":bool,
     *                 "flip_y":bool,...}（可空；其余键原样保留进会话）
     *
     * 成功后 geometry 已投递渲染线程，session/manifest 落盘（重启恢复）。
     */
    bool enable(const std::string& package_id, const std::string& regions_csv,
                const std::string& layout_json,
                const std::string& options_json, std::string& err);
    bool disable(std::string& err);

    /** 启动时按 session 恢复（重走 enable 的构建路径；包文件仍在缓存）。 */
    bool restore_on_startup(std::string& err);

private:
    std::string package_dir(const std::string& id) const;
    std::string package_file(const std::string& id) const;
    std::string make_package_id(const std::string& filename) const;
    void reload_active_locked();
    void write_session_locked(bool enabled);

    mutable std::mutex mu_;
    std::string config_dir_;
    std::string cache_dir_;
    ms::RenderThread* rt_ = nullptr;

    bool enabled_ = false;
    std::string active_package_id_;
    std::string active_region_;       // 逗号连接的 region 列表（兼容 v26 字段名）
    std::string active_layout_json_;  // 最后一次 enable 的 layout（原样 JSON）
    std::string active_options_json_; // 最后一次 enable 的 options（原样 JSON）
    std::string last_manifest_json_;
    std::string last_session_json_;
};

}  // namespace ms
