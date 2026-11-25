// ============================================================================
//  TextRasterizer 实现 —— stb_truetype 光栅化 + 排版（无 Qt / 无平台 API）
//
//  实现要点（对照头注释）：
//    * 字体定位（2026-11-24 内置保底改造）：候选目录链 = MS_FONT_DIR
//      注入 → exe 相对 fonts/（内置字体自含分发约定）→ 系统目录
//      （Windows=WINDIR/Fonts；POSIX=/system/fonts）；【显示名 → 文件名】
//      映射表（fontTable）+ 名字级回退链（雅黑→黑体→内置思源→Arial）
//      + 文件级终极保底（内置思源黑体——请求文件在所有目录缺失时的
//      确定性终点，目标机"必能渲染中文"不再依赖系统字体状态）；
//    * 进程级字体缓存：文件字节 + stbtt_fontinfo 只读共享（stbtt 查询
//      无可变全局态，跨线程并发安全）；加载失败记负缓存避免反复磁盘探测；
//    * 排版：显式 '\n' 分行（\r 忽略、\t 展开 4 空格），不做自动折行
//      （框宽拉伸归 clip transform——与图片素材"自然尺寸 + 缩放"同构）；
//      行距统一 ≥1.12 倍字面高（中文字体 lineGap 普遍为 0 且不稳定，
//      统一呼吸空间观感一致、两端同源）；
//    * 仿斜体：stb_truetype 无"任意轮廓光栅化"公开 API，采用字形位图
//      合成期逐行错位（skew≈tan12°，顶部右移）——质量/复杂度平衡点；
//    * 仿粗体：同字形位图 x 向 -1/0/+1px 三次 over 叠加（侧写权重 0.5，
//      经典 fake-bold）。
// ============================================================================

#include "media/TextRasterizer.hpp"

#include "config/IniConfig.hpp" // exeDirectory（内置字体 exe 相对定位）

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <mutex>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
// stb_truetype 内部大量窄化转换（字形坐标 int↔float 等），库自身语义
// 如此；单 TU 包裹关闭，避免刷屏淹没工程告警
#pragma warning(disable : 4244 4267 4245 4456 4701 4702 4706)
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert(x) (void)(x) // 零依赖（不引 <assert.h>；库约定可替换）
#include "stb_truetype.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace ms {

