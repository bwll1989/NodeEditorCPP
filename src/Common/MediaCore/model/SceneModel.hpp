// ============================================================================
//  SceneModel.hpp —— 场景运行时内存模型（纯 C++ 结构，不依赖 Qt / Protobuf）
//
//  设计依据：《media-server-sync-design.md》§3 / §4.2
//    * Protobuf（core/protocol/scene.proto）只是【序列化格式】；
//      本头文件是【运行时内存模型】：TimelineEngine、ScreenMapper、Renderer
//      等引擎组件全部面向本模型编程，对序列化格式零感知；
//    * SceneLoader（core/loader/）负责 pb ↔ 本模型的双向转换；
//    * 单位约定：时间一律 int64_t 微秒（us）；坐标一律 double（虚拟画布像素）。
//
//  生命周期约定：
//    * TimelineEngine::evaluate() 返回指向本模型内部的【借用指针】，
//      因此 Scene 实例必须在 evaluate 使用期间保持存活且不被修改；
//    * Scene 采用"值语义 + 索引复用"：clip 通过 asset_id 字符串引用素材表，
//      引用完整性由 SceneLoader 加载时校验（见 loader/SceneLoader.cpp）。
//
//  时间区间语义【前闭后开】：
//    t ∈ [start_us, start_us + duration_us) 时片段活跃；
//    即 t == start 活跃、t == end 不活跃——保证相邻片段在切换点不重叠。
// ============================================================================

#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ms {

// ---------------------------------------------------------------------------
// 素材种类 —— 与 scene.proto 的 AssetKind 枚举、MediaFactory 注册键一一对应
// （新增媒体类型：此处加枚举 + loader 加映射 + 工厂注册，§4.7）
// ---------------------------------------------------------------------------
enum class AssetKind : int {
    Unknown = 0, ///< 未知/未设置（解析容错）
    Video   = 1, ///< 视频文件
    Image   = 2, ///< 静态图片
    Text    = 3, ///< 文本
    Audio   = 4, ///< 音频（Player 仅消费时间信息，不出声，§4.4）
    Live    = 5, ///< 实时源（LiveHub：NDI/窗口捕获/Spout/网络流）——
                 ///< 无文件随场景 pb 分发；display 端统一经 LiveMediaSource
                 ///< 消费 editor 再发布的流（见 LiveHub 设计）
    Control = 6, ///< 控制指令（2026-11-13 Control 片段，合并前 Trigger 的
                 ///< 语义后继）——无媒体文件、无画布呈现；指令表存
                 ///< Clip.control_messages（JSON），仅编辑器端消费：播放头
                 ///< 上升沿进入片段 → 逐条灌 StatusContainer（statusUpdated
                 ///< → WS 8992 广播 / GlobalEventBus 控件联动）
};

// ---------------------------------------------------------------------------
// 变换 —— Clip 在虚拟画布上的几何/合成属性
// ---------------------------------------------------------------------------
struct Transform {
    double x        = 0.0;   ///< 画布坐标 X（左上角）
    double y        = 0.0;   ///< 画布坐标 Y（左上角）
    double w        = 0.0;   ///< 绘制宽度（<=0 表示沿用素材原始宽度）
    double h        = 0.0;   ///< 绘制高度（<=0 表示沿用素材原始高度）
    double rotation = 0.0;   ///< 旋转角度（度，顺时针，绕锚点）
    double opacity  = 1.0;   ///< 不透明度 [0,1]
    int32_t z       = 0;     ///< 叠放层级：小值先绘制（底层），大值后绘制（顶层）

    // ---- 锚点（2026-11-09 用户定案）：旋转/缩放的枢轴 ----------------------
    // 0=中心（默认，与历史"绕中心"行为逐位一致——安全阀）/ 1=左上 /
    // 2=右上 / 3=右下 / 4=左下。静态设置（非 tween 属性）；改变锚点不
    // 补偿位置（用户原话"旋转缩放以此为中心"= 求值语义）。
    int32_t anchor = 0;

