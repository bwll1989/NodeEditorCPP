// ============================================================================
//  ClipInspector —— 通用属性行渲染器（面板三件套之视图端，P4-E）
//
//  定位（Doc.md §3 终态）：输入 = FieldDef 列表（ClipSchema 声明）+
//  ClipStore 指针（存储端）——渲染端零类型知识：按 schema 行摆控件、
//  灌播放头处终值、变更写回。片段作者今后只写 FieldDef 表（声明式），
//  真正领域专属的控件仍可走自定义 field editor（ClipPlugInterface
//  createPropertySection 工厂不变——本渲染器是【通用行】的收编底座）。
//
//  交互纪律（与 TweenSection 同源）：
//    * 即时生效：分量值框变更 → ClipStore::set（双模直写：通道存在
//      auto-key 打包打键；无通道静态基准；落定经 on_edited 宿主编排）；
//    * 灌值 refresh()：播放头处 WATCHOUT 终值逐分量灌显示；QSignalBlocker
//      防回环 + 焦点保护（正在编辑的分量不被灌值打断）；
//    * 控件分派（第一版）：type=Double/Int → FloatDragValueWidget
//      （Int 走 decimals=0 渲染）；Enum/Bool/String/Color 待消费者落地时
//      分派专用控件（schema 词汇已备——FieldDef.type）。
//
//  首个消费者：片段专属区 FieldDef 化迁移（Text/Live/Control/Artnet/
//  Mapping 逐插件迁移，长期共存不强制——绞杀者模式）；TweenSection
//  换底为 Inspector 后续切片（其菱形/重置/五分组 UI 语义先行保留）。
// ============================================================================

#pragma once

#include <vector>

#include <QWidget>

#include "ClipSchema.h" // ms::FieldDef（值语义拷贝，schema 唯一事实源）

#include "ClipPanelBaseExport.h"

class QGridLayout;

namespace ms {

class ClipStore; // 三件套之存储端（同库 ClipStore.hpp）

class CLIPPANELBASE_EXPORT ClipInspector : public QWidget {
    Q_OBJECT
public:
    /// \param fields schema 行列表（值语义拷贝；行序即渲染序——
    ///               分组/分页由调用方切分传入，本渲染器平铺）
    /// \param parent Qt 父控件
    explicit ClipInspector(std::vector<FieldDef> fields,
                           QWidget* parent = nullptr);

    /// 注入存储端（构造后立即设置；未设置时控件只改显示不写回）。
    /// 不拥有（宿主保证存活期覆盖本控件）。
    void setStore(ClipStore* store) { store_ = store; }

    /// 【模型→面板向】播放头处终值灌显示（QSignalBlocker + 焦点保护——
    /// 正在编辑的分量框跳过）。播放头移动/外部变化时由消费者调用。
    void refresh();

    /// 行分量控件访问器（测试定位锚/程序化操作）：row ∈ [0, rowCount)、
    /// comp ∈ [0, components)；越界返回 nullptr
    QWidget* valueWidget(int row, int comp) const;

private:
    void onEdited(int row, int comp, double v); ///< 分量编辑 → store 双模写

    /// schema 行列表（构造后只读——rows_ 内指针安全）
    std::vector<FieldDef> fields_;

    struct Row {
        const FieldDef*       fd = nullptr; ///< schema 行（指向 fields_）
        std::vector<QWidget*> value;        ///< 分量控件（components 个）
    };
    std::vector<Row> rows_;
    ClipStore* store_ = nullptr; ///< 存储端（不拥有；nullptr=只显示）
};

} // namespace ms