namespace {

// ===========================================================================
// 字体表与进程级缓存
// ===========================================================================

/// 字体显示名（属性面板下拉项，键已去空白）→ 字体文件（相对 Fonts 目录）。
/// bold = nullptr 表示无粗体变体文件 → 常规文件 + 仿粗叠加。
struct FontFiles {
    const char* regular;
    const char* bold;
};

#ifdef __ANDROID__
// ===========================================================================
// 【Android 字体链 2026-11-24】FlowDisplayer（Pura X 真机联调）：
// 系统开源字体承接 Windows 字体名——fontTable 键（asset.text_font 值）与
// Windows 版同源（编辑器下拉同源不动），仅「文件名」换平台实现：
//   * CJK 名族（雅黑/黑体/宋体…）→ NotoSansCJK（思源黑体同源，观感最
//     接近黑体/雅黑；Android 7+ 恒有，卓易通容器实测在）；
//   * Latin 名族 → Roboto（用 Static 实例——可变字体版 stbtt hmtx 读取
//     失效 advance 全错，模拟器 OSD 实测教训，见 OsdRasterizer.cpp）；
//   * 请求文件缺失时 loadFont 还有文件级兜底链（kPosixFontFallbacks）。
// bold = nullptr → 常规文件 + 仿粗叠加（与 Windows 表同语义）。
const std::map<std::string, FontFiles>& fontTable() {
    static const std::map<std::string, FontFiles> kTable = {
        {"MicrosoftYaHei",  {"NotoSansCJK-Regular.ttc", "NotoSansCJK-Bold.ttc"}},
        {"MicrosoftYaHeiUI",{"NotoSansCJK-Regular.ttc", "NotoSansCJK-Bold.ttc"}},
        {"SimHei",          {"NotoSansCJK-Regular.ttc", "NotoSansCJK-Bold.ttc"}},
        {"SimSun",          {"NotoSansCJK-Regular.ttc", nullptr}},
        {"KaiTi",           {"NotoSansCJK-Regular.ttc", nullptr}},
        {"FangSong",        {"NotoSansCJK-Regular.ttc", nullptr}},
        {"SourceHanSansSC", {"SourceHanSansSC-Regular-2.otf", nullptr}},
        {"Arial",           {"RobotoStatic-Regular.ttf", "Roboto-Bold.ttf"}},
        {"SegoeUI",         {"RobotoStatic-Regular.ttf", "Roboto-Bold.ttf"}},
        {"Consolas",        {"DroidSansMono.ttf", nullptr}},
        {"TimesNewRoman",   {"NotoSerif-Regular.ttf", "NotoSerif-Bold.ttf"}},
    };
    return kTable;
}
#else
const std::map<std::string, FontFiles>& fontTable() {
    static const std::map<std::string, FontFiles> kTable = {
        {"MicrosoftYaHei",  {"msyh.ttc", "msyhbd.ttc"}},
        {"MicrosoftYaHeiUI", {"msyh.ttc", "msyhbd.ttc"}},
        {"SimHei",          {"simhei.ttf", nullptr}},
        {"SimSun",          {"simsun.ttc", nullptr}},
        {"KaiTi",           {"simkai.ttf", nullptr}},
        {"FangSong",        {"simfang.ttf", nullptr}},
        {"SourceHanSansSC", {"SourceHanSansSC-Regular-2.otf", nullptr}},
        {"Arial",           {"arial.ttf", "arialbd.ttf"}},
        {"SegoeUI",         {"segoeui.ttf", "segoeuib.ttf"}},
        {"Consolas",        {"consol.ttf", "consolbd.ttf"}},
        {"TimesNewRoman",   {"times.ttf", "timesbd.ttf"}},
    };
    return kTable;
}
#endif // __ANDROID__

/// 回退顺序（映射表未命中时逐级尝试；雅黑为中文兜底）。内置思源排在
/// Arial 之前：系统无任何 CJK 字体时（非中文 Windows 精简机/POSIX 极简
/// 镜像）CJK 内容落内置保底而非 Arial 的豆腐块；Arial 殿后仅防内置
/// 文件损坏（理论不可达——文件级终极保底见 loadFont）。
const char* const kFallbackFonts[] = {
    "MicrosoftYaHei", "SimHei", "SourceHanSansSC", "Arial"};

/// 仿斜体错位系数（≈ tan 12°，观感对齐常见伪斜体）
constexpr float kItalicSkew = 0.2126f;

std::mutex& fontMutex() {
    static std::mutex m;
    return m;
}

/// 已加载字体：文件字节 + stb 解析上下文（TTC 取第 0 个字体）
struct LoadedFont {
    std::vector<unsigned char> data;
    stbtt_fontinfo info{};
};

std::map<std::string, LoadedFont>& fontCache() {
    static std::map<std::string, LoadedFont> cache;
    return cache;
}

/// 负缓存：加载失败的文件名（防反复磁盘探测；只关心存在性，用 set）
std::set<std::string>& failedCache() {
    static std::set<std::string> failed;
    return failed;
}

/// 显示名归一（剔除空白；映射键均为无空白键）
std::string normalizeName(const std::string& in) {
    std::string out;
    for (char c : in)
        if (c != ' ' && c != '\t') out += c;
    return out;
}

/// 显示名（可能未知/空）→ 字体文件名：查表 + 回退链
bool resolveFontFile(const std::string& display, bool bold,
                     std::string& file) {
    const std::string name = normalizeName(display);
    std::vector<std::string> candidates;
    if (!name.empty()) candidates.push_back(name);
    for (const char* fb : kFallbackFonts) candidates.push_back(fb);
    for (const auto& cand : candidates) {
        const auto it = fontTable().find(cand);
        if (it == fontTable().end()) continue;
        const char* f = bold ? it->second.bold : it->second.regular;
        if (!f) f = it->second.regular; // 无粗体变体 → 常规文件（仿粗兜底）
        file = f;
        return true;
    }
    return false;
}

/// 单文件尝试（调用方持 fontMutex）：读字节 + stbtt 解析；成功填充 lf
bool readFontFile(const std::string& path, LoadedFont& lf) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0, std::ios::end);
    const std::streamoff sz = f.tellg();
    f.seekg(0, std::ios::beg);
    if (sz <= 0 || sz > (1 << 26)) return false; // 上限 64MB：防御异常文件
    lf.data.resize(static_cast<std::size_t>(sz));
    f.read(reinterpret_cast<char*>(lf.data.data()),
           static_cast<std::streamsize>(sz));
    if (!f) return false;
    // TTC 集合取第 0 个字体（非集合文件该调用返回 0，语义一致）
    const int offset = stbtt_GetFontOffsetForIndex(lf.data.data(), 0);
    return offset >= 0 && stbtt_InitFont(&lf.info, lf.data.data(), offset);
}

