// ============================================================================
//  ClipSchema —— 片段属性行声明式 schema（P4-B 2026-11-18，面板三件套之一）
//
//  【P4 终态架构（src/Common/ClipPanelBase/Doc.md §3 权威）】
//    ClipInspector（通用渲染器，P4-E）
//      └─ 输入 = FieldDef 列表 + ClipStore 指针 → 摆控件/灌值/写回
//    ClipSchema（本头文件）
//      └─ FieldDef{key/类型/标签/单位/范围/步长/默认值/分组/页归属}
//    ClipStore（P4-C，ClipPanelBase）
//      └─ get(path, t_us) 播放头处终值 / set(path, value) 双模直写
//    数据事实 = ms::Clip（MediaCore；本库不依赖——铁律）
//
//  【为什么放 BaseClass】片段插件只链 BaseClass+Qt（plugin_audio 不链
//  MediaCore，TweenProp 枚举不可达）——插件作者按本头声明自己的
//  FieldDef 表（声明式专属区，P4 终态："片段插件作者只写 FieldDef 表，
//  真正领域专属的控件仍可走自定义 field editor 注册"）。本头纯数据
//  零依赖（不引 TweenProp / 不引 UI 类型；QString 仅查找重载便利）。
//
//  【与 TweenSection 的关系（P4-D 泛化）】TweenSection.cpp 的
//  kPropOrder[13] PropDef 表是本表的原型（schema 原始出处）——数值/
//  标签/值域/步长/decimals/分量名/默认值等【声明性数据】以本表为唯一
//  事实；TweenProp 通道映射与 objectName 后缀（UI 测试锚）属
//  TweenSection 本地知识（MediaCore 域 + Qt 对象树命名），不进本表。
//
//  【key 词汇契约】= P4-A ClipTweenExposure::exposed_keys 白名单词汇
//  （AbstractClipInterface.h）：position/size/rotation/crop_top/
//  crop_bottom/crop_left/crop_right/corner_lt/corner_rt/corner_rb/
//  corner_lb/opacity/audio_gain。跨 DLL 寻址恒字符串（非枚举）——
//  未知 key = schema 漂移，消费端 qWarning。
//
//  【实现形态】inline constexpr 头文件自包含（无 .cpp）：纯只读数据
//  每 TU 一份副本无 ODR 风险（类型 trivially copyable、无外部链接
//  可变状态），换取零导出符号/零加载期开销。
// ============================================================================

#pragma once

#include <cstdint>

#include <QCoreApplication> // QT_TRANSLATE_NOOP（label 可提取译）
#include <QString> // findTweenField 查找重载（BaseClass 本链 Qt）

