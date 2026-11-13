// ============================================================================
//  display/main.cpp —— 显示端主程序入口（media_display.exe）
//
//  角色模型（设计文档 §2.1 + 第五迭代"显示端极简"改造）：
//    Player = 引擎 SLAVE 角色：监听端口等待 Editor 接入 → 上报身份 →
//             接收 scene/补丁/命令 → 对齐时钟 → 渲染自己的 screen →
//             UDP 上报（250ms）。
//    【零操作】显示端不配置任何 Editor 地址——打开即监听（默认 17100），
//    Editor 端按项目登记（.show 的 DisplayEntry）主动连接并同步数据
//    （WATCHOUT 模型）。引擎核心与 Editor 同源（§2.1）：两端链接同一
//    engine_core 静态库，但本程序为独立 exe（无 Qt 依赖）。
//
//  用法：
//    media_display [--config=config.ini]（启动设置见 config.display.example.ini）
//                  [--id=player-1] [--port=17100]
//                  [--assets-root=<dir>]  （素材落盘目录，收场景后自动拉取）
//                  [--render-fps=60] [--render-out=<dir>]
//                  [--quit-after-ms=<ms>]（联测脚本用；0=常驻）
//    【--editor 已废弃】显示端不再主动连接；Editor 端"添加显示端"接入。
//
//  配置优先级（"所有设置不硬编码"）：命令行显式项 > config.ini > 内置默认。
//    config.ini 查找：--config= 显式 > exe 同目录 > 工作目录；段 [display]，
//    通用项（assets_root）可写 [common]，[display] 同键覆盖 [common]。
// ============================================================================

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <process.h> // _getpid（player_id 缺省生成）
#else
#include <unistd.h>  // getpid（POSIX 对应）
#endif

#include "clock/ClockProvider.hpp"
#include "config/IniConfig.hpp"
#include "loader/ShowFileIO.hpp"   // .show 工程文件（M1 时间轴载体——
                                   // 与 editor 工程互通，方案 §4.4）
#include "platform/ConsoleUtf8.hpp" // 控制台 UTF-8 输出（中文日志不乱码）
#include "platform/CrashDump.hpp"   // 崩溃现场捕获（两端共用；见 runDisplay 注释）
#include "runtime/PlayerRuntime.hpp"
#include "standalone/NodeController.hpp"
#include "standalone/SceneAssembler.hpp"
#include "standalone/TimelineRegistry.hpp"
#include "standalone/standalone_api.hpp"  // M2 控制面 REST API（时间轴文档）
#include "standalone/mpcdi_service.hpp"   // M4 MPCDI 校准包（上传/enable/会话）
#include "standalone/http_server.hpp"     // M2 HTTP 骨架（收编 MMC）

#if defined(_WIN32)
// M5 自重启接力（CreateProcessW/GetCommandLineW/WaitForSingleObject）：
// NOMINMAX 防 min/max 宏污染（工程坑——std::max 调用点会被宏打碎）
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "GlContextWGL.hpp"       // 关窗即退出旗标（GlContextWGL::userClosed）
#include "GlRenderer.hpp"         // GL 合成后端（第②阶段渲染管线）
#include "PreviewWindowWin32.hpp" // Win32 GDI 输出预览窗口（soft 后端用）
#endif

// ---------------------------------------------------------------------------
// NVIDIA Optimus 显卡路由提示（③阶段 CUDA 0 拷贝的前提）
//   双显卡笔记本默认 GL 落在省电核显——CUDA-GL interop 要求 GL 上下文与
//   CUDA 上下文同 GPU，GL 在核显上时 0 拷贝必然探测失败。驱动在进程启动
//   时读取本导出符号（非 0 = 请求性能 GPU 渲染），GL 上下文即落在 N 卡。
//   无 N 卡/无独显的机器上该导出无副作用（驱动忽略）。
//   【必须是 exe 的全局导出符号，且在驱动初始化（首个 GL 调用）前存在】
// ---------------------------------------------------------------------------
#if defined(_WIN32)
extern "C" {
__declspec(dllexport) int NvOptimusEnablement = 0x00000001;
}
#endif