#ifdef __ANDROID__
/// POSIX/Android 文件级兜底链（fontTable 映射后的文件仍缺失时逐个尝试，
/// 首个 stbtt 可解析的胜出）：CJK 优先（对齐 kFallbackFonts 的中文兜底
/// 语义），Latin 次之。覆盖厂商裁剪字体/极简镜像——文字片段宁用替代
/// 字形也绝不静默全灭（Pura X 真机实证：不兜底则黑体/雅黑全不显示）。
/// 终极保底（内置思源）在 loadFont 统一追加，此处只列系统字体名。
const char* const kPosixFontFallbacks[] = {
    "NotoSansCJK-Regular.ttc", "NotoSansCJK-Bold.ttc",
    "NotoSansSC-Regular.otf",
    "RobotoStatic-Regular.ttf", "Roboto-Regular.ttf",
    "DroidSans.ttf",
};
#endif

/// 文件级终极保底（内置分发，两端同文件——"目标机必能渲染中文"的
/// 确定性底座；2026-11-24）。OFL 授权（思源黑体 v2，可随应用分发）；
/// bold 无独立文件 → 常规文件 + 仿粗叠加（与映射表同语义）。文件由
/// 分发方放置：编辑器 build/bin/fonts、display bin/display/fonts（或
/// 上级 bin/fonts 共用一份，exe 相对解析见 fontDirs）。
const char* const kTerminalFontFile = "SourceHanSansSC-Regular-2.otf";

/// 候选目录（按优先级；2026-11-24 内置保底改造）：
///   1. MS_FONT_DIR 注入（两端一致；沙箱/容器/测试覆盖——语义自此从
///      "唯一目录"放宽为"首目录"，注入目录缺失时继续走后续候选，
///      兜底只会更稳不会更脆）；
///   2. exe 相对 fonts/ 与 ../fonts（自含分发约定：编辑器 bin/fonts、
///      display bin/display/fonts 或 bin/display/../fonts 共用 bin/fonts）；
///   3. 平台系统目录殿后（Windows=WINDIR/Fonts；POSIX=/system/fonts）
///      ——系统字体风格丰富但目标机不可控（精简 ROM/语言包缺失实证）。
/// 内置目录排在系统目录之前：同一逻辑名优先解析到同一物理文件，
/// 两端像素一致（WYSIWYG）；系统目录命中则沿用本机字体观感。
std::vector<std::string> fontDirs() {
    std::vector<std::string> dirs;
    if (const char* env = std::getenv("MS_FONT_DIR"); env && *env)
        dirs.emplace_back(env);
#ifdef _WIN32
    // 【ACP 窄字节豁免，同 WINDIR 语义】exeDirectory 产物（GetModule-
    // FileNameA）与环境变量均为 ACP 窄字节，窄串直传 ifstream 与 ACP
    // 语义一致；强转 UTF-8 反而损坏字节（见 WINDIR 豁免注释）。
    const std::string exe = config::exeDirectory();
    if (!exe.empty()) {
        dirs.push_back(exe + "/fonts");
        dirs.push_back(exe + "/../fonts");
    }
    const char* windir = std::getenv("WINDIR");
    std::string sysdir = (windir && *windir) ? windir : "C:\\Windows";
    std::replace(sysdir.begin(), sysdir.end(), '\\', '/');
    dirs.push_back(sysdir + "/Fonts");
#else
    const std::string exe = config::exeDirectory();
    if (!exe.empty()) {
        dirs.push_back(exe + "/fonts");
        dirs.push_back(exe + "/../fonts");
    }
    dirs.emplace_back("/system/fonts");
#endif
    return dirs;
}

