// ============================================================================
//  TextRasterizer —— 文本排版光栅化器（TrueType，无 Qt / 无平台 API）
//
//  定位（Phase 4 文本媒体链路的渲染核心，2026-11-05）：
//    * 【两端同源】Editor 预览与 display 输出使用同一实现（engine_core），
//      相同 TextPayload 在两端产出逐位一致的帧——遵守"两端同源渲染"铁律；
//    * 光栅化底座 = stb_truetype v1.26（公共域单头库，vendor 于
//      3rdparty/stb/）：解析 C:\Windows\Fonts 系统字体（.ttf/.ttc），
//      8bpp 抗锯齿 coverage 光栅化，RGBA 合成由本类完成；
//    * 排版语义（对标 WATCHOUT 文本 cue 的静态文本能力）：
//        显式换行 '\n'（不做自动折行——框宽变化由 clip transform.w/h
//        拉伸承担，文本帧本身恒为"自然尺寸"，与图片素材同构）；
//        水平对齐（左/中/右；垂直方向随 ascent/descent 自然排布，
//        无对齐自由度）；粗体（优先换粗体
//        字体文件，无则仿粗叠加）；斜体（skew 仿斜）；可选背景色块；
//    * 字体名是【显示名 → 字体文件】的固定映射表（见 .cpp kFontTable）：
//      显示端可能没有编辑端选出的任意系统字体，故下拉只提供两端映射
//      表内字体；未知名字回退微软雅黑，雅黑缺失再回退黑体/任意可用。
//
//  线程契约：measure/render 为无状态静态接口（字体表只读共享，首次
//  加载持锁；stbtt 查询函数无共享可变状态，多线程并发安全）。
//  素材根目录外的文件读取仅限系统字体目录（Windows\Fonts），无 IO 风险。
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "model/SceneModel.hpp"

namespace ms {

/// 文本渲染输入（与 Asset.text_* 一一对应；styleFromAsset 负责搬运）
struct TextStyle {
    std::string content;      ///< UTF-8 文本内容
    std::string font;         ///< 字体显示名（映射表解析；空 = 默认雅黑）
    std::int32_t font_size = 48; ///< 字号（px，>0）
    std::uint32_t color    = 0xFFFFFF; ///< 文字颜色 0xRRGGBB
    bool bold   = false;      ///< 粗体（映射表粗体文件；缺文件仿粗叠加）
    bool italic = false;      ///< 斜体（仿斜 skew ≈12°）
    int h_align = 0;          ///< 水平对齐：0=左 1=中 2=右（多行行间对齐；
                              ///<  帧为自然尺寸，垂直方向无对齐自由度）
    std::uint32_t bg_color = 0; ///< 背景色 0xRRGGBB；0 = 透明背景
};

/// 排版结果：自然尺寸（帧尺寸；transform.w/h<=0 时即显示尺寸）
struct TextLayout {
    int width  = 0; ///< 帧宽（px）
    int height = 0; ///< 帧高（px）
    int lines  = 0; ///< 行数（诊断用）
};

class TextRasterizer {
public:
    /// 测量自然尺寸（排版纯计算，不产出像素）
    /// 无可用字体时返回 {0,0,0}——调用方（编辑器测量回填/源 prepare）
    /// 据此画占位；【两端同源】同一输入恒同一结果。
    static TextLayout measure(const TextStyle& st);

    /// 渲染 RGBA 帧（行主序，stride = width*4，无行填充，预乘不使用）。
    /// \param[out] rgba 输出缓冲（width*height*4 字节；复用调用方缓冲）
    /// \return false = 无可用字体（系统无映射表内字体且无兜底）——
    ///         上层保持占位显示，与视频源 prepare 失败同语义（可重试）
    static bool render(const TextStyle& st, std::vector<std::uint8_t>& rgba,
                       int& out_w, int& out_h);

    /// Asset.text_* → TextStyle（两端装配唯一入口，避免字段遗漏）
    static TextStyle styleFromAsset(const Asset& a);

    /// 字体显示名 → 字体文件解析是否可用（编辑器下拉构建/诊断用）
    static bool fontAvailable(const std::string& display_name);

    /// 字体下拉目录（映射表全部显示名，按表序返回；2026-11-05 文本片段
    /// 完整化）——编辑器属性面板下拉数据源：与渲染端同一张表，杜绝
    /// "UI 列表与解析表漂移"；display 端缺文件的字体渲染时自动回退
    /// 雅黑（见 .cpp 回退链），下拉无需按本机过滤。
    static std::vector<std::string> fontCatalog();
};

} // namespace ms