    // ---- 四角点位移（2026-11-09 corner pin 前身，画布域 px）----------------
    // 下标 = 角索引×2 + (0=dx, 1=dy)；角索引序 = 左上0/右上1/右下2/左下3
    //（kCorner* 常量）。语义：未变形矩形（含旋转）四角各自平移——
    // 旋转非 0 时位移作用在【旋转后】的角上（屏幕空间拖角语义）。
    // 全 0 = 无变形（渲染走原快速路径，行为逐位不变）。
    double corner[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    // ---- 姿态扩展（2026-11-09 v3，DaVinci 五分组）：三轴旋转 --------------
    // rotation（上方）= 偏转/滚转（绕画布 Z 轴，2D 旋转）；rot_x = 俯仰
    //（绕画布 X 轴）、rot_y = 偏航（绕画布 Y 轴）。渲染端绕锚点做 3D
    // 旋转 + 透视投影推算四角（走既有变形 quad 管线；透视焦距与近似
    // 语义见 CanvasGeometry::quadCorners3D）。全 0 = 历史行为逐位不变。
    double rot_x = 0.0; ///< 俯仰旋转（度，绕画布 X 轴；正=顶边后仰）
    double rot_y = 0.0; ///< 偏航旋转（度，绕画布 Y 轴；正=右边缘远离）

    // ---- 裁切（2026-11-09 v3，画布 px）------------------------------------
    // 相对【未裁切】名义矩形边缘向内收：被裁区域不显示、内容不缩放，
    // 不回写 Pos/Size（与角点同"末端修正"哲学）。下标 = CropIdx。
    double crop[4] = {0, 0, 0, 0};

    /// 锚点局部比例（u/v ∈ {0,0.5,1}，画布域锚点 = (x+u·w, y+v·h)）。
    /// \param a 锚点索引 [0,4]（越界取 0=中心——数据损坏安全兜底）
    /// \return 通过 out 返回 (u, v)
    static void anchorUV(int a, double& u, double& v) noexcept {
        switch (a) {
            case 1: u = 0.0; v = 0.0; break; // 左上
            case 2: u = 1.0; v = 0.0; break; // 右上
            case 3: u = 1.0; v = 1.0; break; // 右下
            case 4: u = 0.0; v = 1.0; break; // 左下
            default: u = 0.5; v = 0.5; break; // 中心（含越界兜底）
        }
    }

    /// 任一角点位移非 0（渲染端快速路径判定；NaN 视作非 0 走保守路径）
    bool cornerActive() const noexcept {
        for (int i = 0; i < 8; ++i)
            if (corner[i] != 0.0) return true;
        return false;
    }

    /// 任一姿态分量（俯仰/偏航）非 0（v3 渲染分流判定；同 cornerActive 纪律）
    bool poseActive() const noexcept {
        return rot_x != 0.0 || rot_y != 0.0;
    }

    /// 任一裁切边非 0（v3；NaN 视作非 0 走保守路径）
    bool cropActive() const noexcept {
        for (int i = 0; i < 4; ++i)
            if (crop[i] != 0.0) return true;
        return false;
    }

    /// 裁切后的可见矩形（v3）：本 Transform 的 (x,y) 为名义左上角、
    /// (w,h) 为名义尺寸——各边向内收（负值视作 0），可见宽高钳 ≥1px
    ///（过裁不翻转为负，宁显示一丝勿退化）。
    /// 下标用字面量（0=L 1=R 2=T 3=B）——CropIdx 枚举声明在本结构之后，
    /// struct 体内不可前向引用（与 CornerIdx 同层纪律）
    void visibleRect(double& vx, double& vy, double& vw, double& vh) const
        noexcept {
        //（std::max 加括号防 windows.h min/max 宏污染——消费方 include 链
        //  含 Win32/WinRT 头时 max 被宏展开成 std::(max) 语法错误）
        const double cl =
            std::clamp(crop[0], 0.0, (std::max)(0.0, w - 1.0));
        const double cr =
            std::clamp(crop[1], 0.0, (std::max)(0.0, w - 1.0));
        const double ct =
            std::clamp(crop[2], 0.0, (std::max)(0.0, h - 1.0));
        const double cb =
            std::clamp(crop[3], 0.0, (std::max)(0.0, h - 1.0));
        vx = x + cl;
        vy = y + ct;
        vw = (std::max)(w - cl - cr, 1.0);
        vh = (std::max)(h - ct - cb, 1.0);
    }
};

/// 角点索引（Transform::corner 下标基；×2=dx 基，+1=dy）
enum CornerIdx {
    kCornerLT = 0, ///< 左上
    kCornerRT = 1, ///< 右上
    kCornerRB = 2, ///< 右下
    kCornerLB = 3, ///< 左下
};

/// 裁切边索引（Transform::crop 下标；v3）
enum CropIdx {
    kCropL = 0, ///< 左
    kCropR = 1, ///< 右
    kCropT = 2, ///< 上
    kCropB = 3, ///< 下
};

inline bool operator==(const Transform& a, const Transform& b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h &&
           a.rotation == b.rotation && a.opacity == b.opacity && a.z == b.z &&
           a.anchor == b.anchor &&
           std::equal(std::begin(a.corner), std::end(a.corner),
                      std::begin(b.corner)) &&
           a.rot_x == b.rot_x && a.rot_y == b.rot_y &&
           std::equal(std::begin(a.crop), std::end(a.crop),
                      std::begin(b.crop));
}

// ---------------------------------------------------------------------------
// Tween 关键帧动画（2026-11-05 建立对标 WATCHOUT 通道体系；
// 2026-11-09 v2 重构为 DaVinci Resolve Inspector 打包属性模式）
// 曲线定义住在 Clip 上（类型无关），随场景文档分发；运行时只流动播放头 t，
// 求值在 TweenEngine::resolveTransform（纯函数）逐帧插值完成。
// ---------------------------------------------------------------------------
// v2 打包属性（用户定案）：二维值打包成一条属性，每条属性单独打键——
// 一个关键帧携带 v0/v1 全部分量；时间轴片段带菱形不分属性、样式统一。
// v3（2026-11-09，DaVinci 五分组定案）：Position 升三维打包（v0/v1/v2 =
// x/y/z，Z=5 通道退役合并）；新增旋转 X/Y 与裁切四边枚举。
// v4（2026-11-09，用户定案"姿态合并一行按轴命名"）：Rotation=3 升三维
// 打包（v0/v1/v2 = 俯仰绕X/偏航绕Y/滚转绕Z，UI 分量名 X/Y/Z），
// RotationX=10/RotationY=11 退役合并（读端迁移，同 Z=5 先例）。
enum class TweenProp : int {
    Unknown  = 0, ///< 未知/未设置（proto3 枚举解析容错）
    Position = 1, ///< 三维打包（v3）：v0=画布坐标 X，v1=画布坐标 Y，v2=Z
    Size     = 2, ///< 二维打包：v0=绘制宽度 W，v1=绘制高度 H
    Rotation = 3, ///< 三维打包（v4）：v0=俯仰 rot_x（绕画布 X 轴，正=顶边
                  ///< 后仰）、v1=偏航 rot_y（绕画布 Y 轴，正=右缘远离）、
                  ///< v2=滚转 rotation（绕画布 Z 轴，2D 旋转，顺时针）
    Opacity  = 4, ///< 标量：不透明度 [0,1]
    Z        = 5, ///< 【v3 退役】v2 独立 Z 通道——读端迁移进 Position 的
                   ///< v2 分量；枚举值保留不重用（旧文件判据安全）
    // ---- 角点位移（每角一条二维打包属性；作用 = Transform::corner 的
    // CornerIdx 对应角的 (dx, dy)，画布 px 末端修正）-----------------------
    CornerLT = 6, ///< 左上角位移 (v0=dx, v1=dy)
    CornerRT = 7, ///< 右上角位移 (v0=dx, v1=dy)
    CornerRB = 8, ///< 右下角位移 (v0=dx, v1=dy)
    CornerLB = 9, ///< 左下角位移 (v0=dx, v1=dy)
    // ---- v3 扩展（2026-11-09，DaVinci 五分组）--------------------------
    RotationX = 10, ///< 【v4 退役】v3 独立俯仰通道——读端迁移进 Rotation
                    ///< 的 v0 分量；枚举值保留不重用（旧文件判据安全）
    RotationY = 11, ///< 【v4 退役】v3 独立偏航通道——读端迁移进 Rotation
                    ///< 的 v1 分量；枚举值保留不重用（旧文件判据安全）
    CropL     = 12, ///< 标量：左裁切（画布 px，向内收）
    CropR     = 13, ///< 标量：右裁切
    CropT     = 14, ///< 标量：上裁切
    CropB     = 15, ///< 标量：下裁切
    AudioGain = 16, ///< 标量：音频增益（线性，1.0=不衰减，0=静音，>1 提升；
                    ///< 2026-11-10 追加——音频域属性随 tween 体系走关键帧）
};

/// 打包属性分量数（1=标量 / 2=二维向量 / 3=三维向量）——UI 行构造、
/// auto-key 打包、求值共用同一真相（勿在各处手写 switch）
inline constexpr int tweenPropComponents(TweenProp p) noexcept {
    switch (p) {
        case TweenProp::Position:
            return 3; // v3 三维打包 (x, y, z)
        case TweenProp::Rotation:
            return 3; // v4 三维打包 (俯仰绕X, 偏航绕Y, 滚转绕Z)
        case TweenProp::Size:
        case TweenProp::CornerLT:
        case TweenProp::CornerRT:
        case TweenProp::CornerRB:
        case TweenProp::CornerLB:
            return 2;
        case TweenProp::RotationX: // v4 退役（迁移期读入兼容）
        case TweenProp::RotationY: // v4 退役（迁移期读入兼容）
        case TweenProp::Opacity:
        case TweenProp::Z:
        case TweenProp::CropL:
        case TweenProp::CropR:
        case TweenProp::CropT:
        case TweenProp::CropB:
        case TweenProp::AudioGain:
        default:
            return 1;
    }
}

enum class TweenInterp : int {
    Linear = 0, ///< 线性（默认）
    Hold   = 1, ///< 阶跃：保持本关键帧值直到下一关键帧
    Smooth = 2, ///< 平滑缓动：3t²-2t³（Hermite 零端斜率）
};

/// 单个关键帧（t_us 为相对 clip.start_us 的局部时间，∈ [0, duration_us]）。
/// v2/v3 打包语义：一个键携带该属性全部分量（标量属性只用 v0；interp 整键
/// 统一，作用到全部分量；v3 Position 三维用 v0/v1/v2）。
/// ⚠️ 聚合构造必须五字段 {t, v0, v1, v2, interp}——四字段初始化器会把
/// interp 隐转 double 塞进 v2（2026-11-09 三字段同型坑的 v3 复刻）。
struct Keyframe {
    std::int64_t t_us = 0; ///< 局部时间（us）
    double v0 = 0.0;       ///< 标量值 / 向量第一分量
    double v1 = 0.0;       ///< 向量第二分量（标量属性忽略）
    double v2 = 0.0;       ///< 向量第三分量（v3 Position 的 z；其余属性忽略）
    TweenInterp interp = TweenInterp::Linear; ///< 本帧→下一帧的插值方式
};

inline bool operator==(const Keyframe& a, const Keyframe& b) {
    return a.t_us == b.t_us && a.v0 == b.v0 && a.v1 == b.v1 &&
           a.v2 == b.v2 && a.interp == b.interp;
}

/// 一条 tween 通道 = 某属性随时间的关键帧序列（keys 按 t_us 升序）
struct TweenChannel {
    TweenProp prop = TweenProp::Unknown; ///< 动画属性
    std::vector<Keyframe> keys;          ///< 关键帧序列（升序不变量由 loader 保证）
};

inline bool operator==(const TweenChannel& a, const TweenChannel& b) {
    return a.prop == b.prop && a.keys == b.keys;
}

// ---------------------------------------------------------------------------
// 素材 —— 媒体文件元数据（文件本体经 HTTP 分发，§5.5）
// ---------------------------------------------------------------------------
struct Asset {
    std::string id;            ///< 全局唯一 ID
    AssetKind   kind = AssetKind::Unknown; ///< 素材种类
    std::string path;          ///< 相对素材根目录路径（'/' 分隔）
    std::uint64_t size_bytes = 0; ///< 文件大小（字节）
    std::string checksum;      ///< 校验和 "算法:hex"
    std::int64_t duration_us = 0; ///< 视频/音频时长（us）；图片/文本为 0
    std::int32_t width  = 0;   ///< 像素宽
    std::int32_t height = 0;   ///< 像素高
    double fps = 0.0;          ///< 帧率（视频）；图片/文本为 0