/// 加载字体文件 + 初始化 stb 上下文（线程安全；失败进负缓存）
///
/// 文件候选（按序）：主名（fontTable 映射产物）→ 平台文件级兜底链
///（Android 既有）→ 终极保底 kTerminalFontFile（两端统一；Windows
/// 2026-11-24 起同享——此前 Windows 单文件单目录，表内字体缺文件即
/// 渲染失败，名字级回退救不了"表命中但文件缺"，如英文系统 KaiTi）。
/// 命中后以实际文件名入 cache（下次直接命中）；负缓存只记主名——
/// 兜底链保留试探（几个 stat 的代价，POSIX 既有语义推广到两端）。

const stbtt_fontinfo* loadFont(const std::string& file) {
    std::lock_guard<std::mutex> lock(fontMutex());
    auto& cache = fontCache();
    if (const auto it = cache.find(file); it != cache.end())
        return &it->second.info;
    if (failedCache().count(file)) return nullptr;

    // 文件候选链：主名 → 平台文件级兜底链（Android 既有）→ 终极保底
    //（两端统一；主名与保底同名时自然去重）
    std::vector<std::string> candidates{file};
#ifdef __ANDROID__
    for (const char* fb : kPosixFontFallbacks) candidates.emplace_back(fb);
#endif
    if (file != kTerminalFontFile) candidates.emplace_back(kTerminalFontFile);

    // 目录 × 文件 候选矩阵（目录序见 fontDirs；命中以实际文件名入 cache）
    // 【u8Path 豁免】目录产物（WINDIR/exe 路径/env）与文件名均为 ACP
    // 窄字节（英文 Windows 恒 ASCII；中文 Windows 系统目录与常见安装
    // 目录亦为 ACP 可表示命名），窄串直传 ifstream 与 ACP 语义一致；
    // 此处若强转 UTF-8 反而损坏字节（同 WINDIR 豁免注释）。
    const std::vector<std::string> dirs = fontDirs();
    for (const auto& name : candidates) {
        if (const auto hit = cache.find(name); hit != cache.end())
            return &hit->second.info;
        for (const auto& dir : dirs) {
            LoadedFont lf;
            if (!readFontFile(dir + "/" + name, lf)) continue;
            auto [pos, inserted] = cache.emplace(name, std::move(lf));
            (void)inserted;
            return &pos->second.info;
        }
    }
    failedCache().insert(file); // 只负缓存主名：兜底链保留试探
    return nullptr;
}

/// 排版上下文（measure / render 共用的字体度量）
struct FontCtx {
    const stbtt_fontinfo* font = nullptr;
    float scale       = 0.f; ///< 字号 px → stbtt 字体单位换算
    int   ascent      = 0;   ///< 基线上方高度（px，正值）
    int   descent     = 0;   ///< 基线下方高度（px，负值）
    int   face        = 0;   ///< 字面高 ascent-descent
    int   line_height = 0;   ///< 行距（px）
};

bool buildFontCtx(const TextStyle& st, FontCtx& out) {
    std::string file;
    if (!resolveFontFile(st.font, st.bold, file)) return false;
    const stbtt_fontinfo* f = loadFont(file);
    if (!f) return false;
    out.font  = f;
    out.scale = stbtt_ScaleForPixelHeight(
        f, static_cast<float>(std::clamp(st.font_size, 4, 4096)));
    // 【修复 2026-11-05】stbtt_GetFontVMetrics 返回【字体原始单位】
    // （font units，雅黑 upem=2048 下 face≈2700 单位），必须乘 scale
    // 换算成像素——首版漏乘导致：96px 两行文本帧高 5761px（放大 22 倍）、
    // 基线错位到帧外（字形投影错位、编辑器 3D 视口把超长帧压进小矩形
    // 后文本细如发丝不可见）。联测探针 measure=735x5761 lines=2 实锤。
    int ascent_u = 0, descent_u = 0;
    stbtt_GetFontVMetrics(f, &ascent_u, &descent_u, nullptr);
    out.ascent  = static_cast<int>(std::lround(ascent_u * out.scale));
    out.descent = static_cast<int>(std::lround(descent_u * out.scale));
    out.face = out.ascent - out.descent; // descent 为负
    if (out.face <= 0) out.face = std::clamp(st.font_size, 4, 4096);
    // 行距：中文字体 lineGap 普遍为 0 → 贴行可读性差；统一 ≥1.12 倍
    // 字面高（不用字体自带 lineGap——常见字体该值不稳定，统一呼吸空间
    // 观感一致，且两端同源）
    out.line_height = out.face + std::max(1, out.face / 8);
    return true;
}

