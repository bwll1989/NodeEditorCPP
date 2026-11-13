// ============================================================================
//  standalone_api —— standalone 控制面 REST API（display×MMC 合并 M2）
//
//  定位（方案 §4.2/§4.4，2026-11-11）：NodeController/注册表/装配器的
//  HTTP 表面。路由按时间轴/场景语义设计（v2.2 定案）：
//      GET    /api/v1/status                     —— 模式/活跃/播放头/列表概要
//      GET    /api/v1/settings                   —— 设置读（两模式均可）
//      PUT    /api/v1/settings                   —— 设置写（config.ini 落盘，
//                                                  端口/模式类需重启生效）
//      GET    /api/v1/timelines                  —— 时间轴清单（仅独立模式）
//      POST   /api/v1/timelines                  —— 创建（body=时间轴文档）
//      GET    /api/v1/timelines/{id}             —— 读文档
//      PUT    /api/v1/timelines/{id}             —— 更新（整体替换重装配）
//      DELETE /api/v1/timelines/{id}             —— 删除（活跃先停）
//      POST   /api/v1/timelines/{id}/play        —— 启动（唯一活跃仲裁）
//      POST   /api/v1/timelines/{id}/set-default —— 开机默认（config.ini 落盘）
//      POST   /api/v1/playback/pause|resume|seek|stop —— 播放头控制
//      GET    /api/v1/library(?q=)               —— 媒体库（assets_root 扫描）
//      POST   /api/v1/library/refresh            —— 重扫
//      POST   /api/v1/library/upload             —— 上传（multipart/raw 两路）
//      GET/DELETE /api/v1/library/{id}           —— 素材详情/删除
//  模式门禁（2026-11-11 定案）：timelines/playback/library 三组仅在
//  standalone 模式开放；sync 模式下命中返回 409（网页编排台同步收敛）。
//
//  时间轴文档（编辑态真相，前端直接编辑的原样结构）：
//      { "name":..., "loop":bool, "stop_position_us":int,
//        "scene": { canvas_w, canvas_h, tracks:[{ name, clips:[{
//          file, text, start_us, duration_us, in_us,
//          x,y,w,h(画布 px 绝对值), opacity, z(轨内层级 0~999),
//          rotation(度), crop_l/r/t/b(px 向内收), fade_in_us, fade_out_us }] }] } }
//    播放时实时 SceneAssembler::assemble 成 Scene（运行态产物不落盘）；
//    文档落盘 <timelines_dir>/<id>.json（id=文件名，稳定标识）。
//    .show 工程导入（ini timeline.N.*）走 main 的 fillStandaloneRegistry，
//    两路汇合同一注册表（M5 工程互通的雏形）。
//
//  线程模型：Poco HTTP 工作线程并发进 handle()——registry/library 各持
//  自锁；文档文件与 config 落盘经本类一把 io_mutex_ 串行化；NodeController
//  的出点参数快照在 core 层加锁（见 NodeController M2 注释）。
// ============================================================================

#pragma once

#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/URI.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "standalone/SceneAssembler.hpp"
#include "media_library.hpp"

// ---- 前置声明（头文件卫生：构造/成员只持引用，无需完整类型）----------------
// PlayerRuntime/TimelineRegistry/NodeController 完整定义只在 .cpp 引入——
// 本头文件可被任意翻译单元独立包含（main.cpp/http_server.cpp/单测同源）。
namespace ms {
class PlayerRuntime;
namespace standalone {
class TimelineRegistry;
class NodeController;
}  // namespace standalone
}  // namespace ms