    // 【2026-11-05 文本片段完整化】TextPayload 内存镜像（文本随场景 pb
    // 分发，path 恒空——AssetDeps 已排除文本）。2026-11-05 新增的
    // 粗斜体/对齐/背景色为 proto3 普通标量"缺席 = 0 值 = 业务默认"
    // （非粗斜/左对齐/无背景），无 opacity 类"缺席覆盖非零默认"陷阱，
    // 无需 optional。（v_align 未实装：帧为自然尺寸，垂直方向无对齐
    // 自由度；proto 字号 8 号位预留，未来"固定框排版"模式可启用）
    std::string text_content;      ///< UTF-8 文本内容（显式 '\n' 换行）
    std::string text_font;         ///< 字体显示名（映射表解析；空 = 默认雅黑）
    std::int32_t text_font_size = 48; ///< 字号（px，>0；0 视作未设置）
    std::uint32_t text_color = 0xFFFFFF; ///< 文字颜色 0xRRGGBB
    bool text_bold   = false;  ///< 粗体（映射表粗体文件；缺文件仿粗叠加）
    bool text_italic = false;  ///< 斜体（仿斜 ≈12°）
    std::int32_t text_h_align = 0; ///< 水平对齐：0=左 1=中 2=右（多行行间）
    std::uint32_t text_bg_color = 0; ///< 背景色 0xRRGGBB；0 = 透明背景