/// UTF-8 → codepoint（非法序列容错：消费 1 字节返回 U+FFFD；返回后
/// i 指向下一字节。ASCII 控制码不会出现在 UTF-8 多字节序列内部，
/// 逐字节读取安全）
std::uint32_t decodeUtf8(const std::string& s, std::size_t& i) {
    const unsigned char c0 = static_cast<unsigned char>(s[i]);
    auto cont = [&](std::size_t k) {
        return i + k < s.size() &&
               (static_cast<unsigned char>(s[i + k]) & 0xC0) == 0x80;
    };
    if (c0 < 0x80) { ++i; return c0; }
    if ((c0 & 0xE0) == 0xC0 && cont(1)) {
        const std::uint32_t cp =
            ((c0 & 0x1Fu) << 6) |
            (static_cast<unsigned char>(s[i + 1]) & 0x3Fu);
        i += 2;
        return cp;
    }
    if ((c0 & 0xF0) == 0xE0 && cont(1) && cont(2)) {
        const std::uint32_t cp = ((c0 & 0x0Fu) << 12) |
                                 ((static_cast<unsigned char>(s[i + 1]) & 0x3Fu) << 6) |
                                 (static_cast<unsigned char>(s[i + 2]) & 0x3Fu);
        i += 3;
        return cp;
    }
    if ((c0 & 0xF8) == 0xF0 && cont(1) && cont(2) && cont(3)) {
        const std::uint32_t cp = ((c0 & 0x07u) << 18) |
                                 ((static_cast<unsigned char>(s[i + 1]) & 0x3Fu) << 12) |
                                 ((static_cast<unsigned char>(s[i + 2]) & 0x3Fu) << 6) |
                                 (static_cast<unsigned char>(s[i + 3]) & 0x3Fu);
        i += 4;
        return cp;
    }
    ++i;
    return 0xFFFDu; // 非法序列 → 替换字形
}

/// 按行拆分：'\n' 分行、'\r' 忽略、'\t' 展开 4 空格
std::vector<std::string> splitLines(const std::string& content) {
    std::vector<std::string> lines;
    std::string cur;
    for (char c : content) {
        if (c == '\n') {
            lines.push_back(std::move(cur));
            cur.clear();
        } else if (c == '\r') {
            // 忽略（Windows CRLF 的 CR 半边）
        } else if (c == '\t') {
            cur += "    ";
        } else {
            cur += c;
        }
    }
    lines.push_back(std::move(cur)); // 末行（无尾换行也有内容行）
    return lines;
}

/// 单行宽度（advance 累加 + kerning；px）
float measureLineWidth(const FontCtx& fc, const std::string& line) {
    float width = 0.f;
    std::uint32_t prev = 0;
    std::size_t i = 0;
    while (i < line.size()) {
        const std::uint32_t cp = decodeUtf8(line, i);
        int advance = 0;
        stbtt_GetCodepointHMetrics(fc.font, cp, &advance, nullptr);
        if (prev != 0)
            width += static_cast<float>(
                         stbtt_GetCodepointKernAdvance(fc.font, prev, cp)) *
                     fc.scale;
        width += static_cast<float>(advance) * fc.scale;
        prev = cp;
    }
    return width;
}

/// 整体排版度量：帧自然尺寸（含四周 pad + 斜体宽度余量）
struct TextMetrics {
    int width = 0, height = 0, lines = 0;
};

TextMetrics layoutMetrics(const FontCtx& fc, const TextStyle& st) {
    const int pad = std::max(2, st.font_size / 10); // 四周留白（抗锯齿边 +
                                                    // 负 bearing 防裁切）
    const auto lines = splitLines(st.content);
    float max_w = 0.f;
    for (const auto& ln : lines)
        max_w = std::max(max_w, measureLineWidth(fc, ln));
    const int skew_extra =
        st.italic ? static_cast<int>(std::ceil(kItalicSkew * fc.face)) : 0;
    TextMetrics m;
    m.lines  = static_cast<int>(std::max<std::size_t>(1, lines.size()));
    m.width  = pad * 2 + static_cast<int>(std::ceil(max_w)) + skew_extra;
    m.height = pad * 2 + (m.lines - 1) * fc.line_height + fc.face;
    return m;
}

