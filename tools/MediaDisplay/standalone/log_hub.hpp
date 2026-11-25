// ============================================================================
//  log_hub —— 显示端日志枢纽（2026-11-24）：stdout/stderr 全量重定向进
//  环形缓冲，经 GET /api/v1/logs 增量下发网页「运行日志」窗口。
//
//  动机：FlowDisplay 转 Windows 子系统（无终端黑窗，见 MediaDisplay/
//  CMakeLists.txt）后，渲染/tick/recv 线程的 fprintf 输出不再有去处——
//  本模块接管全部 CRT stdout/stderr 写（含 ffmpeg av_log、std::filesystem
//  异常等三方库输出），网页成为日志唯一观看面；exe/logs/display.log
//  同步落盘兜底（HTTP 未起来时的排障通道，如端口占用致命错误）。
//
//  实现（Windows）：_pipe + _dup2 重定向 fd 1/2 → 后台读线程阻塞消费，
//  按 \n 拆行（UTF-8 多字节序列不含 0x0A，行拆分不撕码）；每行：
//    · 编码修正：非合法 UTF-8 字节（本地化 ACP 异常消息）→ acp_to_utf8；
//    · 级别粗判：含 error/fail/错误/失败 → "error"，warn/警告 → "warn"；
//    · 入环形缓冲（容量 4000 行，seq 全局单调）+ 文件 append + 可选 tee。
//
//  --console（tee_console=true）：AttachConsole(ATTACH_PARENT_PROCESS)
//  挂父控制台，行同步经 WriteConsoleW 输出（UTF-16 直写，中文不乱码，
//  不依赖控制台代码页）；同场禁用 QuickEdit（写控制台 I/O 被选择态
//  无限期挂起的既有铁律——tee 写线程同样受保护）。
//
//  线程模型：读线程唯一生产者（mutex 环形缓冲），HTTP 工作线程并发
//  snapshot() 消费；读线程随进程生命周期（detach——CRT 退出 ExitProcess
//  直接终结，无 join 需求，display 退出链无感知）。
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace media_api::log_hub {

/// 单行日志（网页渲染最小单元）。
struct Line {
    std::uint64_t seq = 0;        ///< 全局单调序号（1 起；snapshot 增量锚点）
    std::int64_t  ts_ms = 0;      ///< epoch 毫秒（本地时间格式化由网页承担）
    const char*   level = "info"; ///< "info" | "warn" | "error"
    std::string   text;           ///< UTF-8 单行（不含换行符）
};

/// 安装重定向（进程内一次；重复调用幂等返回）。
/// 必须在任何 fprintf/printf 之前调用（重定向后的写全部进枢纽）。
/// tee_console=true：尝试挂父控制台并同步回显（--console 终端调试形态；
/// 双击启动无父控制台时 attach 失败，静默退化为纯网页+文件）。
void install(bool tee_console);

/// 增量快照：返回 seq > since 的全部行（锁内拷贝）；last_seq = 当前最大
/// 序号（下轮 since 锚点）；dropped = 环形缓冲溢出丢弃累计行数（消费
/// 落后过多时 >0，网页可提示"日志有丢弃"）。线程安全。
void snapshot(std::uint64_t since, std::vector<Line>& out,
              std::uint64_t& last_seq, std::uint64_t& dropped);

/// 当前最大 seq（未安装时 0）。
std::uint64_t lastSeq();

}  // namespace media_api::log_hub