namespace ms {

// ---------------------------------------------------------------------------
// 属性值类型（词汇对齐 Doc.md §3 终态：int/double/string/enum/bool/vec3——
// vec3 用 Double+components=3 表达，不另设类型）
// ---------------------------------------------------------------------------
enum class FieldType : std::uint8_t {
    Double = 0, ///< 浮点标量/向量（components 决定维度 1..3）
    Int    = 1, ///< 整数标量（无小数步进）
    Enum   = 2, ///< 枚举下拉（候选项由控件层/宿主提供）
    Bool   = 3, ///< 开关
    String = 4, ///< 文本（备注名等）
    Color  = 5, ///< 颜色（外观色块）
};

/// 折叠组序号（v3 五分组定案，DaVinci Inspector 档；宿主侧组名表：
/// 0=变换 1=姿态 2=裁切 3=形变 4=合成。行连续切分纪律不变——组 id
/// 按行序单调不减，Inspector 按 id 切分折叠容器）
inline constexpr int kFieldGroupXform  = 0; ///< 变换（锚点+位置/尺寸）
inline constexpr int kFieldGroupPose   = 1; ///< 姿态（旋转三轴一行）
inline constexpr int kFieldGroupCrop   = 2; ///< 裁切（上下左右）
inline constexpr int kFieldGroupCorner = 3; ///< 形变（角点×4）
inline constexpr int kFieldGroupComp   = 4; ///< 合成（不透明度/音频增益）

// ---------------------------------------------------------------------------
// 声明式属性行定义——一行 = 属性面板上的一条可编辑属性。
//
// 列序（constexpr 位置初始化用，C++17 无指定初始化器）：
//   key / label / type / components / comp_names[3] /
//   vmin / vmax / step / decimals / unit / default_value[3] /
//   resettable / tweenable / group / page
// ---------------------------------------------------------------------------
struct FieldDef {
    const char* key = "";          ///< 协议寻址词汇（跨 DLL 字符串；
                                   ///<  持久化/暴露面/Store 寻址同词汇）
    const char* label = "";        ///< 显示名（中文源文，经
                                   ///  QT_TRANSLATE_NOOP("ClipSchema",...) 可提取译；
                                   ///  显示点须包 translate("ClipSchema", ...)）
    FieldType   type = FieldType::Double;
    int         components = 1;    ///< Double 向量维度 1..3（数据通道
                                   ///<  维度——UI 分量数可少于此，如
                                   ///<  位置行 UI 两框、第三值位由
                                   ///  深度 Z 静态框占用，见
                                   ///  TweenSection 2026-11-18 语义终定）
    const char* comp_names[3] = {"", "", ""}; ///< 分量名（值框标签/
                                   ///<  objectName 分量段：位置 X/Y/Z、
                                   ///  尺寸 W/H、角点 X/Y；标量全空——
                                   ///  objectName 无分量段）
    double      vmin = 0.0;        ///< 分量级值域下限
    double      vmax = 0.0;        ///< 分量级值域上限
    double      step  = 1.0;       ///< 步长（单步/拖拽加速基准）
    int         decimals = 2;      ///< 显示小数位
    const char* unit = "";         ///< 数值框后缀（" px"/"°"/""）
    double      default_value[3] = {0.0, 0.0, 0.0}; ///< ⟲ 重置默认值
                                   ///<  （各分量同值取 [0]；尺寸行语义
                                   ///  = 素材原始尺寸，schema 存 0、
                                   ///  控件层经 hint 注入特判）
    bool        resettable = true; ///< ⟲ 重置钮显隐
    bool        tweenable = true;  ///< 参与 tween 打键（false = 纯静态
                                   ///  属性——锚点/深度 Z/层级/遮挡次序
                                   ///  等追加区声明用，无菱形钮）
    int         group = 0;         ///< 折叠组序号（kFieldGroup*）
    const char* page = "tween";    ///< 页归属（三页式面板：通用/动画/
                                   ///  类型专属；P4 终态 FieldDef.page）
    bool        visible = true;    ///< 行级可见性（Doc.md §4 可见性分级：
                                   ///  false = 渲染器整行不生成——与 P4-A
                                   ///  "不生成"语义同源；P4-A 动画页显隐
                                   ///  走 ClipTweenExposure 白名单，本字段
                                   ///  供专属区/新页声明用）。尾部追加
                                   ///  （kTweenSchema 聚合初始化缺省 = true，
                                   ///  零改动）
};

// ---------------------------------------------------------------------------
// 动画页 schema —— 13 行 tween 属性表（数据自 TweenSection kPropOrder
// [13] 迁入，行序即 UI 行序即 prop_index 语义；分组按 v3 五分组）。
//
// 语义注释（自原型表逐条保留）：
//  * 位置 decimals=2（2026-11-06 方案 A 回归修正：适配器广播跟随用例
//    断言 105.07 精确显示——1 位小数会把 0.07px 取整掉）；UI 两分量
//    X/Y，第三数据槽（v2=叠放层级）独立「层级 Z」行写同槽（2026-11-18
//    语义终定，Row 契约 22 槽不动）；
//  * 尺寸 = 画布绝对 px（与位置同参照系），0 下限；⟲ 重置语义 = 恢复
//    素材原始尺寸（default_value 不生效，控件层 hint 注入）；
//  * 旋转三维打包（v4 姿态合并）：分量 X=俯仰（绕画布 X 轴）/Y=偏航
//   （绕画布 Y 轴）/Z=滚转（绕画布 Z 轴 2D 旋转）。行级值域 ±3600°
//    为通用上界；透视轴 X/Y 特判 ±89°（90° 起画面翻面透视退化）留
//    控件层 setRange（UI 知识不进 schema）；
//  * 裁切四边：向内收的画布 px（0 下限只收不放），行序按用户定案
//   「上下左右」；crop 数组字面下标 0=L 1=R 2=T 3=B（与行序无关——
//    存储映射归 ClipStore）；
//  * 角点每角一条二维属性（每角单独打键）：偏移量语义（相对未变形
//    位置的末端修正，画布 px；旋转≠0 时位移恒沿屏幕方向不随画面转）；
//    step 1.0（单像素步进——角点校正常为小位移）；
//  * 不透明度/音频增益归「合成」组：默认 1.0（表内 default_value 显式
//    1）；音频增益标量线性 [0,2]（上限 2 防增益曲线异常把混音推入
//    持续削波；1.0=不衰减/0=静音）；
//  * 颜色合成（2026-11-20 追加）：RGB 增益三维打包（分量 R/G/B 线性
//    增益 [0,2]，1=不变；step 0.05 同不透明度粒度）；Gamma 标量伽马
//    校正 [0.1,4]（1.0=不校正；渲染端 pow(c, 1/γ)，下限 0.1 防除零
//    指数发散、上限 4 防全白）。
// ---------------------------------------------------------------------------
inline constexpr FieldDef kTweenSchema[15] = {
    // 变换组（组 0）
    {"position",   QT_TRANSLATE_NOOP("ClipSchema", "位置"),     FieldType::Double, 3, {"X", "Y", "Z"},
     -100000.0, 100000.0, 10.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupXform, "tween"},
    {"size",       QT_TRANSLATE_NOOP("ClipSchema", "尺寸"),     FieldType::Double, 2, {"W", "H", ""},
     0.0, 100000.0, 10.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupXform, "tween"},
    // 姿态组（组 1）
    {"rotation",   QT_TRANSLATE_NOOP("ClipSchema", "旋转"),     FieldType::Double, 3, {"X", "Y", "Z"},
     -3600.0, 3600.0, 5.0, 2, "°", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupPose, "tween"},
    // 裁切组（组 2）
    {"crop_top",    QT_TRANSLATE_NOOP("ClipSchema", "裁切 上"), FieldType::Double, 1, {"", "", ""},
     0.0, 100000.0, 1.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupCrop, "tween"},
    {"crop_bottom", QT_TRANSLATE_NOOP("ClipSchema", "裁切 下"), FieldType::Double, 1, {"", "", ""},
     0.0, 100000.0, 1.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupCrop, "tween"},
    {"crop_left",   QT_TRANSLATE_NOOP("ClipSchema", "裁切 左"), FieldType::Double, 1, {"", "", ""},
     0.0, 100000.0, 1.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupCrop, "tween"},
    {"crop_right",  QT_TRANSLATE_NOOP("ClipSchema", "裁切 右"), FieldType::Double, 1, {"", "", ""},
     0.0, 100000.0, 1.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupCrop, "tween"},
    // 形变组（组 3）
    {"corner_lt",   QT_TRANSLATE_NOOP("ClipSchema", "角点 LT"), FieldType::Double, 2, {"X", "Y", ""},
     -100000.0, 100000.0, 1.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupCorner, "tween"},
    {"corner_rt",   QT_TRANSLATE_NOOP("ClipSchema", "角点 RT"), FieldType::Double, 2, {"X", "Y", ""},
     -100000.0, 100000.0, 1.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupCorner, "tween"},
    {"corner_rb",   QT_TRANSLATE_NOOP("ClipSchema", "角点 RB"), FieldType::Double, 2, {"X", "Y", ""},
     -100000.0, 100000.0, 1.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupCorner, "tween"},
    {"corner_lb",   QT_TRANSLATE_NOOP("ClipSchema", "角点 LB"), FieldType::Double, 2, {"X", "Y", ""},
     -100000.0, 100000.0, 1.0, 2, " px", {0.0, 0.0, 0.0}, true, true,
     kFieldGroupCorner, "tween"},
    // 合成组（组 4）
    {"opacity",     QT_TRANSLATE_NOOP("ClipSchema", "不透明度"), FieldType::Double, 1, {"", "", ""},
     0.0, 1.0, 0.05, 2, "", {1.0, 1.0, 1.0}, true, true,
     kFieldGroupComp, "tween"},
    {"audio_gain",  QT_TRANSLATE_NOOP("ClipSchema", "音频增益"), FieldType::Double, 1, {"", "", ""},
     0.0, 2.0, 0.05, 2, "", {1.0, 1.0, 1.0}, true, true,
     kFieldGroupComp, "tween"},
    {"rgb_gain",    QT_TRANSLATE_NOOP("ClipSchema", "RGB 增益"), FieldType::Double, 3, {"R", "G", "B"},
     0.0, 2.0, 0.05, 2, "", {1.0, 1.0, 1.0}, true, true,
     kFieldGroupComp, "tween"},
    {"gamma",       QT_TRANSLATE_NOOP("ClipSchema", "Gamma"),   FieldType::Double, 1, {"", "", ""},
     0.1, 4.0, 0.05, 2, "", {1.0, 1.0, 1.0}, true, true,
     kFieldGroupComp, "tween"},
};

/// 动画页 schema 行数（= TweenSection::propCount 契约值）
inline constexpr int kTweenSchemaCount =
    static_cast<int>(sizeof(kTweenSchema) / sizeof(kTweenSchema[0]));

/// 按 key 线性查找（13 行规模无需索引；未命中返回 nullptr——调用方
/// qWarning schema 漂移）
inline const FieldDef* findTweenField(const char* key) {
    for (int i = 0; i < kTweenSchemaCount; ++i)
        if (qstrcmp(kTweenSchema[i].key, key) == 0) return &kTweenSchema[i];
    return nullptr;
}

/// QString 重载（暴露面白名单 QStringList 逐 key 查找便利）
inline const FieldDef* findTweenField(const QString& key) {
    return findTweenField(key.toUtf8().constData());
}

/// 按行下标访问（TweenSection kPropOrder 表序直通；越界返回 nullptr）
inline const FieldDef* tweenFieldAt(int row) {
    return (row >= 0 && row < kTweenSchemaCount) ? &kTweenSchema[row]
                                                 : nullptr;
}

} // namespace ms
