// ============================================================================
//  IniConfig —— 轻量 INI 配置读写器（Editor / Display 两端同源共用）
//
//  定位（"所有设置不硬编码"改造，第五迭代补丁）：
//    * 两端启动设置收敛到 config.ini（[common]/[editor]/[display] 分段），
//      命令行参数仍可用且【显式项优先于 ini、ini 优先于内置默认】；
//    * 纯 C++ 无 Qt 依赖——display 端不带 Qt（§2.1 架构约束），故不用
//      QSettings，自实现一个最小解析器（约 100 行，行为两端完全一致）。
//    * 【写回 2026-11-06】运行期产生的持久化设置（如 display 采用 Editor
//      指派 id）由 saveKeyToFile 文本级读改写回——只替换目标键行，
//      原文件注释/未触及键/行尾风格全部保留（手写配置不被机器破坏）。
//
//  语法约定（刻意保持最小）：
//    * 节：[section]（节名大小写敏感——与键名一致按原样匹配，不做大小写
//      折叠，避免与 QtTimeLine/Qt 生态的约定打架）；
//    * 键值：key = value（= 两侧空白剥离；值内空白保留，可含中文路径）；
//    * 注释：整行 ';' 或 '#' 开头；行内注释：' ;' 或 ' #'"'"'（分号/井号
//      前有空白）——避免误伤含 '#' 的路径值；
//    * 编码：UTF-8（/utf-8 编译链下 std::string 直存字节）；自动跳过
//      UTF-8 BOM；引号不特殊处理（路径直接写，不带引号）。
//
//  查找约定（入口层实现，见 display/editor main.cpp）：
//    --config=<path> 显式指定 > exe 同目录 config.ini > 工作目录 config.ini；
//    文件不存在 = 空配置（全部回落内置默认，行为与旧版一致，不阻塞启动）。
// ============================================================================

#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ms::config {

class IniConfig {
public:
    /// 解析 ini 文件。\param path 文件不存在/不可读 → 空配置（不报错——
    /// 配置缺失是正常路径，调用方按内置默认运行）
    static IniConfig load(const std::string& path);

    /// 空配置（测试/兜底用）
    IniConfig() = default;

    /// 字符串值。\param fallback 键不存在（或节不存在）时的返回值
    std::string getStr(const std::string& section, const std::string& key,
                       const std::string& fallback = {}) const;

    /// 整数值：空值/非法数字回落 fallback（配置写坏不致命）
    long getLong(const std::string& section, const std::string& key,
                 long fallback = 0) const;

    /// 64 位整数值（时长/间隔类，单位见键名后缀 ms/us）
    std::int64_t getI64(const std::string& section, const std::string& key,
                        std::int64_t fallback = 0) const;

    /// 布尔值：1/true/yes/on（大小写不敏感）为真；0/false/no/off 为假；
    /// 其余回落 fallback
    bool getBool(const std::string& section, const std::string& key,
                 bool fallback = false) const;

private:
    /// 存储：section -> (key -> value)（均为 trim 后的 UTF-8 字节）
    std::map<std::string, std::map<std::string, std::string>> sections_;
};

/// 字符串两端空白剥离（ASCII 空白：空格/\t/\r/\n；值内空白保留）
std::string trim(const std::string& s);

/// 配置文件查找（入口层共用）：按候选顺序返回第一个存在的路径；
/// 都不存在返回空串（调用方按无配置运行）。候选由调用方给定——
/// display/editor 各自构造（exe 目录 / 工作目录 / --config= 显式项）。
std::string findConfigFile(const std::vector<std::string>& candidates);

/// 键值写回（文本级读改写，2026-11-06）：在 path 文件的 [section] 节内把
/// key 的值更新为 value，语义对齐 IniConfig::load（重复键行取最后者）。
/// 刻意做成【文本级改写】而非全量重序列化：原文件的注释、未触及键、
/// 行尾风格（CRLF/LF）全部原样保留——用户手写的配置注释不被抹掉。
///   * 键存在（含重复）→ 替换最后一处键值行为 "key = value"；
///   * 节存在键不存在 → 插入到节内最后一个键值行之后（无键则紧跟节头）；
///   * 节不存在 → 文件末尾追加空行 + [section] + key = value；
///   * 文件不存在 → 新建，仅含 [section]/key 两行。
/// \return false = 目录不可写/文件打不开（调用方决定告警级别；不抛异常）
bool saveKeyToFile(const std::string& path, const std::string& section,
                   const std::string& key, const std::string& value);

/// exe 所在目录（配置/插件的"随程序部署"定位基准；失败返回空串——
/// 调用方回落工作目录候选）。Windows 用 GetModuleFileNameA，Linux 用
/// /proc/self/exe；实现只在 .cpp，避免平台头泄漏到使用方。
std::string exeDirectory();

} // namespace ms::config
