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
#include <cmath>
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
    Artnet  = 7, ///< 灯控（2026-11-15 Artnet 片段移植，节点体系同源）——
                 ///< DMX 视频（宽 512×高=域数）逐帧解码 → ArtnetService
                 ///< 状态表（50Hz 泵广播）；配置存 Clip.clip_data（JSON）。
                 ///< 仅编辑器端消费：播放头推进钩子驱动；无素材绑定、无画布
                 ///< 呈现（与 Control 同口径）
    Mapping = 8, ///< 映射（2026-11-15 Mapping 片段移植）——消息表逐帧按
                 ///< $percent/$frame（JS 表达式）求值重发 StatusContainer；
                 ///< 消息表存 Clip.clip_data（JSON，表达式原文）。仅编辑器
                 ///< 端消费；无素材绑定、无画布呈现
    Mesh    = 9  ///< 3D 模型（2026-11-18 阶段 3A，3D mapping 地基）——
                 ///< Wavefront OBJ（v/vt/f 最小集）；不建片段、不上画布，
                 ///< 唯一消费方 = Screen.model_asset_id（Mapped 投影映射
                 ///< 屏绑定模型作投影表面）；两端共用 ms::loadMesh 解析（Assimp 后端）
                 ///<（同一文件恒得同一顶点数据）
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

    // ---- 几何深度（2026-11-17 素材 3D 化阶段①，对齐 WATCHOUT cue Z；
    //      2026-11-18 深度轴翻转定案）---------------------------------------
    // 素材在三维中的真实 z 位置（px，正 = 朝观察者 Eye/舞台向上 → 全局
    // 透视下放大，s=F/(F−Z)；负 = 远离/缩小、向消隐点收敛）。与 3D 舞台
    // 视口高度轴同向（面片升高=靠近 Eye=放大，两链语义统一）。
    // 【命名纪律】本字段是几何深度（position_z），与上方 z（叠放层级 /
    // render_order）是两种语义——严禁混用（路线文档三·0 语义链定案）。
    // 0 = 历史行为逐位不变；投影消费见 CanvasGeometry::quadCorners3DP。
    // 注意：不接入 Tween 通道（TweenProp::Position 的 v2=z 语义是叠放层
    // 级，TweenSection 13 行定长契约不因 3D 化扩表——tween 化留待后续）。
    double pos_z = 0.0;

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

    /// 任一 3D 位姿分量（俯仰/偏航/几何深度）非 0（素材 3D 化阶段①；
    /// 渲染端透视路径判定——pos_z 非 0 时即使无姿态也须走透视投影）
    bool pose3DActive() const noexcept {
        return rot_x != 0.0 || rot_y != 0.0 || pos_z != 0.0;
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
           a.rot_x == b.rot_x && a.rot_y == b.rot_y && a.pos_z == b.pos_z &&
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
    std::string asset_id;  ///< 引用素材表（加载时校验存在性；Control 片段恒空）
    // 片段插件类型（2026-11-15 轨道去类型化）：与宿主 clip 插件 clipType
    // 一致（"Video"/"Image"/"Text"/"Audio"/"Live"/"Control"…）。类型跟片段
    // 走——轨道不再约束类型后此字段是唯一持久化事实；空 = 旧文件，装载按
    // 轨道 kind 推断（向后兼容）
    std::string type;
    // 片段备注名（2026-11-15）：用户在属性面板「备注名」起的显示名——
    // 时间轴片段文字（displayText 备注名优先）的来源；
    // 空 = 未起名（显示回退 clip id/类型名）；pb note 字段往返
    std::string note;
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
    // 类型专属业务数据（2026-11-15 片段移植）：JSON 文本，形状由片段类型
    // 自描述（编辑器端消费）——Artnet 配置 {"file","targetHost",
    // "startUniverse"}；Mapping 消息表 {"messages":[...]}（value 为表达式
    // 原文，$percent/$frame 播放期求值）。其余类型恒空
    std::string clip_data;
    // per-cue 深度遮挡模式（2026-11-17 素材 3D 化阶段③；WO7 Stacking 对标
    // ——cue_properties.html："By Layer…Layer N is rendered before layer
    // N+1 / By Z means that the cue surface depth defines the render
    // order"）：0 = By Layer（叠放层级 z，历史行为逐位）；1 = By Z（几何
    // 深度 pos_z，远先绘近后绘）。TimelineEngine 排序键 = stacking 决定
    // （标量键全序，混合模式确定有序）；缺席 = 旧文件 = By Layer。
    // 0/1 之外的值装载时钳回 0（宁退回历史勿乱序）。
    int stacking = 0;
};

