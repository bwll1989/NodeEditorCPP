// ============================================================================
//  ShowFileIO.hpp —— .show 项目文件序列化/反序列化（第五迭代 Core 组件）
//
//  设计依据：《media-server-sync-design.md》§3 + 第五迭代需求
//    * 一个 .show 文件 = 一个完整项目：Scene（画布/屏幕/时间轴/素材）
//      + 显示端登记列表（DisplayEntry：editor 主动连接的目标）；
//    * Scene 的内存模型转换与引用完整性校验【复用 SceneLoader】
//      （toProto/fromProto——单一事实来源，避免两套映射漂移）；
//    * 文件格式（2026-11-04 JSON 化）：
//        - 新版：JSON 文本（protobuf 官方 JSON 映射，美化缩进 + 原始
//          snake_case 字段名 + 零值字段打印）——可读、可手工编辑、可 git
//          diff；顶层即 ShowDocument（{"scene":{...},"displays":[...]}），
//          中文等 UTF-8 内容原样保留。
//        - 旧版兼容：8 字节魔数 "MSSHOW\x01\x00" + ShowDocument pb 二进制
//          的历史文件【仍可加载】（kMagic 现仅供读取端识别旧格式）；
//        - 读取端按内容自适应：先查旧版魔数，否则按 JSON 解析——
//          两种格式都走 fromProto 的引用完整性校验（fail-fast）。
//
//  与 .scene 的关系：.scene 只承载 Scene（显示端运行时仍用），.show 是
//  编辑器的项目存档（Scene + displays）；Editor 打开 .show 后可把其中的
//  Scene 另存/下发给显示端。
// ============================================================================

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "model/SceneModel.hpp"
#include "scene.pb.h" // ms_proto::ShowDocument / DisplayEntry

namespace ms {

/// 显示端登记（内存模型；对应 scene.proto 的 ms_proto::DisplayEntry）
struct DisplayEntry {
    std::string   display_id;     ///< 显示端标识（= screen.player_id）
    std::string   host;           ///< 显示端主机（IP/主机名）
    std::uint16_t port = 17100;   ///< 显示端监听端口（media_display 默认 17100）
    bool          enabled = true; ///< false = editor 不连接/不同步
    /// 配对码（空 = 不认证；非空 = 连接后发 MsgAuth，须与显示端
    /// config.ini [display] pairing_code 一致，否则被显示端踢除）
    std::string   pairing_code;
};

/// 项目文档（内存模型）：场景 + 显示端登记
struct ShowDocument {
    Scene                     scene;
    std::vector<DisplayEntry> displays;
};

class ShowFileIO {
public:
    // ---- pb ↔ 内存模型（displays 部分无独立校验器，直接映射）--------------

    static ms_proto::ShowDocument toProto(const ShowDocument& doc);
    /// scene 部分经 SceneLoader::fromProto 做引用完整性校验
    static std::optional<ShowDocument> fromProto(const ms_proto::ShowDocument& pb,
                                                 std::string& err);

    // ---- 文件 IO ----------------------------------------------------------

    /// 保存为 .show 文件（JSON 文本，UTF-8；目录不存在自动创建）
    /// \return false = 序列化/打开/写入失败（err 给出原因）
    static bool saveToFile(const ShowDocument& doc, const std::string& path,
                           std::string& err);

    /// 从 .show 文件加载（双格式自适应：
    ///   旧版 = 8 字节魔数 + ShowDocument pb 二进制；
    ///   新版 = JSON 文本（顶层 ShowDocument）。
    /// 两种格式都过 Scene 引用完整性校验）
    /// \return nullopt = 文件不可读 / 内容损坏或场景校验失败（err 给出原因）
    static std::optional<ShowDocument> loadFromFile(const std::string& path,
                                                    std::string& err);

    // ---- 文本级 API（M5 工程互通：HTTP 导出/导入直用，免落临时文件）--------
    /// ShowDocument → .show JSON 文本（选项与 saveToFile 完全一致——
    /// 美化缩进 + snake_case + 零值字段打印；saveToFile 内部亦走此函数）
    static bool toJsonText(const ShowDocument& doc, std::string& out,
                           std::string& err);
    /// .show JSON 文本 → ShowDocument（不含旧版魔数分支——loadFromFile
    /// 先识别魔数，JSON 部分走此函数；过 Scene 引用完整性校验）
    static std::optional<ShowDocument> fromJsonText(const std::string& text,
                                                    std::string& err);

    /// 【旧格式识别】8 字节魔数（版本字节低 8 位；现仅读取端用于兼容
    /// 历史二进制 .show 文件——新版保存为 JSON 文本，不再写魔数）
    static constexpr char kMagic[8] = {'M', 'S', 'S', 'H', 'O', 'W',
                                       '\x01', '\x00'};
};

} // namespace ms