/// straight-alpha 前景像素合成（over 运算；fake-bold 多次叠加自然增粗）
inline void blendPixel(std::vector<std::uint8_t>& rgba, int w, int x, int y,
                       int fr, int fg, int fb, float sa) {
    std::uint8_t* p = &rgba[(static_cast<std::size_t>(y) * w + x) * 4];
    const float da = p[3] / 255.0f;
    const float oa = sa + da * (1.0f - sa);
    if (oa <= 0.0f) return;
    p[0] = static_cast<std::uint8_t>(
        std::lround((fr * sa + p[0] * da * (1.0f - sa)) / oa));
    p[1] = static_cast<std::uint8_t>(
        std::lround((fg * sa + p[1] * da * (1.0f - sa)) / oa));
    p[2] = static_cast<std::uint8_t>(
        std::lround((fb * sa + p[2] * da * (1.0f - sa)) / oa));
    p[3] = static_cast<std::uint8_t>(std::lround(oa * 255.0f));
}

} // namespace

// ===========================================================================
// 公开 API
// ===========================================================================

TextStyle TextRasterizer::styleFromAsset(const Asset& a) {
    TextStyle st;
    st.content   = a.text_content;
    st.font      = a.text_font;
    st.font_size = a.text_font_size > 0 ? a.text_font_size : 48;
    st.color     = a.text_color;
    st.bold      = a.text_bold;
    st.italic    = a.text_italic;
    st.h_align   = std::clamp(a.text_h_align, 0, 2);
    st.bg_color  = a.text_bg_color;
    return st;
}

bool TextRasterizer::fontAvailable(const std::string& display_name) {
    std::string file;
    if (!resolveFontFile(display_name, false, file)) return false;
    return loadFont(file) != nullptr;
}

std::vector<std::string> TextRasterizer::fontCatalog() {
    std::vector<std::string> names;
    names.reserve(fontTable().size());
    for (const auto& kv : fontTable())
        names.push_back(kv.first); // std::map 序 = 字母序（下拉稳定）
    return names;
}

TextLayout TextRasterizer::measure(const TextStyle& st) {
    TextLayout out;
    FontCtx fc;
    if (!buildFontCtx(st, fc)) return out; // 无可用字体 → {0,0,0}（占位）
    const TextMetrics m = layoutMetrics(fc, st);
    out.width  = m.width;
    out.height = m.height;
    out.lines  = m.lines;
    return out;
}