// Clip::stacking 取值常量（proto int32 往返；0 缺席即 By Layer）
inline constexpr int kStackingRenderOrder = 0; ///< By Layer（叠放层级 z）
inline constexpr int kStackingPositionZ   = 1; ///< By Z（几何深度 pos_z）

inline bool operator==(const Clip& a, const Clip& b) {
    return a.id == b.id && a.asset_id == b.asset_id && a.type == b.type &&
           a.note == b.note &&
           a.start_us == b.start_us && a.duration_us == b.duration_us &&
           a.in_us == b.in_us && a.transform == b.transform &&
           a.tweens == b.tweens && a.audio_gain == b.audio_gain &&
           a.control_messages == b.control_messages &&
           a.clip_data == b.clip_data && a.stacking == b.stacking;
}

// ---------------------------------------------------------------------------
// 轨道 —— 片段的时间容器（2026-11-15 去类型化：轨道不再约束片段类型，
// 片段类型由 Clip.type 声明；kind 降级为可选显示提示）
// ---------------------------------------------------------------------------
struct Track {
    std::string id;    ///< 全局唯一 ID
    std::string name;  ///< 显示名
    AssetKind kind = AssetKind::Unknown; ///< 可选类型提示（hint）：Unknown =
                                         ///< 无约束混合轨道；仅显示参考，不
                                         ///< 据此拒绝片段
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
// 【与屏姿态的复合序（2026-11-17 阶段2b 定案）】两级映射正交串联、
// 域不同不叠加：内容 →（屏姿态单应 rot_x/rot_y，画布域→输出域）→
// FBO（输出域 1:1）→（warp 网格，输出域→物理输出窗）→ 屏幕。屏姿态
// 语义是"取景窗朝向"（内容域透视，烘进 FBO），warp 语义是"物理屏
// 安装校正"（输出域网格重排）——先姿态后 warp，调 warp 时屏姿态
// 已定型；两者编辑入口独立（屏姿态=属性对话框俯仰/偏航，warp=几何
// 结构页网格），语义不重叠。
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
// 【Display 统一模型定案（2026-11-18）】——不再有 2D/3D 两种类型：
//   * 本结构 = 画布上的【取景窗】+ 可选 3D 姿态（统一的一种 Screen）。
//     素材默认铺在画布 XY 平面（可经 Transform::pos_z 离面，走全局透视
//     persp_focal/vp 缩放）；在指定范围经 Show 级透视相机取景输出——
//     "正交视图"只是焦距无穷大的兜底特例。零姿态 = 旧 2D 平面屏行为，
//     rot_x/rot_y 非 0 = 斜装/立面屏——同一结构的两档预设，不是两类；
//   * z = 仅编辑器内多屏取景窗重叠的绘制先后，不下发显示端（display
//     单机单屏，无叠放概念）；未来 Z-buffer 接管后本字段退役；
//   * rot_x/rot_y = 斜装屏（地面屏/斜幕/非正交安装）取景姿态，WATCHOUT
//     投影仪模型的轻量替身：0 = 历史仿射逐位不变；UI 钳 ±85°（90° 单应
//     奇异，89° 已近边——2026-11-18 收紧，原 ±89°）；ScreenMapper 端
//     solveHomography 失败另有恒等仿射防御（宁缺勿错）；
//   * 投影仪视锥（Eye/Target/Lens Shift）与曲面 surface 作为本结构的
//     字段扩展在统一模型上做（不另立类型——两套类型是兼容性历史包袱，
//     Flow 无历史 show 负担无需继承）；届时 Surface/View/Device 分层
//     中本结构升格为 Surface，字段全部可复用（见 2026-11-18 分析结论）。
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
    /// 屏姿态（2026-11-17 阶段2，与 Clip Transform::rot_x/rot_y 同构）：
    /// 俯仰/偏航旋转（度）——屏取景窗带 3D 姿态，画布→屏映射升级单应
    ///（ScreenTransform pose3d 分支经 cg::quadCorners3D 预投影）。0 = 无
    /// 姿态（历史仿射映射逐位不变）。
    double rot_x = 0.0;      ///< 俯仰旋转 X（度，正=顶边后仰）
    double rot_y = 0.0;      ///< 偏航旋转 Y（度，正=右缘远离）
    /// 投影模式（2026-11-18 Display 统一模型阶段1，数据先行）：
    /// 0=Framed 取景窗（默认，历史行为——引用 Show 全局 Eye 的取景窗）；
    /// 1=Mapped 投影映射（3D mapping 预留——屏持有自己的投影仪 Eye，
    /// 内容按视锥投到网格面片；渲染路径后续接入）。非 0/1 读端钳 0。
    int projectionMode = 0;