namespace media_api {

class MpcdiService;   // M4：MPCDI 校准包服务（Config 指针；完整类型在 cpp）

// ---------------------------------------------------------------------------
// 请求上下文（http_server 分发用；路由函数按需取用）
// ---------------------------------------------------------------------------
struct RouteCtx {
    class StandaloneApi& api;
    const std::string& method;
    const std::string& path;
    const Poco::URI&   uri;
    Poco::Net::HTTPServerRequest&  req;
    Poco::Net::HTTPServerResponse& resp;
};

// ---------------------------------------------------------------------------
// API 宿主
// ---------------------------------------------------------------------------
class StandaloneApi {
public:
    struct Config {
        std::string assets_root;          ///< 素材根（媒体库扫描 + 上传落盘）
        std::string web_dir;              ///< 静态网页目录（http_server 托管）
        std::string timelines_dir;        ///< 时间轴文档目录（<id>.json）
        std::string config_path;          ///< config.ini 路径（set-default 落盘）
        std::size_t max_upload_mb = 2048; ///< 上传上限（0 = 不限）
        /// 节点运行模式（2026-11-11 手动切换定案）：standalone = 全功能
        /// 控制面（编排/媒体库/播放/设置）；sync = 仅 状态/设置/静态页——
        /// 编排面路由返回 409（门禁在 handle 入口，网页编排台随之收敛）
        std::string mode = "standalone";
        /// MPCDI 服务（M4；可空 = 未装配，mpcdi 路由组不注册）。
        /// 门禁语义：MPCDI 是 display 本机输出校准能力——**两模式均开放**
        ///（sync 受控下 display 仍有自己的输出校准，与编排面互不相干）。
        MpcdiService* mpcdi = nullptr;
        /// 进程重启回调（M5 /api/v1/system/restart；空 = 路由 503）。
        /// main 注入"置退出标志 + 优雅退出链后拉起新实例"实现（接力模型，
        /// 见 main.cpp restart_requested）——回调自身须立即返回（HTTP 工作
        /// 线程上下文，响应尚未 flush，禁止就地退出进程）。
        std::function<void()> restart_fn;
    };

    StandaloneApi(ms::PlayerRuntime& player,
                  ms::standalone::TimelineRegistry& registry,
                  ms::standalone::NodeController& node,
                  ms::standalone::SceneAssembler& assembler,
                  std::string player_id, Config cfg);

    /// http_server 静态托管取目录
    const std::string& webDir() const { return cfg_.web_dir; }

    /// 媒体库（测试/诊断直调刷新用；路由内只读访问）
    MediaLibrary& library() { return library_; }

    /// API 总入口：命中请求则已发送响应并返回 true；未命中返回 false（404）
    bool handle(RouteCtx& ctx);

    // ---- 时间轴文档（公开：main 开机装载 / 单测复用同一解析器）--------------
    struct TimelineDoc {
        std::string name;
        bool loop = false;
        std::int64_t stop_position_us = 0;
        ms::standalone::SceneSpec scene;
        /// 乐观锁版本号（W1，2026-11-11）：创建/导入时 0，每次成功 PUT
        /// 自增；PUT 请求体携带的 rev 与磁盘现行值不等 → 409。旧文档无
        /// 该字段（解析缺省 0），提交方也按 0 处理 → 双 0 放行（兼容）。
        std::int64_t rev = 0;
    };

    /// 文档 JSON 文本 → 结构（Poco JSON DOM；失败 err 说明）
    static bool parseDoc(const std::string& body, TimelineDoc& doc, std::string& err);
    /// 结构 → 规范 JSON 文本（Poco 序列化，往返幂等）
    static std::string docToJson(const std::string& id, const TimelineDoc& doc);
    /// .show 场景 → 网页时间轴文档（M5 工程互通导入方向；SceneLoader 校验
    /// 过的 Scene 做逆向映射）。非致命差异（Live/未知素材、自定义 tween
    /// 动画）跳过并追加 note；track 次序即轨序、轨内 clip 按 transform.z
    /// 稳定升序重排（z%1000 还原"轨内层级"文档语义）。
    static bool sceneToDoc(const ms::Scene& scene, TimelineDoc& doc,
                           std::string& note);
    /// [S2] 混合轨道拆分 + z 层级重编（.show 导出适配；静态——单测同源）。
    /// 网页 MVP 轨道是混合轨（装配端 kind=Unknown），.show R2 校验要求
    /// 轨道 kind 与 clip 素材类型一致——导出侧按素材类型拆子轨。拆分会
    /// 改变轨序，若沿用原 z（装配端全局合成产物）一次导入回环即层级漂移；
    /// 此处统一重编：子轨组按组内最大原 z 降序排（最顶层组排最前——装配
    /// 端 base=(N-1-ti)*1000，ti 小层高），组内按原 z 稳定升序重编 rank
    ///（0..999 封顶）。非交错场景完全保序；交错场景按 kind 块保序
    ///（track-major 语义极限）；一次回环即收敛不动点。
    static void splitTracksForExport(ms::Scene& scene);

    /// 开机默认落盘键（main 启动时装载与网页 set-default 共用）
    static constexpr const char* kDefaultKey = "default_timeline";