namespace {

// ---------------------------------------------------------------------------
// 启动参数（display 端专用子集——编辑器参数见 src/editor/main.cpp）
//   默认值由 config.ini 预填（loadDefaultsFromIni），命令行显式项覆盖。
// ---------------------------------------------------------------------------
struct Args {
    std::string player_id;                  ///< --id=（Player 标识；缺省自动编号）
    long listen_port = 17100;               ///< --port=：监听端口（等待 Editor）
    std::string assets_root;                ///< --assets-root=：素材落盘目录
                                            ///<   （空=不启用素材拉取）
    long asset_workers = 3;                 ///< --asset-workers=：素材并发
                                            ///<   拉取 worker 数（1=串行旧行为）
    long render_fps = 60;                   ///< --render-fps=：渲染线程目标帧率
                                            ///<   （0=关闭渲染线程）
    std::string render_out;                 ///< --render-out=：诊断帧导出目录
                                            ///<   （渲染画面周期存 BMP，联测核对）
    long report_interval_ms = 250;          ///< [display] UDP 上报节流周期
    long diag_interval_ms = 5000;           ///< [display] 同步诊断日志周期
                                            ///<   （0=关闭——日常使用静默，
                                            ///<   联测保持默认或调小采样）
    long discovery_port = 17610;            ///< [display] 局域网发现应答端口
                                            ///<   （editor 扫描添加显示端；0=禁用）
    long screen = -1;                       ///< [display] 预览窗口定位到第 N 块
                                            ///<   显示器（0=第一块；-1=系统默认；
                                            ///<   多实例各占一屏互不遮挡）
    bool fullscreen = false;                ///< [display] 预览窗口无边框铺满
                                            ///<   目标屏（配合 screen）
    long window_x = -1;                     ///< [display] 窗口位置 X（fullscreen
                                            ///<   =false 时生效；<0=屏内居中。
                                            ///<   有 screen 时为屏内偏移）
    long window_y = -1;                     ///< [display] 窗口位置 Y（同上）
    long window_w = 0;                      ///< [display] 窗口客户区宽（<=0=
                                            ///<   默认 960）
    long window_h = 0;                      ///< [display] 窗口客户区高（<=0=
                                            ///<   默认 540）
    long quit_after_ms = 0;                 ///< --quit-after-ms=：运行 N ms 后
                                            ///<   优雅退出（联测脚本用；0=常驻）
    std::string pairing_code;               ///< [display] 配对码（空=不校验；
                                            ///<   非空=只接受携带同码 MsgAuth
                                            ///<   的 Editor 连接，其余踢除）
    /// --no-preview：关闭输出预览窗口（服务/联测部署免 GUI；仅 Windows 有效
    /// ——非 Windows 本就 headless）。默认开预览（用户反馈：只有终端没画面）
    bool no_preview = false;
    /// [display] hw_accel：视频硬解后端（d3d11va/cuda/nvdec/vaapi/mediacodec/
    /// auto/soft）。【默认 cuda（2026-11-05 修订）】render_backend=gl 下
    /// cuda 走 CUDA-GL interop 0 拷贝（实测 1080p60 满帧）；d3d11va 只能
    /// 走硬解下载路径（GPU→CPU 读回 8.8ms/帧，比软解 4.7ms 还慢 1.9×，
    /// ③阶段基准实证）——用户实测 d3d11va 机器 H.264 卡顿、editor 预览
    /// （默认 cuda）流畅即此差异。无 N 卡机器 cuda 探测失败运行时自动
    /// 回退软解（多线程提速），不影响可用性。
    std::string hw_accel = "cuda";
    /// [display] audio_enable：音频输出开关（P3，2026-11-10）。默认开
    /// （display 出声是音频链路的交付目标；无声卡/独占占用时自动回退
    /// 静音输出，不影响视频链路）。--no-audio / ini audio_enable=0 关闭。
    bool audio_enable = true;
    /// [display] audio_device：音频设备名（空 = 系统默认播放设备；
    /// FriendlyName 包含匹配，大小写不敏感）
    std::string audio_device;
    /// [display] render_backend：画面合成后端（gl/soft，第②阶段 GL 管线）。
    /// gl = GPU 合成（shader 采样 + 双线性，硬解帧经 GPU 上屏）；soft =
    /// 旧版 CPU 画布 + GDI 预览窗。默认 gl（GL 初始化失败自动告警，
    /// 可 --render-backend=soft 显式回退）。
    std::string render_backend = "gl";
    /// [display] gl_single_buffer：【已废弃 2026-11-04】单缓冲兼容模式在
    /// 呈现冻结排查中实证无效（清屏色循环实验：单/双缓冲窗口均恒首帧色），
    /// 已按 MMC（MPV_Media_Console）呈现模型移除——恒双缓冲 + SwapBuffers
    /// + vsync(1)。键保留解析仅为兼容旧 config.ini（配置了也不生效，
    /// 仅打一条废弃告警），后续版本可移除。
    long gl_single_buffer = 0;
    /// [display] borderless：无边框窗口（2026-11-04 用户决策——媒体服务
    /// 器输出窗不做系统装饰）。WS_POPUP 无边框无标题栏，几何仍按
    /// screen/window_* 配置定位；关闭靠 Alt+F4 / 任务栏右键。
    /// 【默认 1】与 WATCHOUT 输出窗一致的纯净呈现；0 = 保留系统边框。
    long borderless = 1;
    /// [node] mode：节点模式（2026-11-11 手动切换定案）——
    ///   * standalone：本机播放器（HTTP 控制面全功能：编排/媒体库/播放/设置；
    ///     不监听 17100，NodeController 驱动本机注入）；
    ///   * sync：Editor 受控（17100 监听 + Editor 下发；HTTP 仅 状态/设置）。
    /// 【auto 退役 2026-11-11】M3 自动仲裁计划取消——模式必须手动指定
    /// （网页设置页切换；auto/未知值按 standalone 兜底并警告迁移）。
    /// CLI --mode= 覆盖 ini。
    std::string mode = "standalone";
    /// --timeline=：开机自动播放的时间轴 id（覆盖 [standalone]
    /// default_timeline；特殊值 demo = 内置演示时间轴，无文件依赖）
    std::string timeline;
    /// [standalone] http_port：standalone 控制面监听端口（M2；0=不启用 HTTP）
    long http_port = 8995;                  ///< --http-port= 覆盖
    /// [standalone] max_upload_mb：网页上传素材单文件上限（0=不限）
    long max_upload_mb = 2048;              ///< --max-upload-mb= 覆盖
    /// [standalone] timelines_dir：网页编排时间轴文档目录（<id>.json）；
    /// 空 = exe 目录/timelines
    std::string timelines_dir;              ///< --timelines-dir= 覆盖
    /// --restart-wait-pid=<pid>（M5 自重启接力内部参数，非用户配置）：
    /// 新实例启动先等该 pid 的旧实例退出（端口/文件句柄释放）再初始化，
    /// 避免 HTTP 8995/TCP 17100 bind 撞车。正常启动不出现此参数。
    long restart_wait_pid = 0;
    bool help = false;
};

/// ini 预填默认值：键缺省时回落内置默认（与旧版硬编码一致——
/// config.ini 缺失时行为不变）。assets_root 允许 [common] 共享、
/// [display] 同键覆盖。
Args loadDefaultsFromIni(const ms::config::IniConfig& ini) {
    Args a;
    a.player_id         = ini.getStr("display", "player_id");
    a.listen_port       = ini.getLong("display", "listen_port", 17100);
    a.assets_root       = ini.getStr("common", "assets_root");
    a.assets_root       = ini.getStr("display", "assets_root", a.assets_root);
    a.asset_workers     = ini.getLong("display", "asset_workers", 3);
    a.render_fps        = ini.getLong("display", "render_fps", 60);
    a.render_out        = ini.getStr("display", "render_out");
    a.report_interval_ms = ini.getLong("display", "report_interval_ms", 250);
    a.diag_interval_ms  = ini.getLong("display", "diag_interval_ms", 5000);
    a.discovery_port    = ini.getLong("display", "discovery_port", 17610);
    a.screen            = ini.getLong("display", "screen", -1);
    a.fullscreen        = ini.getLong("display", "fullscreen", 0) != 0;
    a.window_x          = ini.getLong("display", "window_x", -1);
    a.window_y          = ini.getLong("display", "window_y", -1);
    a.window_w          = ini.getLong("display", "window_w", 0);
    a.window_h          = ini.getLong("display", "window_h", 0);
    a.quit_after_ms     = ini.getLong("display", "quit_after_ms", 0);
    a.pairing_code      = ini.getStr("display", "pairing_code");
    a.no_preview        = ini.getLong("display", "no_preview", 0) != 0;
    // 【默认 cuda（2026-11-05 修订，2026-11-06 落实）】此前此兜底仍写
    // d3d11va——键不存在时把结构体的 cuda 修订静默覆盖回 d3d11va（无
    // ini 用户永远走下载路径）。无 N 卡机器 cuda 探测失败运行时自动
    // 回退软解，无黑屏风险；与结构体默认/注释三处对齐。
    a.hw_accel          = ini.getStr("display", "hw_accel", "cuda");
    a.render_backend    = ini.getStr("display", "render_backend", "gl");
    a.audio_enable      = ini.getLong("display", "audio_enable", 1) != 0;
    a.audio_device      = ini.getStr("display", "audio_device");
    a.gl_single_buffer  = ini.getLong("display", "gl_single_buffer", 0);
    a.borderless        = ini.getLong("display", "borderless", 1);
    // ---- 节点模式 + standalone（display×MMC 合并方案 v2.2，§4.1/§4.4）----
    // [node] mode：standalone/sync（auto 已退役——解析兜底见运行段警告）
    a.mode              = ini.getStr("node", "mode", a.mode);
    // [standalone] default_timeline：开机自动播放的时间轴 id
    a.timeline          = ini.getStr("standalone", "default_timeline");
    // [standalone] http_port / max_upload_mb / timelines_dir：控制面（M2）
    a.http_port         = ini.getLong("standalone", "http_port", a.http_port);
    a.max_upload_mb     = ini.getLong("standalone", "max_upload_mb", a.max_upload_mb);
    a.timelines_dir     = ini.getStr("standalone", "timelines_dir");
    return a;
}

/// 解析 --key=value 命令行参数（覆盖 defaults 中由 ini 预填的值；
/// 复杂参数表出现后再引库）
std::optional<Args> parseArgs(int argc, char** argv, const Args& defaults) {
    Args args = defaults;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value_of = [&](const char* key) -> std::optional<std::string> {
            const std::string prefix = std::string("--") + key + "=";
            if (arg.rfind(prefix, 0) == 0) return arg.substr(prefix.size());
            return std::nullopt;
        };
        auto value_num = [&](const char* key, long& out) -> bool {
            if (auto v = value_of(key)) {
                out = std::atol(v->c_str());
                return true;
            }
            return false;
        };

        if (arg == "--help" || arg == "-h") {
            args.help = true;
        } else if (arg == "--no-preview") {
            args.no_preview = true; // 无值开关：命令行强制关预览
        } else if (value_num("port", args.listen_port)) {
        } else if (value_num("discovery-port", args.discovery_port)) {
        } else if (value_num("screen", args.screen)) {
        } else if (arg == "--fullscreen") {
            args.fullscreen = true; // 无值开关：预览窗口铺满目标屏
        } else if (value_num("window-x", args.window_x)) {
        } else if (value_num("window-y", args.window_y)) {
        } else if (value_num("window-w", args.window_w)) {
        } else if (value_num("window-h", args.window_h)) {
        } else if (auto v = value_of("id")) {
            args.player_id = *v;
        } else if (value_num("quit-after-ms", args.quit_after_ms)) {
        } else if (auto v = value_of("pairing-code")) {
            args.pairing_code = *v; // 配对码覆盖（ini [display].pairing_code）
        } else if (value_num("render-fps", args.render_fps)) {
        } else if (auto v = value_of("assets-root")) {
            args.assets_root = *v;
        } else if (value_num("asset-workers", args.asset_workers)) {
            // 素材并发拉取 worker 数（ini [display].asset_workers）
        } else if (auto v = value_of("hwaccel")) {
            args.hw_accel = *v; // 硬解后端覆盖（ini [display].hw_accel）
        } else if (auto v = value_of("render-backend")) {
            args.render_backend = *v; // 合成后端覆盖（ini [display].render_backend）
        } else if (value_num("gl-single-buffer", args.gl_single_buffer)) {
            // 【已废弃 2026-11-04】GL 单缓冲兼容模式已随 MMC 呈现模型对齐
            // 移除（单/双缓冲呈现冻结实证，见 GlContextWGL.hpp 注释）——
            // 参数保留仅为兼容旧脚本，配置了只打告警不生效。
        } else if (value_num("borderless", args.borderless)) {
            // 无边框窗口开关（ini [display].borderless）
        } else if (auto v = value_of("render-out")) {
            args.render_out = *v;
        } else if (arg == "--no-audio") {
            args.audio_enable = false; // 无值开关：关闭音频输出
        } else if (auto v = value_of("audio-device")) {
            args.audio_device = *v; // 音频设备覆盖（ini [display].audio_device）
            args.audio_enable = true; // 显式选设备 = 隐含开启音频
        } else if (auto v = value_of("mode")) {
            args.mode = *v; // 节点模式（ini [node].mode；standalone/sync）
        } else if (auto v = value_of("timeline")) {
            args.timeline = *v; // 开机自动播放时间轴 id（ini [standalone]
                                // .default_timeline；demo = 内置演示）
        } else if (value_num("http-port", args.http_port)) {
            // standalone 控制面端口（ini [standalone].http_port；0=不启用）
        } else if (value_num("max-upload-mb", args.max_upload_mb)) {
            // 网页上传上限（ini [standalone].max_upload_mb）
        } else if (auto v = value_of("timelines-dir")) {
            args.timelines_dir = *v; // 网页编排时间轴文档目录（ini
                                     // [standalone].timelines_dir；空=exe/timelines）
        } else if (value_num("restart-wait-pid", args.restart_wait_pid)) {
            // M5 自重启接力内部参数（旧实例退出等待；非用户配置，用法不印）
        } else if (arg.rfind("--config=", 0) == 0) {
            // 配置路径已由 main 消费（查找阶段）；此处容错跳过
        } else if (arg.rfind("--editor=", 0) == 0) {
            // 兼容提示：第五迭代起显示端为监听端，--editor 不再需要
            std::fprintf(stderr,
                         "提示: --editor 已废弃（显示端现自动监听等待 Editor"
                         " 接入；Editor 端\"添加显示端\"填本机地址即可）\n");
        } else {
            std::fprintf(stderr, "未知参数: %s（--help 查看用法）\n", arg.c_str());
            return std::nullopt;
        }
    }
    return args;
}