    // 【2026-11-08 Live Hub】实时载荷内存镜像（kind == Live 时有效；path
    // 恒空随场景 pb 分发，同文本模式——AssetDeps 已排除拉文件）。字段全部
    // 为"缺席 = 0 = 业务默认"普通标量（proto3 无 has_ 陷阱）。
    std::string  live_source_name; ///< NDI 源名（发现列表选择）
    std::string  live_url;         ///< 网络流 URL（ingest=network 时有效）
    std::int32_t live_ingest  = 0; ///< 采集器：0=ndi 1=window 2=spout 3=network 4=monitor 5=camera
    std::int32_t live_latency = 0; ///< 0=最低（帧到即显，默认）1=同步（缓冲 2~3 帧）
};

/// 实时源采集器类型（与 proto LivePayload.ingest 一一对应；
/// 条目清单见 live/LiveIngest.hpp 注册表——新增协议 = 枚举 + 注册表一行）
enum class LiveIngest : std::int32_t {
    Ndi     = 0, ///< 网络 NDI 源（NdiIngest：发现列表选择）
    Window  = 1, ///< 本机窗口捕获（Windows.Graphics.Capture / WgcIngest）
    Spout   = 2, ///< 同机 Spout sender（GPU 纹理共享 / SpoutIngest）
    Network = 3, ///< 网络流 rtsp/srt/udp（FFmpeg，待实现）
    Monitor = 4, ///< 本机显示器捕获（Windows.Graphics.Capture / WgcIngest）
    Camera  = 5, ///< 本机摄像头/采集卡设备（Media Foundation Source Reader / CameraIngest）
};

inline bool operator==(const Asset& a, const Asset& b) {
    return a.id == b.id && a.kind == b.kind && a.path == b.path &&
           a.size_bytes == b.size_bytes && a.checksum == b.checksum &&
           a.duration_us == b.duration_us && a.width == b.width &&
           a.height == b.height && a.fps == b.fps &&
           a.text_content == b.text_content && a.text_font == b.text_font &&
           a.text_font_size == b.text_font_size && a.text_color == b.text_color &&
           a.text_bold == b.text_bold && a.text_italic == b.text_italic &&
           a.text_h_align == b.text_h_align && a.text_bg_color == b.text_bg_color &&
           a.live_source_name == b.live_source_name && a.live_url == b.live_url &&
           a.live_ingest == b.live_ingest && a.live_latency == b.live_latency;
}

// ---------------------------------------------------------------------------
// 片段 —— 时间轴最小编排单元【前闭后开】
// ---------------------------------------------------------------------------
struct Clip {
    std::string id;        ///< 全局唯一 ID
    std::string asset_id;  ///< 引用素材表（加载时校验存在性 + 类型匹配）
    std::int64_t start_us    = 0; ///< 时间轴起点（us，>= 0）
    std::int64_t duration_us = 0; ///< 播放时长（us，> 0）
    std::int64_t in_us       = 0; ///< 素材内部入点（us，>= 0）
    Transform transform;   ///< 画布变换属性（静态基准值；有 tween 通道的属性
                           ///< 每帧被通道值覆写，无通道的属性恒用基准值）
    std::vector<TweenChannel> tweens; ///< Tween 关键帧通道（声明式变换动画）
    // 音频增益静态基准（2026-11-10）：线性增益，1.0=不衰减。有 AudioGain
    // tween 通道时被通道整条接管（同 transform 属性语义）；proto 缺席回填
    // 1.0（optional+has_，防"缺席=0=静音"打哑旧工程）
    double audio_gain = 1.0;
    // Control 片段触发指令表（2026-11-13）：JSON 文本，格式与
    // OSCMessageListWidget 的 save/load 兼容（{"messages":[...]}——
    // 编辑区控件直接 load/save 此串）；仅 kind=Control 轨道片段使用，
    // 其余类型恒空（proto 缺席 = 空串，零回填需求）
    std::string control_messages;
};

inline bool operator==(const Clip& a, const Clip& b) {
    return a.id == b.id && a.asset_id == b.asset_id &&
           a.start_us == b.start_us && a.duration_us == b.duration_us &&
           a.in_us == b.in_us && a.transform == b.transform &&
           a.tweens == b.tweens && a.audio_gain == b.audio_gain &&
           a.control_messages == b.control_messages;
}

// ---------------------------------------------------------------------------
// 轨道 —— 同类片段容器，带类型约束（§4.2：轨道→类型约束）
// ---------------------------------------------------------------------------
struct Track {
    std::string id;    ///< 全局唯一 ID
    std::string name;  ///< 显示名
    AssetKind kind = AssetKind::Unknown; ///< 类型约束：本轨 clip 的素材须为此类型
    std::vector<Clip> clips; ///< 片段列表（允许同轨交叠，绘制次序由 z 决定）
};

inline bool operator==(const Track& a, const Track& b) {
    return a.id == b.id && a.name == b.name && a.kind == b.kind &&
           a.clips == b.clips;
}

// ---------------------------------------------------------------------------
// 时间轴 —— evaluate(t) 的编排数据
// ---------------------------------------------------------------------------
struct Timeline {
    std::int64_t duration_us = 0; ///< 总时长（us；0 = 按片段自动推算）
    std::vector<Track> tracks;
};

inline bool operator==(const Timeline& a, const Timeline& b) {
    return a.duration_us == b.duration_us && a.tracks == b.tracks;
}

// ---------------------------------------------------------------------------
// 屏幕输出变形（warp）—— WATCHOUT 式网格变形（2026-11-09）
//
// 定位：display 端输出像素级的二次映射。合成管线先把本机画面 1:1 画进
// 离屏画布（FBO），再按本结构的网格把画布内容"贴"到物理输出窗上——
// 网格节点定义输出窗上的目标位置，内容 UV 恒为规则网格（只移动位置、
// 不重采样内容语义）。
//
// 预留：mode=2 为 MPCDI（VESA 标准，pfm/缓冲区参数化）占位——本轮不实装，
// 枚举先立位防协议字段再挪；融合（blending / alpha mask）同属输出变形
// 家族，接入点在渲染端 warp pass 之后追加混合权重图，模型暂不涉及。
// ---------------------------------------------------------------------------
struct ScreenWarp {
    int mode = 0;   ///< 0=无变形 1=手动网格（拖拽编辑）2=MPCDI（预留）
    int cols = 4;   ///< 网格列数（节点数，含边界；合法域 [2,16]）
    int rows = 4;   ///< 网格行数（同上）
    /// 节点目标位置，输出归一化 [0,1]、y 向下、行主序（先列后行），
    /// size = cols*rows*2（每节点 x,y 两个分量）。
    /// 恒等网格 = 节点均匀铺满 [0,1]²；拖拽编辑改写节点位置实现变形。
    std::vector<double> pts;