bool TextRasterizer::render(const TextStyle& st,
                            std::vector<std::uint8_t>& rgba,
                            int& out_w, int& out_h) {
    out_w = out_h = 0;
    FontCtx fc;
    if (!buildFontCtx(st, fc)) return false;
    const TextMetrics m = layoutMetrics(fc, st);
    // 尺寸防御：异常巨帧（超长内容/异常字号）直接拒绝——上层占位显示
    constexpr int kMaxDim = 16384;
    if (m.width <= 0 || m.height <= 0 || m.width > kMaxDim ||
        m.height > kMaxDim)
        return false;

    const int w = m.width;
    const int h = m.height;
    const int pad = std::max(2, st.font_size / 10);
    rgba.assign(static_cast<std::size_t>(w) * h * 4, 0);

    // ---- 背景填充（0xRRGGBB；0 = 透明底，文本形状由 alpha 承载）--------
    if (st.bg_color != 0) {
        const std::uint8_t br = static_cast<std::uint8_t>((st.bg_color >> 16) & 0xFF);
        const std::uint8_t bg = static_cast<std::uint8_t>((st.bg_color >> 8) & 0xFF);
        const std::uint8_t bb = static_cast<std::uint8_t>(st.bg_color & 0xFF);
        for (std::size_t p = 0; p + 3 < rgba.size(); p += 4) {
            rgba[p]     = br;
            rgba[p + 1] = bg;
            rgba[p + 2] = bb;
            rgba[p + 3] = 255;
        }
    }

    // 前景色通道（straight alpha over 的源色）
    const int fr = static_cast<int>((st.color >> 16) & 0xFF);
    const int fgc = static_cast<int>((st.color >> 8) & 0xFF);
    const int fbc = static_cast<int>(st.color & 0xFF);

    const auto lines = splitLines(st.content);
    const int line_count =
        static_cast<int>(std::max<std::size_t>(1, lines.size()));
    const int skew_extra =
        st.italic ? static_cast<int>(std::ceil(kItalicSkew * fc.face)) : 0;
    const int inner_w = std::max(0, w - pad * 2 - skew_extra);

    for (int li = 0; li < line_count; ++li) {
        const std::string& ln = lines[li];
        const float lw = measureLineWidth(fc, ln);
        // 行内水平对齐（自然尺寸帧：非最长行才有对齐余量）
        int x0 = pad;
        if (st.h_align == 1)
            x0 = pad + (inner_w - static_cast<int>(lw)) / 2;
        else if (st.h_align == 2)
            x0 = pad + inner_w - static_cast<int>(lw);
        if (x0 < 0) x0 = 0;
        const int baseline = pad + fc.ascent + li * fc.line_height;

        float pen_x = static_cast<float>(x0);
        std::uint32_t prev_cp = 0;
        std::size_t pos = 0;
        while (pos < ln.size()) {
            const std::uint32_t cp = decodeUtf8(ln, pos);
            if (prev_cp != 0)
                pen_x += static_cast<float>(
                             stbtt_GetCodepointKernAdvance(fc.font, prev_cp,
                                                           cp)) *
                         fc.scale;
            prev_cp = cp;

            // 字形位图盒（相对基线的整数偏移；笔位分数并入亚像素参数）
            int ix0 = 0, iy0 = 0, ix1 = 0, iy1 = 0;
            const float frac_x = pen_x - std::floor(pen_x);
            stbtt_GetCodepointBitmapBoxSubpixel(fc.font, cp, fc.scale,
                                                fc.scale, frac_x, 0.f, &ix0,
                                                &iy0, &ix1, &iy1);
            const int bw = ix1 - ix0;
            const int bh = iy1 - iy0;
            int advance = 0;
            stbtt_GetCodepointHMetrics(fc.font, cp, &advance, nullptr);
            if (bw > 0 && bh > 0) {
                std::vector<unsigned char> glyph(
                    static_cast<std::size_t>(bw) * bh);
                stbtt_MakeCodepointBitmapSubpixel(
                    fc.font, glyph.data(), bw, bh, bw, fc.scale, fc.scale,
                    frac_x, 0.f, cp);
                const int gx = static_cast<int>(std::floor(pen_x)) + ix0;
                const int gy = baseline + iy0;
                for (int v = 0; v < bh; ++v) {
                    const int dy = gy + v;
                    if (dy < 0 || dy >= h) continue;
                    // 仿斜体：目的行相对基线偏移决定本行右移（顶部右移
                    // ——iy0+v 为负（基线上方）时 xshift 为正）
                    const int xshift =
                        st.italic
                            ? static_cast<int>(std::lround(
                                  -kItalicSkew *
                                  static_cast<float>(iy0 + v)))
                            : 0;
                    for (int u = 0; u < bw; ++u) {
                        const int cov8 =
                            glyph[static_cast<std::size_t>(v) * bw + u];
                        if (cov8 == 0) continue;
                        const float cov = cov8 / 255.0f;
                        // 仿粗：x 向 -1/0/+1 三次 over 叠加（中缝满权、
                        // 侧写半权——观感自然增粗且两端一致）
                        const int reps = st.bold ? 3 : 1;
                        for (int rep = 0; rep < reps; ++rep) {
                            const int dx =
                                gx + u + xshift + (st.bold ? rep - 1 : 0);
                            if (dx < 0 || dx >= w) continue;
                            const float sa =
                                (st.bold && rep != 1) ? cov * 0.5f : cov;
                            blendPixel(rgba, w, dx, dy, fr, fgc, fbc, sa);
                        }
                    }
                }
            }
            pen_x += static_cast<float>(advance) * fc.scale;
        }
    }

    out_w = m.width;
    out_h = m.height;
    return true;
}

} // namespace ms