void printUsage() {
    std::printf(
        "media_display —— MediaServer 显示端（多屏帧同步播放器）\n"
        "【零操作】打开后自动监听端口等待 Editor 接入（无需任何配置）\n"
        "用法:\n"
        "  media_display [--config=<ini>]（启动设置；缺省按 exe 目录 →\n"
        "      工作目录查找 config.ini，模板见 config.display.example.ini）\n"
        "      [--id=player-1] [--port=17100] [--discovery-port=17610]\n"
        "      （--id：本地标签（发现应答自报/预览窗初显）；连接 Editor 后\n"
        "      采用其指派的屏 id（RegisterAck 下发），无需与工程绑定一致；\n"
        "      指派 id 自动写回 config.ini [display].player_id 持久保存，\n"
        "      下次启动即沿用（--id 显式项仍优先于配置文件））\n"
        "      （--discovery-port：局域网发现应答端口，供编辑器扫描；0=禁用）\n"
        "      [--assets-root=<dir>]  （素材落盘目录，收场景后自动拉取；\n"
        "      同机多开请各实例指定不同目录，避免并发拉取互相干扰）\n"
        "      [--asset-workers=3]    （素材并发拉取 worker 数；1=串行。\n"
        "      多个大素材首次下发时可显著缩短总同步时长）\n"
        "      [--render-fps=60] [--render-out=<dir>]\n"
        "      （渲染线程帧率；诊断帧 BMP 导出目录，联测核对画面）\n"
        "      [--no-audio] [--audio-device=<名称>]\n"
        "      （音频输出开关——默认开，与画面同会话时刻出声；无声卡时\n"
        "      自动回退静音。--audio-device 按 FriendlyName 包含匹配选设备）\n"
        "      [--screen=N] [--fullscreen] [--window-x=X] [--window-y=Y]\n"
        "      [--window-w=W] [--window-h=H]\n"
        "      （预览窗口定位到第 N 块显示器；--fullscreen 无边框铺满。\n"
        "      window-x/y/w/h 仅 fullscreen=false 生效：x/y 为屏内偏移\n"
        "      （配 screen）或全局坐标，负值=屏内居中；w/h<=0=默认 960×540。\n"
        "      同机多开实例各配一块屏，预览互不遮挡）\n"
        "      [--quit-after-ms=<ms>] （运行 N 毫秒后自动退出；0=常驻）\n"
        "      [--pairing-code=<码>]  （配对码：非空则只接受同码 Editor 连接；\n"
        "                               覆盖 config.ini 的 pairing_code）\n"
        "      [--no-preview]         （关闭输出预览窗口；Windows 默认开启）\n"
        "      [--render-backend=gl|soft]（合成后端：gl=GPU shader 合成（默认，\n"
        "      GL 窗口即输出窗口）；soft=CPU 画布 + GDI 预览窗）\n"
        "      [--gl-single-buffer=0|1]（GL 单缓冲兼容模式，默认 1：GL 渲染\n"
        "      直绘主表面，任何观看环境（含远程桌面/锁屏会话）都可靠上屏；\n"
        "      0 = 双缓冲 flip——正常桌面最低延迟，远程会话可能恒黑）\n"
        "      [--borderless=0|1]（无边框窗口，默认 1：无标题栏边框的输出\n"
        "      窗，几何按 screen/window_* 定位；0 = 保留系统边框）\n"
        "      [--mode=standalone|sync]（节点模式，ini [node].mode：\n"
        "      standalone=本机播放器（时间轴自配，HTTP/网页全功能：编排/\n"
        "      媒体库/播放/设置）；sync=等待 Editor 受控（HTTP 仅状态与\n"
        "      设置）。auto 已退役（自动仲裁计划取消）——按 standalone\n"
        "      兜底并在网页设置页手动切换）\n"
        "      [--timeline=<id>]（standalone 开机自动播放的时间轴，ini\n"
        "      [standalone].default_timeline；demo = 内置演示时间轴）\n"
        "      [--http-port=8995]（HTTP 控制面端口，两模式共用；0=不启用。\n"
        "      网页编排台 http://<本机IP>:<port>/，sync 模式下仅设置页）\n"
        "      [--max-upload-mb=2048]（网页上传素材单文件上限，0=不限）\n"
        "      [--timelines-dir=<dir>]（网页编排时间轴文档目录 <id>.json，\n"
        "      ini [standalone].timelines_dir；默认 exe 目录/timelines）\n"
        "配置优先级：命令行显式项 > config.ini > 内置默认\n"
        "接入方式：在编辑器中\"添加显示端\"填 本机IP:<port>，或用\n"
        "\"扫描局域网显示端\"自动带出（同机多开端口被占时自动改配，\n"
        "以扫描列表/启动日志里的实际端口为准）\n");
}

// ---------------------------------------------------------------------------
// standalone 装配辅助（display×MMC 合并方案 v2.2；M1：ini 直配时间轴）
// ---------------------------------------------------------------------------

/// 文件路径 → 时间轴 id（文件名去扩展名；ini 未显式给 id 时的缺省）
std::string idFromFile(const std::string& file) {
    std::filesystem::path p(reinterpret_cast<const char8_t*>(file.c_str()));
    return p.stem().string();
}

/// 从 config.ini [standalone] 读 timeline.N.* 条目注册到注册表。
/// 每条：timeline.<N>.file（.show 工程文件 = 时间轴编排载体，与 editor
/// 工程互通）+ 可选 .id / .name / .loop / .stop-position。
/// 相对路径先按素材根目录域解析（素材域习惯；否则按工作目录）。
/// \return 成功注册条数
int fillStandaloneRegistry(ms::standalone::TimelineRegistry& registry,
                           const ms::config::IniConfig& ini,
                           const std::string& assets_root,
                           const std::string& player_id) {
    using ms::standalone::TimelineEntry;
    int loaded = 0;
    for (int n = 1; n <= 64; ++n) {
        const auto key = [n](const char* leaf) {
            return "timeline." + std::to_string(n) + "." + leaf;
        };
        std::string file = ini.getStr("standalone", key("file"));
        if (file.empty()) continue; // 容忍序号空洞（用户删中间条目）

        // 相对路径解析：素材根目录优先，缺失回落工作目录
        if (!file.empty() && file.find(':') == std::string::npos &&
            file.front() != '/' && file.front() != '\\') {
            const std::string rooted = assets_root + "/" + file;
            std::error_code ec;
            if (std::filesystem::exists(
                    std::filesystem::path(reinterpret_cast<const char8_t*>(
                        rooted.c_str())), ec))
                file = rooted;
        }

        std::string err;
        const auto doc = ms::ShowFileIO::loadFromFile(file, err);
        // [round3 S2] 加载失败不再静默丢弃：注册降级条目（同 loadTimelineDocs
        // 的可见性纪律），note 说明失效原因
        if (!doc) {
            std::fprintf(stderr,
                         "[standalone] 时间轴文件加载失败（降级装载）: %s\n  %s\n",
                         file.c_str(), err.c_str());
            TimelineEntry e;
            e.id = ini.getStr("standalone", key("id"), idFromFile(file));
            e.name = ini.getStr("standalone", key("name"), e.id);
            e.note = "工程文件加载失败: " + err;
            if (registry.add(std::move(e)).empty()) {
                std::fprintf(stderr,
                             "[standalone] 时间轴 id 冲突（跳过）: %s (%s)\n",
                             file.c_str(),
                             ini.getStr("standalone", key("id")).c_str());
            } else {
                ++loaded;
            }
            continue;
        }

        TimelineEntry e;
        e.id   = ini.getStr("standalone", key("id"), idFromFile(file));
        e.name = ini.getStr("standalone", key("name"), e.id);
        e.loop = ini.getLong("standalone", key("loop"), 0) != 0;
        e.stop_position_us =
            ini.getLong("standalone", key("stop-position"), 0) * 1000;
        e.scene = ms::standalone::SceneAssembler::adoptForStandalone(
            doc->scene, player_id);
        if (registry.add(std::move(e)).empty()) {
            std::fprintf(stderr,
                         "[standalone] 时间轴 id 冲突（跳过）: %s (%s)\n",
                         file.c_str(),
                         ini.getStr("standalone", key("id")).c_str());
            continue;
        }
        ++loaded;
    }
    return loaded;
}