    // ---- 3D mapping 投影表面（2026-11-18 阶段 3A 数据先行）----------------
    // 仅 projectionMode=1 时消费。模型坐标 = 舞台空间（画布 px + Z 向上），
    // 经 ms::loadMesh 加载（Y-up→Z-up 轴变换 + 落地归一化：包围盒 XY 中心
    // 归原点、底面贴 z=0）后再施加 placement——摆位语义与 Watchout 3D
    // mapping 对齐：模型即投影对象，取景窗语义（x/y/w/h/rotation）在此
    // 退化为表面足迹参考框（2D 画布上仍画框，3D 视口画模型本体）。
    std::string model_asset_id; ///< 绑定的 Mesh 资产（Asset.kind==Mesh；空 =
                                ///< 未绑模型——Mapped 屏退化为单四边形表面，
                                ///< 走屏姿态单应快路径，3B 起接入）
    double model_offset_x = 0.0; ///< 模型平移 X（舞台域 px；默认 0 = 画布原点）
    double model_offset_y = 0.0; ///< 模型平移 Y（同上）
    double model_offset_z = 0.0; ///< 模型平移 Z（高度；0 = 底面贴地）
    double model_scale    = 0.0; ///< 模型等比缩放（<=0 派生 1.0——proto3 缺席
                                 ///< = 0 陷阱，读端语义"未设置 = 原尺寸"）
    // ---- 投影仪 Eye（阶段 3B 消费；此处数据先行一次申请字段号）----------
    // 独立于 Show 全局 Eye（四种相机角色定稿：投影仪 Eye 每屏一台）。
    // 单位 = 舞台域（画布 px / Z 高度 px，同全局 Eye 惯例）。全部字段
    // "缺席 = 0"，focal <= 0 = 未配置走派生默认（Eye=表面中心正上方
    // D=2×max(w,h)、Target=表面中心、F 铺满输出——新建 Mapped 屏开箱
    // 即用，再微调出真实透视）。
    double proj_eye_x = 0.0;    ///< 投影仪光心 X（舞台域）
    double proj_eye_y = 0.0;    ///< 投影仪光心 Y
    double proj_eye_z = 0.0;    ///< 投影仪光心 Z（高度；派生默认 >0）
    double proj_tgt_x = 0.0;    ///< 注视点 X（派生默认 = 表面中心）
    double proj_tgt_y = 0.0;    ///< 注视点 Y
    double proj_tgt_z = 0.0;    ///< 注视点 Z
    double proj_focal = 0.0;    ///< 焦距 px（<=0 = 派生默认铺满输出）
    double proj_shift_x = 0.0;  ///< Lens Shift X（输出域归一化 [-0.5,0.5]）
    double proj_shift_y = 0.0;  ///< Lens Shift Y（同上）
    double proj_roll    = 0.0;  ///< 投影仪滚转（度；防 up 向量退化，垂直
                                ///< 俯仰照射地面屏时必需）
};