    /// timeline id 校验（S2-1 路径穿越防线，2026-11-11）：id 即文档文件名
    ///（<timelines_dir>/<id>.json），客户端可控——必须拒绝路径成分。
    /// 规则：非空且 ≤128 字节；拒绝路径分隔符 / \ 与盘符冒号（穿越与
    /// 绝对路径一并否决，不做 basename 化改写——id 是稳定标识，静默改名
    /// 会破坏 id↔文件一致性）；拒绝 ".." 分量（id==".." 及含 ".."）；拒绝
    /// Windows 文件名非法字符 < > : " | ? * 与控制字符。其余字符（含中文）
    /// 放行——兼容 timelines_dir 既有中文文件名文档（loadTimelineDocs 以
    /// 文件名 stem 为 id）。失败时 err 给出原因。
    static bool validTimelineId(const std::string& id, std::string& err);

private:
    /// 时间轴文档磁盘路径（timelines_dir/<id>.json）。
    /// [S1] 目录分量与文件名分量都必须经 utf8_to_path：id 是 UTF-8 字节
    /// 串，fs::path 窄字符串构造在 Windows 按 ACP(GBK) 解码——中文 id 落盘
    /// 成 mojibake NTFS 名，与装载链（u8string 语义）错位 → 重启后 id
    /// 乱码化（round3 S1 根因）。此处统一收口，杜绝新调用点再犯。
    std::filesystem::path timelineDocPath(const std::string& id) const;
    /// 磁盘装载（timelines_dir/<id>.json；不存在返回 false）
    bool loadDoc(const std::string& id, TimelineDoc& doc, std::string& err) const;
    /// 锁内磁盘装载（调用方必须已持 io_mutex_——PUT 乐观锁"读现行 rev →
    /// 比较 → 写回"复合段用，经外层包装 loadDoc 会重复加锁自死锁）
    bool loadDocLocked(const std::string& id, TimelineDoc& doc, std::string& err) const;
    /// 磁盘保存（原子性不做——文档量小，重写即落；并发经 io_mutex_ 串行）
    bool saveDoc(const std::string& id, const TimelineDoc& doc, std::string& err) const;
    /// 锁内实现（调用方必须已持 io_mutex_——创建分支"登记+落盘"复合原子用，
    /// 经外层包装 saveDoc 会重复加锁自死锁）
    bool saveDocLocked(const std::string& id, const TimelineDoc& doc, std::string& err) const;
    bool removeDoc(const std::string& id, std::string& err) const;
    /// [S2] id 大小写折叠查重（Windows NTFS 不区分大小写，"ABC.json" 与
    /// "abc.json" 是同一文件）：对 timelines_dir 做文件系统级折叠比对，
    /// 命中时 hit 返回磁盘上既有的实际 id（stem）。查文件系统而非注册表，
    /// 可覆盖注册表之外的既有文档（含解析失败未登记者）。非 Windows
    /// 恒返回 false（ext4 等区分大小写，无冲突）。
    bool timelineIdCasefoldExists(const std::string& id, std::string& hit) const;
    /// 装配（文档.scene → Scene 快照；err 含被跳过片段说明）
    std::shared_ptr<const ms::Scene> assembleDoc(const TimelineDoc& doc, std::string& err) const;

    // ---- 路由组（命中返回 true）---------------------------------------------
    bool routeStatus(RouteCtx& ctx);
    bool routeSettings(RouteCtx& ctx);   ///< 设置读/写（两模式均可用）
    bool routeMpcdi(RouteCtx& ctx);      ///< MPCDI 校准包（M4；两模式均可用）
    bool routeSystem(RouteCtx& ctx);     ///< 进程重启（M5；两模式均可用）
    bool routeTimelines(RouteCtx& ctx);
    bool routePlayback(RouteCtx& ctx);
    bool routeLibrary(RouteCtx& ctx);

    // ---- 媒体库（收编 MMC MediaLibrary；assets_root 同域）--------------------
    void handleUpload(RouteCtx& ctx);
    /// MPCDI 包上传（multipart/raw 两路；三段式 → MpcdiService）
    void handleMpcdiUpload(RouteCtx& ctx);

    ms::PlayerRuntime& player_;
    ms::standalone::TimelineRegistry& registry_;
    ms::standalone::NodeController&   node_;
    ms::standalone::SceneAssembler&   assembler_;
    std::string player_id_;   ///< 本机生效 id（装配产物 Screen.player_id）
    Config cfg_;

    /// 媒体库（root=assets_root；HTTP 工作线程并发安全——内部持锁）
    MediaLibrary library_;

    /// 文档文件与 config.ini 落盘串行化（HTTP 工作线程并发保护）
    mutable std::mutex io_mutex_;
};

}  // namespace media_api