/// 扫描 timelines_dir/*.json 装载网页编排的时间轴（M2 控制面的持久化侧；
/// 文档格式/落盘由 StandaloneApi 管理——此处只做"开机恢复"读取）。
/// id = 文件名去 .json；解析失败单条跳过（不阻塞其余装载）。
/// \return 成功注册条数
int loadTimelineDocs(ms::standalone::TimelineRegistry& registry,
                     ms::standalone::SceneAssembler& assembler,
                     const std::string& timelines_dir,
                     const std::string& player_id) {
    namespace fs = std::filesystem;
    int loaded = 0;
    std::error_code ec;
    const fs::path dir = fs::path(reinterpret_cast<const char8_t*>(timelines_dir.c_str()));
    if (!fs::is_directory(dir, ec)) return 0;
    for (auto it = fs::directory_iterator(dir, ec);
         !ec && it != fs::directory_iterator(); it.increment(ec)) {
        const auto& entry = *it;
        if (!entry.is_regular_file(ec)) continue;
        std::string name(reinterpret_cast<const char*>(entry.path().filename()
                                                            .u8string()
                                                            .c_str()));
        if (name.size() <= 5 || name.substr(name.size() - 5) != ".json") continue;
        const std::string id = name.substr(0, name.size() - 5);

        std::string content, err;
        {
            std::ifstream ifs(entry.path(), std::ios::binary);
            if (!ifs) continue;
            std::ostringstream oss;
            oss << ifs.rdbuf();
            content = oss.str();
        }
        // 文档解析复用 StandaloneApi::parseDoc（公开静态——单一解析器原则，
        // 网页写入与开机装载同构）
        // [round3 S2] 解析/装配失败不再静默丢弃：注册降级条目（scene 为空
        // + note 失效说明），列表保留可见性——与"文档=编辑态真相、play 时
        // 才校验"纪律对齐（磁盘文档还在，运行期自洽创建的条目重启后凭空
        // 消失会让用户失去追踪入口）。修好素材或改编排保存即可恢复。
        media_api::StandaloneApi::TimelineDoc doc;
        std::string degrade_note;  // 非空 = 降级装载（不可直接播放）
        if (!media_api::StandaloneApi::parseDoc(content, doc, err)) {
            std::fprintf(stderr,
                         "[standalone] 时间轴文档解析失败（降级装载）: %s\n  %s\n",
                         name.c_str(), err.c_str());
            degrade_note = "文档解析失败: " + err;
        }
        std::shared_ptr<const ms::Scene> scene;
        if (degrade_note.empty()) {
            scene = assembler.assemble(doc.scene, player_id, err);
            if (scene == nullptr) {
                std::fprintf(stderr,
                             "[standalone] 时间轴装配失败（降级装载）: %s\n  %s\n",
                             name.c_str(), err.c_str());
                degrade_note = "装配失败（素材未就位或编排无效）: " + err;
            }
        }
        ms::standalone::TimelineEntry e;
        e.id = id;
        e.name = doc.name.empty() ? id : doc.name;
        e.loop = doc.loop;
        e.stop_position_us = doc.stop_position_us;
        e.scene = std::move(scene);
        e.note = std::move(degrade_note);
        if (registry.add(std::move(e)).empty()) {
            std::fprintf(stderr, "[standalone] 时间轴 id 冲突（跳过）: %s\n",
                         id.c_str());
            continue;
        }
        ++loaded;
    }
    return loaded;
}

/// 内置演示时间轴（--timeline=demo；无文件依赖——Text 素材随场景走）：
/// 主标题淡入 → 副标题错位淡入 → 双淡出 → 循环。M1 渲染链接缝的冒烟
/// 载体（场景注入/播放头推进/淡入淡出双通道/出点行为一次跑通）。
void registerDemoTimeline(ms::standalone::TimelineRegistry& registry,
                          const std::string& assets_root,
                          const std::string& player_id) {    ms::standalone::SceneSpec spec;
    spec.canvas_w = 1920.0;
    spec.canvas_h = 1080.0;

    ms::standalone::TrackSpec tr;
    tr.name = "演示";

    // 主标题：整段 8s，头尾 0.8s 淡入淡出（Opacity+AudioGain 双通道）
    ms::standalone::ClipSpec c1;
    c1.text         = "MediaServer Standalone";
    c1.start_us     = 0;
    c1.duration_us  = 8'000'000;
    c1.x            = 460;
    c1.y            = 470;
    c1.z            = 1;
    c1.fade_in_us   = 800'000;
    c1.fade_out_us  = 800'000;

    // 副标题：错位 1.2s 进场，6s 段，稍低不透明度
    ms::standalone::ClipSpec c2;
    c2.text         = "display x MMC 合并 - 本机时间轴控制器 (M1)";
    c2.start_us     = 1'200'000;
    c2.duration_us  = 6'000'000;
    c2.x            = 540;
    c2.y            = 570;
    c2.z            = 2;
    c2.opacity      = 0.85;
    c2.fade_in_us   = 600'000;
    c2.fade_out_us  = 600'000;

    tr.clips = {c1, c2};
    spec.tracks.push_back(std::move(tr));

    ms::standalone::SceneAssembler asm_(assets_root);
    std::string err;
    auto scene = asm_.assemble(spec, player_id, err);
    if (scene == nullptr) {
        std::fprintf(stderr, "[standalone] 演示时间轴装配失败: %s\n",
                     err.c_str());
        return;
    }
    ms::standalone::TimelineEntry e;
    e.id    = "demo";
    e.name  = "内置演示";
    e.loop  = true; // 循环——出点行为顺带被冒烟
    e.scene = std::move(scene);
    if (registry.add(std::move(e)).empty())
        std::fprintf(stderr, "[standalone] 演示时间轴注册冲突（已有同名）\n");
}