inline bool operator==(const Screen& a, const Screen& b) {
    return a.id == b.id && a.player_id == b.player_id && a.x == b.x &&
           a.y == b.y && a.w == b.w && a.h == b.h &&
           a.rotation == b.rotation && a.z == b.z && a.warp == b.warp &&
           a.rot_x == b.rot_x && a.rot_y == b.rot_y &&
           a.projectionMode == b.projectionMode &&
           a.model_asset_id == b.model_asset_id &&
           a.model_offset_x == b.model_offset_x &&
           a.model_offset_y == b.model_offset_y &&
           a.model_offset_z == b.model_offset_z &&
           a.model_scale == b.model_scale &&
           a.proj_eye_x == b.proj_eye_x && a.proj_eye_y == b.proj_eye_y &&
           a.proj_eye_z == b.proj_eye_z && a.proj_tgt_x == b.proj_tgt_x &&
           a.proj_tgt_y == b.proj_tgt_y && a.proj_tgt_z == b.proj_tgt_z &&
           a.proj_focal == b.proj_focal && a.proj_shift_x == b.proj_shift_x &&
           a.proj_shift_y == b.proj_shift_y && a.proj_roll == b.proj_roll;
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

    // ---- Show 级全局透视参数（2026-11-17 素材 3D 化阶段①，对齐 WATCHOUT
    // 5.2 Perspective 滑杆 + Vanishing Point / 7 的 Eye point）--------------
    // 语义链（路线文档三·0）：全场唯一一次透视投影发生在"素材 3D 位姿 →
    // 成品画面"环节；本参数是该投影的全部——焦距（小=广角透视强，大=长焦
    // ≈正交）+ 消隐点（素材沿 Z 远去时向其收敛）。
    // 【解耦契约】display 取景窗（Screen x/y/w/h）不消费本参数——调透视
    // 不改取景、移取景窗不改透视（白名单 5.1 Z 平移禁止的数学依据）。
    // focal <= 0 = 未配置：渲染端经 effectivePersp() 取画布派生默认
    //（F=2×画布对角线、vp=画布中心——2026-11-18 单一透视定案：旋转扭曲
    // 与 Z 缩放共用本参数，对齐 Watchout Preferences→3D Perspective 模型；
    // 此前的两级拆分已退役）。focal > 0 = 用户配置全场统一投影。
    // 非 3D 片段两种模式逐位一致（quadCorners3DP 快路径 s≡1）。
    double persp_focal = 0.0; ///< 全局透视焦距（px；>0 启用，<=0 未配置）
    double persp_vp_x  = 0.0; ///< 消隐点 X（画布 px；focal>0 时消费）
    double persp_vp_y  = 0.0; ///< 消隐点 Y（画布 px；focal>0 时消费）

    /// 有效透视参数（2026-11-18 单一透视定案，对齐 Watchout：旋转扭曲与
    /// Z 缩放共用一组全局透视参数）：渲染端统一入口——persp_focal>0 =
    /// 用户配置直通；未配置（<=0）= 画布派生默认：F = 2×画布对角线、
    /// 消隐点 = 画布中心（全场共享同一投影仪——素材沿 Z/旋转远去时向
    /// 画布中心收敛、同深度等比，WO 语义）。
    /// 非 3D 片段（pos_z=0 且无俯仰偏航）在 quadCorners3DP 快路径 s≡1
    /// 恒等——默认值切换对历史画面零影响（逐位不变）。
    /// 画布无效（<=0）输出 focal=0：调用方保留对象级回退（数据损坏防御）。
    void effectivePersp(double& F, double& vpx, double& vpy) const noexcept {
        if (persp_focal > 0.0) {
            F = persp_focal; vpx = persp_vp_x; vpy = persp_vp_y;
            return;
        }
        if (!(canvas_w > 0.0) || !(canvas_h > 0.0)) { F = 0.0; return; }
        F   = 2.0 * std::sqrt(canvas_w * canvas_w + canvas_h * canvas_h);
        vpx = canvas_w * 0.5;
        vpy = canvas_h * 0.5;
    }

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
           a.timeline == b.timeline && a.assets == b.assets &&
           a.persp_focal == b.persp_focal && a.persp_vp_x == b.persp_vp_x &&
           a.persp_vp_y == b.persp_vp_y;
}

// （第七迭代：ScenePatch 增量补丁内存结构已随增量机制退役——场景同步
//  一律广播全量 Scene，无需差异补丁的 set/removed/track_ops 描述。）

} // namespace ms