    /// 是否生效（mode 非 0 且参数完备；渲染端以此决定是否走变形管线）
    bool active() const { return mode != 0; }

    /// 恒等网格（cols×rows 均匀铺满 [0,1]²，y 向下行主序）
    static ScreenWarp identity(int cols_, int rows_) {
        ScreenWarp w;
        w.mode = 1;
        w.cols = cols_ < 2 ? 2 : cols_;
        w.rows = rows_ < 2 ? 2 : rows_;
        w.pts.resize(static_cast<std::size_t>(w.cols) * w.rows * 2);
        for (int r = 0; r < w.rows; ++r) {
            for (int c = 0; c < w.cols; ++c) {
                const std::size_t i =
                    (static_cast<std::size_t>(r) * w.cols + c) * 2;
                w.pts[i]     = w.cols == 1 ? 0.0
                                           : static_cast<double>(c) / (w.cols - 1);
                w.pts[i + 1] = w.rows == 1 ? 0.0
                                           : static_cast<double>(r) / (w.rows - 1);
            }
        }
        return w;
    }

    /// 规范化：cols/rows 钳到合法域、pts 长度修正（不足补恒等、超出截断）、
    /// 坐标钳制 [0,1]。编辑器/加载侧共用的"信任边界收口"——非法数据
    /// （长度不匹配等）由此收敛到可渲染状态，调用方无需逐项防御。
    void normalize() {
        if (cols < 2) cols = 2;
        if (cols > 16) cols = 16;
        if (rows < 2) rows = 2;
        if (rows > 16) rows = 16;
        const std::size_t need =
            static_cast<std::size_t>(cols) * rows * 2;
        if (pts.size() < need) {
            // 尺寸不足：截断到已有节点数对应的完整行，再补恒等
            const std::size_t have_pairs = pts.size() / 2;
            pts.resize(need);
            for (std::size_t p = have_pairs; p < need / 2; ++p) {
                const std::size_t c = p % static_cast<std::size_t>(cols);
                const std::size_t r = p / static_cast<std::size_t>(cols);
                pts[p * 2]     = cols == 1 ? 0.0
                                           : static_cast<double>(c) / (cols - 1);
                pts[p * 2 + 1] = rows == 1 ? 0.0
                                           : static_cast<double>(r) / (rows - 1);
            }
        } else if (pts.size() > need) {
            pts.resize(need);
        }
        for (auto& v : pts) {
            if (!(v > 0.0)) v = 0.0; // NaN/负 → 0（NaN 比较恒 false）
            if (v > 1.0) v = 1.0;
        }
    }
};

inline bool operator==(const ScreenWarp& a, const ScreenWarp& b) {
    return a.mode == b.mode && a.cols == b.cols && a.rows == b.rows &&
           a.pts == b.pts;
}

// ---------------------------------------------------------------------------
// 屏幕 —— 虚拟画布中的一个物理输出区域，绑定到某个 Player（§2.1）
//
// 旋转语义：screen 是画布上的一块【可旋转取景窗】（物理 LED 异形拼接/
// 竖屏安装时，屏在画布上以任意角度摆放）。渲染时取该旋转矩形围住的画布
// 内容输出到本机，内容方向【跟随画布】而非屏体——输出像素与画布点的
// 对应由 ScreenTransform 的仿射（含逆旋转）完成。
// ---------------------------------------------------------------------------
struct Screen {
    std::string id;        ///< 全局唯一 ID
    std::string player_id; ///< 绑定的 Player 标识（注册时上报）
    double x = 0.0, y = 0.0; ///< 画布中的矩形位置（未旋转时左上角）
    double w = 0.0, h = 0.0; ///< 画布中的矩形尺寸（> 0）
    double rotation = 0.0;   ///< 旋转角度（度，顺时针，绕矩形中心）
    int32_t z = 0;           ///< 叠放层级：多屏重叠时小值先绘（底层），
                             ///< 大值后绘（顶层）；单屏独占时无视觉差异
    ScreenWarp warp;         ///< 输出变形网格（WATCHOUT 式；mode=0 无变形）
};

inline bool operator==(const Screen& a, const Screen& b) {
    return a.id == b.id && a.player_id == b.player_id && a.x == b.x &&
           a.y == b.y && a.w == b.w && a.h == b.h &&
           a.rotation == b.rotation && a.z == b.z && a.warp == b.warp;
}

// ---------------------------------------------------------------------------
// 场景 —— 顶层容器（Editor 编辑/分发的完整单元）
// ---------------------------------------------------------------------------
struct Scene {
    std::string id;      ///< 全局唯一 ID
    std::string version; ///< 场景版本号（增量 PATCH 比对用，见实现计划 P3）
    double canvas_w = 0.0; ///< 虚拟画布宽（像素，> 0）
    double canvas_h = 0.0; ///< 虚拟画布高（像素，> 0）
    std::vector<Screen> screens;  ///< 输出屏幕布局
    Timeline timeline;            ///< 时间轴编排
    std::vector<Asset> assets;    ///< 素材表

    /// 按素材 ID 查找素材（线性查找；素材量级小，可接受。
    /// 若未来素材量大，可在 SceneLoader 加载时建 unordered_map 索引）
    const Asset* findAsset(const std::string& asset_id) const noexcept {
        for (const auto& a : assets)
            if (a.id == asset_id) return &a;
        return nullptr;
    }
};

inline bool operator==(const Scene& a, const Scene& b) {
    return a.id == b.id && a.version == b.version && a.canvas_w == b.canvas_w &&
           a.canvas_h == b.canvas_h && a.screens == b.screens &&
           a.timeline == b.timeline && a.assets == b.assets;
}

// （第七迭代：ScenePatch 增量补丁内存结构已随增量机制退役——场景同步
//  一律广播全量 Scene，无需差异补丁的 set/removed/track_ops 描述。）

} // namespace ms