// ---------------------------------------------------------------------------
// 主循环：监听等待 → Editor 接入 → 收场景 → tick 驱动（锚点/纠偏/上报）
// ---------------------------------------------------------------------------
int runDisplay(const Args& args, const ms::config::IniConfig& ini,
               const std::string& config_write_path) {
    // 【崩溃现场捕获 2026-11-06】高频场景下发闪退排障基建（editor 端
    // CrashDump 平移至 core/platform 两端共用）——未处理异常 → exe 旁
    // crash-<时间戳>.dmp（线程栈+异常上下文+模块表），用户侧复现的闪退
    // 靠 dump 一锤定音。进程级一次；任意线程的硬件异常（GL 驱动 AV 等）
    // 都会走过滤器。
    ms::installCrashDump();

#if defined(_WIN32)
    // ---- 自重启接力等待（M5 /api/v1/system/restart）------------------------
    // 旧实例拉起本实例（原样命令行 + --restart-wait-pid）后立即走完退出链；
    // 此处等它真正退出（进程句柄 signaled = 端口/文件句柄全部释放）再继续
    // 初始化，避免 HTTP 8995/TCP 17100/素材锁 bind 撞车。上限 10s（旧实例
    // 卡死时不无限等待——撞车报错也比永久挂起可见）。
    if (args.restart_wait_pid > 0) {
        const HANDLE prev = ::OpenProcess(SYNCHRONIZE, FALSE,
                        static_cast<DWORD>(args.restart_wait_pid));
        if (prev != nullptr) {
            std::printf("[display] 自重启接力：等待旧实例(pid=%ld)退出…\n",
                        args.restart_wait_pid);
            std::fflush(stdout);
            ::WaitForSingleObject(prev, 10000);
            ::CloseHandle(prev);
            std::printf("[display] 自重启接力：旧实例已退出，继续初始化\n");
            std::fflush(stdout);
        } else {
            std::printf("[display] 自重启接力：旧实例(pid=%ld)已不存在，"
                        "直接初始化\n", args.restart_wait_pid);
        }
    }
#endif


    // ---- 节点模式（2026-11-11 手动切换定案）---------------------------------
    // standalone = 本机播放器（HTTP 控制面全功能：编排/媒体库/播放/设置）；
    // sync = Editor 受控（17100 监听 + Editor 下发；HTTP 仅 状态/设置/静态页
    // ——编排面路由 409 门禁，网页编排台随之收敛到设置页）。
    // 【auto 退役 2026-11-11 用户定案】M3 自动仲裁计划取消——模式必须手动
    // 指定；auto（旧配置兼容）与未知值均按 standalone 兜底并警告迁移。
    // args 为 const 引用——归一化结果落局部变量 mode_effective。
    std::string mode_effective = args.mode;
    if (mode_effective != "standalone" && mode_effective != "sync") {
        std::fprintf(stderr,
                     "[display] 警告: mode='%s' 已退役/无法识别（M3 自动仲裁"
                     "计划取消），按 standalone 处理——请在网页设置页手动切换"
                     "（可用: standalone/sync）\n", mode_effective.c_str());
        mode_effective = "standalone";
    }
    const bool standalone_mode = (mode_effective == "standalone");

    // 自重启请求旗标（M5 /api/v1/system/restart）：restart_fn（HTTP 工作
    // 线程）只置位；主循环轮询到 → break 走与关窗同一优雅退出链——全部
    // 资源（HTTP/预览窗/渲染网络线程）按顺序析构后，末尾 CreateProcess
    // 拉起新实例接力。禁止在 HTTP 线程就地 exit（跳过 main 栈析构 =
    // GL 上下文/接收线程 UAF 风险）。
    std::atomic<bool> restart_requested{false};

    // 【端口值域】long → uint16 静默截断（70000→4464）会让 display 监听
    // 在错误端口且无报错；越界显式拒绝（0 允许：系统分配）。HTTP 控制面
    // 端口两模式共用（sync 下仅 状态/设置），同样校验。
    if (!standalone_mode &&
        (args.listen_port < 0 || args.listen_port > 65535)) {
        std::fprintf(stderr, "[display] 监听端口越界: %ld（有效范围 0-65535）\n",
                     args.listen_port);
        return 2;
    }
    if (args.http_port < 0 || args.http_port > 65535) {
        std::fprintf(stderr, "[display] HTTP 控制面端口越界: %ld"
                             "（有效范围 0-65535；0=禁用）\n", args.http_port);
        return 2;
    }

    ms::PlayerRuntime::Config cfg;
    // --id 缺省：按进程 ID 生成（联测脚本建议显式指定）
    cfg.player_id = args.player_id.empty()
#if defined(_WIN32)
                        ? "player-" + std::to_string(_getpid())
#else
                        ? "player-" + std::to_string(::getpid())
#endif
                        : args.player_id;
    // 【白屏防呆 2026-11-04】编辑器 screen 默认绑定 player-N（编号与屏幕
    // 对齐，scr-1↔player-1）——display 自报 id（player-<PID>）与其天然
    // 不一致。指派模型（2026-11-06）下连接建立即自愈：Editor 随 Ack 下发
    // 槽位 id，display 采用后重注册/重选屏——提示语同步改为"等待自动指派"。
    if (args.player_id.empty()) {
        std::fprintf(stderr,
                     "[display] 未指定 --id，本地标签=%s——连接 Editor 后将\n"
                     "自动采用其指派的屏 id（无需手动对齐）\n",
                     cfg.player_id.c_str());
        std::fflush(stderr);
    }
    // 监听模式（WATCHOUT 模型，2026-11-03 起为唯一模式）：零配置等待
    // Editor 接入（端口已校验值域）
    cfg.listen_port = static_cast<std::uint16_t>(args.listen_port);
    // 配对码（多 Editor 隔离）：非空 = 只接受携带同码 MsgAuth 的 Editor
    cfg.pairing_code = args.pairing_code;
    cfg.assets_root = args.assets_root; // 素材落盘目录
    // 素材并发拉取 worker 数（WATCHOUT Runner 2 worker 同型能力；<=0 归一为 1）
    cfg.asset_workers = args.asset_workers > 0
                            ? static_cast<int>(args.asset_workers)
                            : 1;
    // 【素材路径兜底（裸启动无素材修复）】assets_root 未配置时回退 exe 目录，
    // 与 editor 端（media_editor.exe main.cpp 同构兜底）对称。此前空根目录
    // 双重禁用素材链路：①RenderThread 跳过全部媒体源注册 → 每帧刷
    // "[sched] 素材类型未注册"且画面全占位；②fetchAssetsAsync 直接视为
    // 就绪不拉取。裸启动（双击 exe / 只给 --id）是交付包的常态用法，
    // 素材与 exe 同目录（editor 复制落盘也在 exe 旁 media/），exe 目录
    // 即最合理的缺省根。
    if (cfg.assets_root.empty()) {
        const std::string exe_dir = ms::config::exeDirectory();
        if (!exe_dir.empty()) {
            cfg.assets_root = exe_dir;
            std::fprintf(stderr,
                         "[display] 未配置 assets_root，回退 exe 目录：%s"
                         "（--assets-root= 可显式指定）\n",
                         exe_dir.c_str());
        } else {
            // exeDirectory 失败（罕见）：宁可显著警告也不静默半残运行
            std::fprintf(stderr,
                         "[display] 警告: assets_root 为空——媒体源不注册、"
                         "素材不拉取，画面将全为占位色块！请用 "
                         "--assets-root= 指定素材目录\n");
        }
        std::fflush(stderr);
    }
    // 渲染线程（§5.6）：--render-fps 启用；--render-out 指定诊断帧导出目录
    cfg.render_fps         = static_cast<int>(args.render_fps);
    // 音频输出（P3）：默认开；渲染线程关闭时音频拍无处挂载 → 随之关闭
    cfg.audio_enable       = args.audio_enable && args.render_fps > 0;
    cfg.audio_device       = args.audio_device;
    cfg.render_capture_dir = args.render_out;
    // 运维可调参数（config.ini [display]；缺省保持旧硬编码值）
    cfg.report_interval_us =
        static_cast<std::int64_t>(args.report_interval_ms) * 1000;
    cfg.diag_interval_us =
        static_cast<std::int64_t>(args.diag_interval_ms) * 1000; // <=0=关闭诊断心跳
    // 局域网发现应答（editor 扫描添加显示端的数据源；0 = 禁用）
    cfg.discovery_port = static_cast<std::uint16_t>(args.discovery_port);
    // 发现应答自报运行模式（M5 编辑器互通）：editor 扫描列表徽标，
    // 提示该端可否导入/导出 .show 工程（仅 standalone 开放）
    cfg.discovery_mode = standalone_mode ? 1 : 2;
    // ---- standalone 本机模式（v2.2 §4.1）：PlayerRuntime 关网络三件套，
    // 场景/播放控制经 NodeController 本机注入口（本机 = 立即锚点走同一
    // 状态机）。渲染线程/音频/硬解/GL 全链照常。
    cfg.standalone = standalone_mode;
    // ---- 视频硬解后端（第①阶段硬解抽象层）----
    // 配置意图（默认 d3d11va）：解析失败/后端不可用由媒体源运行时探测
    // 自动回退软解——配置错误不致命，日志给出实际生效路径。
    cfg.hw_accel.type = ms::HwAccelConfig::parseType(args.hw_accel);
    if (cfg.hw_accel.type == ms::HwAccelType::Soft &&
        args.hw_accel != "soft" && args.hw_accel != "off" &&
        args.hw_accel != "none") {
        std::fprintf(stderr,
                     "[display] 警告: hw_accel='%s' 无法识别，已回落软解"
                     "（可用: d3d11va/cuda/nvdec/vaapi/mediacodec/auto/soft）\n",
                     args.hw_accel.c_str());
    }
    std::fprintf(stderr, "[display] 硬解配置: %s\n",
                 ms::HwAccelConfig::typeName(cfg.hw_accel.type));

#if defined(_WIN32)
    // ---- 画面合成后端（第②阶段 GL 渲染管线）----------------------------
    //   * gl（默认）：GPU shader 合成——注入 backend_factory，GL 窗口/
    //     上下文在渲染线程内创建（线程亲和）；画面经 SwapBuffers 直上窗口。
    //     与 GDI 预览窗【互斥】（GL 窗口本身就是输出窗口，双窗冗余）。
    //   * soft：旧路径——CPU 画布合成 + frame_sink → GDI 预览窗上屏。
    const bool use_gl_backend = (args.render_backend == "gl");

    // ---- CUDA 0 拷贝协商旗标（③阶段）------------------------------------
    // 生效条件：GL 后端 + cuda/nvdec 硬解——两者都配齐才创建共享旗标
    // （解码端发布 GpuCudaNv12 原生帧 ⇄ 渲染端 interop 消费）。任一端
    // 探测失败（无 N 卡 GL / 注册失败 / D2D 拷贝异常）→ 置 false →
    // 解码端下一帧起自动回退下载路径（无黑屏，首帧竞态 <1s 自愈）。
    std::shared_ptr<std::atomic_bool> gpu_delivery_flag;
    const bool hw_is_cuda = (cfg.hw_accel.type == ms::HwAccelType::Cuda);
    if (use_gl_backend && hw_is_cuda) {
        // 【意图开关 + 协商旗标缺一不可（2026-11-05 补齐）】FfmpegMediaSource
        // 的 GPU 发布分支要求 gpu_delivery（意图）与 gpu_delivery_ok（协商）
        // 同时置位——此前只设旗标漏了意图，0 拷贝发布分支从未激活（恒走
        // 硬解下载路径）。补齐后 cuda 硬解 + GL 合成才真正零拷贝。
        gpu_delivery_flag = std::make_shared<std::atomic_bool>(true);
        cfg.hw_accel.gpu_delivery     = true;
        cfg.hw_accel.gpu_delivery_ok  = gpu_delivery_flag;
        std::fprintf(stderr, "[display] CUDA 0 拷贝交付: 已启用"
                             "（cuda 硬解 + gl 合成；探测失败自动回退）\n");
    } else if (use_gl_backend &&
               cfg.hw_accel.type != ms::HwAccelType::Soft) {
        // 【性能提示 2026-11-05】非 cuda 硬解 + gl 合成 = 硬解帧必须下载
        // 回 CPU 再上传 GPU 合成（GPU→CPU 读回是大头，比软解还慢 1.9×，
        // ③阶段基准 8.8ms vs 4.7ms/帧）——用户反馈"editor 预览流畅、
        // display 卡顿"的典型根因。提示改配置，不改行为（用户显式选择
        // 尊重之；如 d3d11va 在无 N 卡且软解吃力时的折中）。
        std::fprintf(stderr,
                     "[display] 性能提示: hw_accel=%s + gl 合成走硬解下载"
                     "路径（比软解慢）——建议 hw_accel=cuda（N 卡 0 拷贝）"
                     "或 soft（软解多线程）\n",
                     ms::HwAccelConfig::typeName(cfg.hw_accel.type));
    }

    if (use_gl_backend) {
        // 捕获窗口几何值（args 生命周期覆盖 runDisplay，直接引用亦可，
        // 拷贝一份更稳——工厂闭包由 RenderThread 持有，避免悬垂歧义）
        ms::GlRenderer::WindowGeom g;
        g.fullscreen = args.fullscreen;
        g.screen     = static_cast<int>(args.screen);
        g.x          = static_cast<int>(args.window_x);
        g.y          = static_cast<int>(args.window_y);
        g.w          = static_cast<int>(args.window_w);
        g.h          = static_cast<int>(args.window_h);
        // 无边框窗口透传到 GL 建窗（gl_single_buffer 已废弃不透传——
        // 恒双缓冲 + SwapBuffers + vsync(1)，MMC 呈现模型）
        g.borderless = (args.borderless != 0);
        if (args.gl_single_buffer != 0) {
            std::fprintf(stderr, "[display] 提示: gl_single_buffer 已废弃"
                                 "（呈现模型已对齐 MMC：恒双缓冲 + SwapBuffers"
                                 " + vsync）\n");
        }
        if (args.borderless != 0) {
            std::fprintf(stderr, "[display] 无边框窗口: 开（媒体输出纯净"
                                 "呈现；关闭用 Alt+F4 / 任务栏右键）\n");
        }
        g.title      = L"media_display 输出（GL）" +
                       std::wstring(cfg.player_id.begin(), cfg.player_id.end());
        // 协商旗标进闭包（shared_ptr 副本——渲染后端与解码端各持一份）
        auto delivery_flag = gpu_delivery_flag;
        cfg.render_backend_factory = [g, delivery_flag]() {
            return std::unique_ptr<ms::IComposeRenderer>(
                new ms::GlRenderer(g, delivery_flag));
        };
        std::fprintf(stderr, "[display] 合成后端: gl（GPU shader 合成，"
                             "GL 窗口即输出窗口）\n");
    } else {
        if (args.render_backend != "soft")
            std::fprintf(stderr,
                         "[display] 警告: render_backend='%s' 无法识别，"
                         "已回落 soft（可用: gl/soft）\n",
                         args.render_backend.c_str());
        std::fprintf(stderr, "[display] 合成后端: soft（CPU 画布 + GDI 预览窗）\n");
    }

    // ---- 输出预览窗口（仅 soft 后端；GL 后端窗口即输出）------------------
    // soft 路径（旧版）：渲染线程每帧经 frame_sink 回调 → 互斥快照 →
    // GDI 窗口 ~30fps 拉伸上屏。start 失败（服务会话/无桌面）自动降级
    // headless，不影响同步输出。
    const bool want_preview =
        !use_gl_backend && args.render_fps > 0 && !args.no_preview;
    if (want_preview) {
        cfg.render_frame_sink = [](int w, int h, const std::uint8_t* px,
                                   std::uint32_t stride) {
            ms::PreviewWindowWin32::present(w, h, px, stride); // 互斥快照拷贝
        };
    }
#endif

    // ---- 媒体同步进度（"同步媒体"可视化；媒体文件独立同步的配套）----
    // 双出口：
    //   1. 预览窗口底部进度条（纯 Win32 GDI 自绘——显示端保持零 UI 库
    //      依赖，安装包只有 exe + FFmpeg/core 运行时）；
    //   2. 控制台节流日志（500ms/条，\r 原地刷新；headless 部署
    //      [--no-preview / 服务模式] 的进度可观测兜底）。
    // 回调在拉取线程上下文同步调用（PlayerRuntime 文档约定）：两个出口
    // 都是非阻塞快照/打印，不拖累拉取节奏。
    cfg.asset_progress_fn = [](const ms::PlayerRuntime::AssetProgress& p) {
#if defined(_WIN32)
        // 出口 1：预览窗口进度条（窗口未开/已关 = 内部零开销直返）
        ms::PreviewWindowWin32::setAssetProgress(
            !p.finished, static_cast<int>(p.done),
            static_cast<int>(p.total), p.current_path.c_str(),
            p.current_total > 0
                ? static_cast<double>(p.current_bytes) /
                      static_cast<double>(p.current_total)
                : -1.0);
#endif
        // 出口 2：控制台节流日志（阶段变化立即打；同阶段 500ms 限频）。
        // 进行中用 \r 原地刷新（末尾补空格清残留）；完成行换行固定留档。
        static std::string last_line;
        static std::chrono::steady_clock::time_point last_tp{};
        std::string line;
        if (p.finished) {
            line = "[媒体同步] 完成 " + std::to_string(p.done) + "/" +
                   std::to_string(p.total);
        } else {
            char pct[16] = "";
            if (p.current_total > 0)
                std::snprintf(pct, sizeof(pct), "%.0f%%",
                              100.0 * static_cast<double>(p.current_bytes) /
                                  static_cast<double>(p.current_total));
            line = "[媒体同步] " + std::to_string(p.done) + "/" +
                   std::to_string(p.total) + "  " + p.current_path +
                   (pct[0] ? std::string("  ") + pct : "");
        }
        const auto now = std::chrono::steady_clock::now();
        if (p.finished) {
            std::printf("%s\n", line.c_str()); // 完成行：固定留档
            std::fflush(stdout);
            last_line.clear();
        } else if (line != last_line ||
                   now - last_tp > std::chrono::milliseconds(500)) {
            std::printf("\r%-72s", line.c_str());
            std::fflush(stdout);
            last_line = line;
            last_tp   = now;
        }
    };

    ms::PlayerRuntime player(cfg);
    if (!player.start()) {
        std::fprintf(stderr, "[display] 启动失败（监听端口被占用？）\n");
        return 2;
    }

    // ---- standalone：多时间轴注册表 + 节点控制器 + HTTP 控制面 + 开机播放 --
    // （v2.2 §4.4：时间轴 = 播放列表；唯一活跃；default_timeline/--timeline=
    //   开机即播。时间轴三个来源：ini timeline.N.*（.show 工程）→
    //   timelines_dir/*.json（M2 网页编排文档）→ demo 内置演示。
    //   析构序：声明逆序——http_server → api → node_ctl → assembler →
    //   registry → player（HTTP 先停，不再触碰引擎/注册表引用）。
    //   【HTTP 必须为栈对象而非 unique_ptr 成员拆装】保证 stop() 先于
    //   引用者析构（退出链铁律：先 join/停消费线程，再拆被指向资源）。
    std::unique_ptr<ms::standalone::TimelineRegistry> timeline_registry;
    std::unique_ptr<ms::standalone::SceneAssembler>   scene_assembler;
    std::unique_ptr<ms::standalone::NodeController>   node_ctl;
    // HTTP 控制面（optional 就地构造，块外声明——存活覆盖整个主循环；
    // 声明序在其引用者之后 → 逆序析构 http → api → node → assembler →
    // registry → player，退出链"先停消费方再拆资源"铁律满足）
    std::optional<media_api::StandaloneApi> standalone_api;
    std::optional<media_api::HttpServer>    http_server;
    // MPCDI 服务（M4 收编自 MMC；声明在 api 之前 → 逆序析构 http → api
    // → mpcdi —— api 析构不引用 service ✓；service 只持 RenderThread 裸
    // 指针（不拥有），析构不触碰渲染线程生命周期 ✓）。MPCDI 是本机输出
    // 校准能力：两模式均装配（路由门禁前，sync 受控下仍可用）。
    std::optional<media_api::MpcdiService> mpcdi_service;
    // 注册表/装配器/NodeController 两模式都建（sync 下空转：无时间轴注册、
    // 无开机自动播——NodeController 仅被动等 HTTP 调用，构造无线程无监听）。
    // HTTP 控制面统一挂在 StandaloneApi；sync 门禁在 handle 入口收敛为
    // 仅 状态/设置/静态页（2026-11-11 手动模式切换定案：设置页在两种
    // 模式下都可用，编排面按 409 拒绝）。
    timeline_registry =
        std::make_unique<ms::standalone::TimelineRegistry>();
    scene_assembler =
        std::make_unique<ms::standalone::SceneAssembler>(cfg.assets_root);
    std::string exe_dir = ms::config::exeDirectory();
    std::string timelines_dir = !args.timelines_dir.empty()
                                    ? args.timelines_dir
                                    : (exe_dir.empty()
                                           ? std::string("timelines")
                                           : exe_dir + "/timelines");
    const std::string player_id = player.effectiveId(); // 装配产物 Screen 绑定
    if (standalone_mode) {
        const int loaded = fillStandaloneRegistry(*timeline_registry, ini,
                                                  cfg.assets_root, player_id);
        if (loaded > 0)
            std::printf("[standalone] 已从配置注册 %d 个时间轴（共 %zu 项）\n",
                        loaded, timeline_registry->size());
        // M2：网页编排文档装载（timelines_dir/*.json——HTTP CRUD 的落盘侧）
        const int docs = loadTimelineDocs(*timeline_registry, *scene_assembler,
                                          timelines_dir, player_id);
        if (docs > 0)
            std::printf("[standalone] 已装载 %d 个网页编排时间轴（%s）\n",
                        docs, timelines_dir.c_str());
        if (loaded == 0 && docs == 0)
            std::fprintf(stderr,
                         "[standalone] 未配置时间轴——config.ini [standalone]"
                         " 段 timeline.<N>.file=<.show 文件>，或经网页编排"
                         "（--http-port，默认开）后保存\n");
        // demo 请求且注册表无同名条目 → 构造内置演示（无文件依赖）
        if (args.timeline == "demo" &&
            timeline_registry->find("demo") == nullptr)
            registerDemoTimeline(*timeline_registry, cfg.assets_root,
                                 player_id);
    }

    node_ctl = std::make_unique<ms::standalone::NodeController>(
        player, *timeline_registry);

    // ---- HTTP 控制面（两模式共用；optional 就地构造，见块外声明）----------
    media_api::StandaloneApi::Config api_cfg;
    api_cfg.assets_root    = cfg.assets_root;
    api_cfg.timelines_dir  = timelines_dir;
    api_cfg.config_path    = config_write_path;
    api_cfg.max_upload_mb =
        args.max_upload_mb > 0 ? static_cast<std::size_t>(args.max_upload_mb) : 0;
    api_cfg.web_dir = exe_dir.empty() ? std::string("web") : exe_dir + "/web";
    api_cfg.mode = mode_effective; // 归一化后模式（门禁依据；见 handle 入口）
    // 重启回调（M5）：只置旗标立即返回（HTTP 工作线程上下文，响应尚未
    // flush；实际退出由主循环承担）。引用捕获 restart_requested——其作用
    // 域覆盖主循环，http_server->stop() 后不再有调用方（顺序纪律见退出链）。
    api_cfg.restart_fn = [&restart_requested] {
        restart_requested.store(true);
    };

    // MPCDI 服务装配（M4）：缓存目录 = config 同域（session 持久化）；
    // 几何投递目标 = 渲染线程（render_fps<=0 时 nullptr——enable 会拒绝
    // 并报"render thread not ready"，语义自洽）。startup 恢复在 HTTP
    // 启动后（路由可用、渲染线程已跑——重走 enable 构建路径）。
    mpcdi_service.emplace();
    mpcdi_service->set_paths(config_write_path.empty()
                                 ? (exe_dir.empty() ? std::string(".")
                                                    : exe_dir)
                                 : config_write_path);
    mpcdi_service->set_render_thread(player.renderThread());
    api_cfg.mpcdi = &mpcdi_service.value();

    standalone_api.emplace(player, *timeline_registry, *node_ctl,
                           *scene_assembler, player_id, api_cfg);
    http_server.emplace(standalone_api.value());
    if (args.http_port > 0) {
        std::string http_err;
        if (http_server->start("0.0.0.0", static_cast<int>(args.http_port),
                               http_err)) {
            std::printf(
                "[display] 控制面已启动: http://<本机IP>:%ld/  （%s）\n",
                args.http_port,
                standalone_mode ? "时间轴编排/媒体库/播放控制/设置"
                                : "仅状态与设置——同步模式下编排面已关闭");
        } else {
            // S2-2（2026-11-11 测试清单）：端口独占修复后，同端口第二实例
            // 从此分支失败退出。控制面是显式启用项（http_port>0）——
            // "有播放无控制面"的半残实例会让编排请求静默黑洞（双绑年代
            // 的排障噩梦同源），改为致命：日志 + exit 2。退出链同主循环
            // 收尾：HTTP 先停 → player.stop() join 渲染/网络线程（此时
            // 预览窗/MPCDI 会话尚未启动，其余栈对象逆序析构兜底）。
            std::fprintf(stderr,
                         "[display] 控制面启动失败（端口 %ld 被占用？）: %s\n"
                         "——进程退出（exit 2）：控制面为显式启用项，半残运行"
                         "只会让编排请求静默黑洞\n",
                         args.http_port, http_err.c_str());
            std::fflush(stderr);
            if (http_server) http_server->stop();
            player.stop();
            return 2;
        }
    } else {
        std::printf("[display] 控制面未启用（http_port=0）\n");
    }
    std::fflush(stdout);

    // MPCDI 会话恢复（M4，两模式均做——session enabled=true 时重走 enable
    // 构建路径；包缺失/渲染线程未跑等失败仅告警，不影响播放链；"未启用"
    // 是正常待机态，静默）
    if (mpcdi_service) {
        std::string mpcdi_err;
        if (mpcdi_service->restore_on_startup(mpcdi_err)) {
            std::printf("[mpcdi] 已从会话恢复变形融合\n");
        } else if (mpcdi_err != "mpcdi not enabled in session") {
            std::fprintf(stderr, "[mpcdi] 会话恢复未执行: %s\n",
                         mpcdi_err.c_str());
        }
        std::fflush(stdout);
    }

    if (standalone_mode) {
        // 开机自动播放（v2.2 定案）：default_timeline（ini）优先，
        // --timeline= 显式项覆盖；无配置 = 待机（不视为故障）
        std::string play_err;
        if (!node_ctl->playDefault(args.timeline, play_err))
            std::fprintf(stderr, "[standalone] 开机自动播放未执行: %s\n",
                         play_err.c_str());
        std::fflush(stderr);
    }

#if defined(_WIN32)
    if (want_preview) {
        // 窗口标题带实例身份（同机多开时窗口可辨认；id 通常为 ASCII，
        // 逐字节扩展转换足够——非 ASCII 字符显示不完美但不影响功能）
        std::wstring title = L"media_display 输出预览";
        if (!cfg.player_id.empty()) {
            title += L"  [" + std::wstring(cfg.player_id.begin(),
                                           cfg.player_id.end()) + L"]";
        }
        // --screen=N / --fullscreen / window-*：预览窗口几何（多实例各占一屏）
        const std::string where =
            args.fullscreen
                ? (args.screen >= 0 ? "（全屏于显示器 " +
                                          std::to_string(args.screen) + "）"
                                    : "（全屏于主屏）")
                : (args.screen >= 0
                       ? "（定位显示器 " + std::to_string(args.screen) + "）"
                       : "");
        // 尺寸提示（仅手动配置了 w/h 且非全屏时显示；<=0 的轴用实际默认值）
        std::string size_note;
        if (!args.fullscreen && (args.window_w > 0 || args.window_h > 0)) {
            size_note = "（尺寸 " +
                        std::to_string(args.window_w > 0 ? args.window_w : 960) +
                        "×" +
                        std::to_string(args.window_h > 0 ? args.window_h : 540) +
                        "）";
        }
        if (ms::PreviewWindowWin32::start(
                title.c_str(), static_cast<int>(args.screen),
                args.fullscreen, static_cast<int>(args.window_x),
                static_cast<int>(args.window_y),
                static_cast<int>(args.window_w),
                static_cast<int>(args.window_h)))
            std::printf("[display] 输出预览窗口已打开%s%s"
                        "（关闭窗口不影响同步输出；--no-preview 可关）\n",
                        where.c_str(), size_note.c_str());
        else
            std::fprintf(stderr,
                         "[display] 预览窗口创建失败，保持 headless 输出\n");
    }
#endif

    ms::ClockProvider clock;
    const auto t0 = clock.alignedNowUs();

    // 【指派 id 标题跟随 2026-11-06】轮询生效 id：display 采用 Editor 指派
    // id 后立即反映到预览窗标题（现场核验"这块屏渲染的是谁"的直接证据；
    // 控制台同步留档）。低频探测（生效 id 变化才会动作，常态零开销）。
    // 【指派 id 持久化 2026-11-06 用户需求】变化时同步写回 config.ini 的
    // [display] player_id——下次启动自动沿用指派 id（无需每次重新对号；
    // 命令行 --id 显式项优先级仍高于 ini，不受影响）。
    std::string shown_id = player.effectiveId();

    // 主循环：10ms 节拍驱动（渲染循环由 RenderThread 独立线程承担）
    // 【关窗即退出（2026-11-04 用户需求）】轮询两类窗口的关闭旗标——
    //   * GL 输出窗（默认后端）：GlContextWGL::userClosed（WM_CLOSE/DESTROY）
    //   * soft 预览窗：PreviewWindowWin32::userClosed（WM_CLOSE）
    // 旧行为"关窗后进程继续空转"曾是有意设计（预览窗可独立关闭），
    // 实际使用中用户关窗即预期 display 退出——旧行为留下残留进程（占
    // 监听端口/心跳线程），重启撞端口、任务管理器还得手动清理。
    // no_preview/headless 部署无窗口可关，退出仍走 --quit-after-ms。
    while (true) {
        // 【standalone 分派】NodeController 包装 tick：player 推进 + 出点
        // 行为（loop 回头 / 停止归零）；sync 模式直驱 player（历史路径）
        if (node_ctl)
            node_ctl->tick(clock.alignedNowUs());
        else
            player.tick(clock.alignedNowUs());

#if defined(_WIN32)
        // 【指派 id 标题跟随】生效 id 变化（Editor 指派采用）→ 更新预览窗
        // 标题 + 留档日志。同机多开辨识靠标题，指派后不改标题会误导现场。
        {
            const std::string eid = player.effectiveId();
            if (eid != shown_id) {
                shown_id = eid;
                std::printf("[display] 生效 id 已更新: %s（预览窗标题跟随）\n",
                            eid.c_str());
                // 持久化指派 id → config.ini（下次启动沿用；写失败仅告警
                // ——id 已生效，配置写不了不影响本次运行）
                if (!config_write_path.empty()) {
                    if (ms::config::saveKeyToFile(config_write_path, "display",
                                                  "player_id", eid)) {
                        std::printf("[display] 指派 id 已保存到 %s"
                                    "（下次启动自动沿用）\n",
                                    config_write_path.c_str());
                    } else {
                        std::fprintf(stderr,
                                     "[display] 警告: 指派 id 写入配置失败："
                                     "%s\n",
                                     config_write_path.c_str());
                    }
                }
                std::fflush(stdout);
                if (want_preview) {
                    std::wstring title = L"media_display 输出预览  [" +
                                         std::wstring(eid.begin(), eid.end()) +
                                         L"]";
                    ms::PreviewWindowWin32::updateTitle(title.c_str());
                }
            }
        }

        // 自重启请求（M5 网页"应用设置需重启"）：与关窗同一优雅退出链
        // （HTTP 先停——restart_fn 不会再被调，旗标语义全程单线程可见）
        if (restart_requested.load()) {
            std::printf("[display] 收到网页重启请求 → 优雅退出并拉起新实例\n");
            std::fflush(stdout);
            break;
        }

        // 输出/预览窗被用户关闭 → 优雅退出（全线程 join 后进程结束）
        if (ms::GlContextWGL::userClosed()) {
            std::printf("[display] 输出窗口已关闭 → 退出显示端\n");
            std::fflush(stdout);
            break;
        }
        if (ms::PreviewWindowWin32::userClosed()) {
            std::printf("[display] 预览窗口已关闭 → 退出显示端\n");
            std::fflush(stdout);
            break;
        }
#endif

        // 自动退出（联测脚本）
        if (args.quit_after_ms > 0) {
            const auto elapsed_ms =
                (clock.alignedNowUs() - t0) / 1000;
            if (elapsed_ms >= args.quit_after_ms) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 退出前状态快照（联测脚本核对；rendered=渲染线程累计帧数）
    // id 用生效 id（指派模型：现场渲染身份以 Editor 指派值为准）
    std::string exit_tail;
    if (node_ctl) {
        exit_tail = " active_timeline=" +
                    (node_ctl->hasActiveTimeline()
                         ? node_ctl->registry().activeId()
                         : std::string("(无)"));
    }
    std::printf("[display:%s] 退出: state=%d session_time=%lldus rate=%.3f "
                "delta=%lldus frame=%u assets=%zu/%zu ready=%d rendered=%llu"
                "%s\n",
                player.effectiveId().c_str(), static_cast<int>(player.state()),
                static_cast<long long>(player.session_time_us()),
                player.rate(),
                static_cast<long long>(player.last_delta_us()),
                player.frame_index(),
                player.assets_ready_count(), player.assets_total(),
                player.assets_ready() ? 1 : 0,
                static_cast<unsigned long long>(player.rendered_frames()),
                exit_tail.c_str());
    // 退出链（顺序纪律）：HTTP 控制面先停（不再受理请求、工作线程退出，
    // 之后无人触碰 registry/player）→ 预览窗 → 渲染/网络线程（player.stop
    // 内部 join）——"先停消费方再拆资源"。
    if (http_server) http_server->stop();   // optional 析构再兜底一次（幂等）
#if defined(_WIN32)
    ms::PreviewWindowWin32::stop(); // 关预览窗口 + join 消息泵（幂等）
#endif
    player.stop();

#if defined(_WIN32)
    // ---- 自重启接力（M5）：资源已全部析构（上方退出链），此处拉起新
    // 实例（原样命令行 + --restart-wait-pid 接力等待）后本进程结束。
    // 【为何不在 restart_fn 里就地重启】exit 跳过 main 栈对象析构（GL
    // 上下文/接收线程/音频引擎），且新实例会与未退出的旧实例撞端口。
    if (restart_requested.load()) {
        wchar_t exe[MAX_PATH]{};
        ::GetModuleFileNameW(nullptr, exe, MAX_PATH);
        // 原样转发完整命令行（含 --config 等），追加接力等待参数。
        // 【--key=value 形式】parseArgs 的 value_of 只认 --key= 前缀——
        // 空格分隔（--restart-wait-pid 123）会落"未知参数"退出（实测踩坑）。
        std::wstring cmd = ::GetCommandLineW();
        cmd += L" --restart-wait-pid=" +
               std::to_wstring(::GetCurrentProcessId());
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        // CREATE_BREAKAWAY_FROM_JOB：新实例脱离本进程所在 job（沙箱/任务
        // 调度器多为 kill-on-close 进程树——旧实例退出触发树清理时会连带
        // 杀掉尚未 breakaway 的新实例，实测踩坑）。job 未授权 breakaway
        // 时该标志报拒绝 → 退回普通创建（功能仍通，仅无树脱离）。
        const char* how = "breakaway";
        if (!::CreateProcessW(exe, cmd.data(), nullptr, nullptr, FALSE,
                              CREATE_BREAKAWAY_FROM_JOB, nullptr, nullptr,
                              &si, &pi)) {
            how = "plain";
            if (!::CreateProcessW(exe, cmd.data(), nullptr, nullptr, FALSE,
                                  0, nullptr, nullptr, &si, &pi)) {
                std::fprintf(stderr,
                             "[display] 自重启失败：CreateProcessW 错误码 %lu"
                             "（本次仅退出，请手动启动）\n", ::GetLastError());
                std::fflush(stderr);
                return 0;
            }
        }
        std::printf("[display] 新实例已拉起（pid=%lu，%s）→ 本进程退出\n",
                    pi.dwProcessId, how);
        ::CloseHandle(pi.hThread);
        ::CloseHandle(pi.hProcess);
        std::fflush(stdout);
        std::fflush(stderr);
    }
#endif
    return 0;
}

} // namespace

// ---------------------------------------------------------------------------
// main：display 端入口（编辑端见 media_editor.exe / src/editor）
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    // 控制台代码页 → UTF-8：源码 /utf-8 编译输出 UTF-8 字节流，Windows
    // 控制台默认 GBK(936) 按其解释必乱码（详见 platform/ConsoleUtf8.hpp）
    ms::platform::setupConsoleUtf8();

#ifdef _WIN32
    // 【注入噪音防御 2026-11-05】display 是纯显示进程，没有任何文本输入
    // ——显式禁用主线程输入法（IME）。动机：部分国产输入法的广告同步
    // 组件（AdSync，log4cplus 打点、自带老版 Qt）会随窗口创建被系统
    // 注入 GUI 进程，其初始化在主线程同步执行并向控制台刷
    // "log4cplus:ERROR ... AdSyncNamespace" + "Qt: Untested Windows
    // version 6.2 detected!"（多次窗口创建即反复打印）——用户实测每次
    // 打印伴随一次界面卡顿（同步控制台 I/O + 组件初始化都落在主线程）。
    // 禁用 IME 后窗口创建不再触发输入法组件激活，打印与卡顿一并消失。
    // 运行时挂接（不引入 imm32 链接依赖）；imm32 缺失/函数不存在静默跳过。
    // 【时序要求】ImmDisableIME 必须在线程创建任何窗口之前调用——此处为
    // main 首段，先于全部窗口创建，满足前提。
    {
        using ImmDisableIMEFn = int(WINAPI*)(DWORD);
        // 不 FreeLibrary：禁用语义要求 imm32 映射存活到窗口创建期
        HMODULE imm = ::LoadLibraryW(L"imm32.dll");
        if (imm != nullptr) {
            const auto disable = reinterpret_cast<ImmDisableIMEFn>(
                ::GetProcAddress(imm, "ImmDisableIME"));
            if (disable != nullptr) disable(0); // 0 = 当前（主）线程
        }
    }
#endif

    // ---- 启动配置：--config= 显式 > exe 同目录 config.ini > 工作目录 ------
    std::string config_override; // --config= 显式路径（优先级最高）
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind("--config=", 0) == 0) config_override = a.substr(9);
    }
    const std::string exe_dir = ms::config::exeDirectory();
    std::vector<std::string> candidates;
    if (!config_override.empty()) candidates.push_back(config_override);
    if (!exe_dir.empty()) candidates.push_back(exe_dir + "/config.ini");
    candidates.push_back("config.ini"); // 工作目录兜底
    const std::string config_path = ms::config::findConfigFile(candidates);
    const auto ini = config_path.empty()
                         ? ms::config::IniConfig{} // 无配置 = 全内置默认
                         : ms::config::IniConfig::load(config_path);
    if (!config_path.empty()) {
        std::fprintf(stderr, "[display] 配置文件：%s\n",
                     config_path.c_str());
    }
    std::fflush(stderr);

    // 写回目标（运行期持久化设置用，如指派 id）：优先跟随已加载的配置；
    // 无配置文件时落到 exe 目录（分发布局下 config.ini 与 exe 同目录，
    // 重启即被读取；exeDirectory 失败再退工作目录 config.ini）
    const std::string config_write_path = !config_path.empty()
                                              ? config_path
                                              : (!exe_dir.empty()
                                                     ? exe_dir + "/config.ini"
                                                     : std::string("config.ini"));
    // 【2026-11-13 修复】启动即确保 config.ini 文件存在（空文件即可——
    // saveKeyToFile 对"不存在=新建仅含目标节"）。实证坑：文件缺位时
    // MpcdiService::set_paths 的 is_regular_file 判空走"当目录"分支，
    // create_directories 会把 config.ini 当目录建出 config.ini/mpcdi_cache/，
    // 此后一切配置写回（settings PUT / set-default / 指派 id 持久化）
    // 全部"落盘失败"（路径已是被目录占用的名字）。
    if (!std::filesystem::exists(std::filesystem::path(
            reinterpret_cast<const char8_t*>(config_write_path.c_str())))) {
        std::ofstream touch(
            std::filesystem::path(reinterpret_cast<const char8_t*>(
                config_write_path.c_str())),
            std::ios::binary);
        // 打开失败不阻断启动（只读模式照常跑，写回时报错可见）
    }

    const Args ini_defaults = loadDefaultsFromIni(ini);
    const auto args = parseArgs(argc, argv, ini_defaults);
    if (!args) return 1;
    if (args->help) {
        printUsage();
        return 0;
    }

    std::printf("[media_display] 显示端 SLAVE 引擎同源构建 EngineCore"
                "（监听模式：打开即监听，等 Editor 接入）\n");
    return runDisplay(*args, ini, config_write_path);
}
