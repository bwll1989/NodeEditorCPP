// ============================================================================
//  ClipInspector 实现 —— 声明式属性行渲染（FieldDef → 控件装配）
//
//  装配规则（第一版，通用数值行）：
//    * 行 = [标签][分量值框...]（QGridLayout 单列行表；分组/折叠归消费
//      方——fields_ 按页/组切分后分次传入即可，本渲染器不持有分组状态）；
//    * type=Double → components 个 FloatDragValueWidget（setRange/step/
//      decimals/suffix 全自 schema 行）；type=Int 同控件 decimals=0；
//      Enum/Bool/String/Color 第一版不渲染（qWarning 留痕——消费者需要
//      时再分派专用控件）；
//    * objectName = "insp" + <key> + <分量名>（测试定位锚，对齐
//      TweenSection tween<suffix><分量> 命名式样；标量行无分量段）；
//    * 值域/步进/小数位/后缀缺失（schema 未填）走控件默认；
//    * 写回经 ClipStore::set 双模直写（通道打包 auto-key / 静态基准），
//      编辑落定（on_edited）在 store 内收口——渲染端零编排知识。
// ============================================================================

#include "ClipInspector.hpp"

#include <algorithm> // std::clamp（分量数钳制）

#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include "ClipStore.hpp" // ms::ClipStore 完整定义（set/get 调用）

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace ms {

ClipInspector::ClipInspector(std::vector<FieldDef> fields, QWidget* parent)
    : QWidget(parent), fields_(std::move(fields)) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(4);

    auto* box  = new QWidget(this);
    auto* grid = new QGridLayout(box);
    grid->setContentsMargins(6, 4, 6, 6);
    grid->setHorizontalSpacing(4);
    grid->setVerticalSpacing(3);
    // 行名列统一最小宽（四字行名——与 TweenSection makeRowGrid 同法，防
    // 行名宽度不同导致值区起点漂移）
    grid->setColumnMinimumWidth(
        0, QFontMetrics(box->font())
               .horizontalAdvance(QStringLiteral("音频增益")) +
               8);
    grid->setColumnStretch(0, 0);

    int row = 0;
    for (std::size_t i = 0; i < fields_.size(); ++i) {
        const FieldDef& fd = fields_[i];
        // schema 可见性闸（false = 整行不渲染——与 P4-A "不生成"语义一致）
        if (!fd.visible) continue;
        // 控件分派（第一版：数值域行；其余类型待消费者需要时扩展）
        if (fd.type != FieldType::Double && fd.type != FieldType::Int) {
            qWarning() << "inspector: schema 行类型暂无控件分派（待消费"
                          "者需要时扩展）" << fd.key << (int)fd.type;
            continue;
        }

        Row r;
        r.fd = &fields_[i];

        auto* name_lbl = new QLabel(QString::fromUtf8(fd.label), box);
        name_lbl->setToolTip(
            QStringLiteral("%1（key=%2）").arg(QString::fromUtf8(fd.label),
                                               QString::fromUtf8(fd.key)));
        grid->addWidget(name_lbl, row, 0);

        const int comps = std::clamp(fd.components, 1, 3);
        for (int c = 0; c < comps; ++c) {
            auto* sp = new FloatDragValueWidget(box);
            sp->setObjectName(QStringLiteral("insp%1%2")
                                  .arg(QString::fromUtf8(fd.key),
                                       QString::fromUtf8(fd.comp_names[c])));
            sp->setRange(fd.vmin, fd.vmax);
            sp->setDecimals(fd.type == FieldType::Int ? 0 : fd.decimals);
            sp->setSingleStep(fd.step);
            if (fd.unit && fd.unit[0])
                sp->setSuffix(QString::fromUtf8(fd.unit));
            // 【下标口径统一 2026-11-20】回调传 rows_ 下标（渲染行序），
            // 经 Row.fd 反查 key——此前混用 fields_ 下标：fields_ 前部存在
            // visible=false / 非数值行时 rows_ 与 fields_ 错位，编辑误拒/
            // refresh 灌错行。rows_ 下标 = 控件数组序，唯一稳定口径。
            connect(sp, &FloatDragValueWidget::valueChanged, this,
                    [this, rid = static_cast<int>(rows_.size()), c](double v) {
                        onEdited(rid, c, v);
                    });
            grid->addWidget(sp, row, 1 + c);
            r.value.push_back(sp);
        }
        for (int col = 0; col < comps; ++col)
            grid->setColumnStretch(1 + col, 1);
        rows_.push_back(std::move(r));
        ++row;
    }
    root->addWidget(box);
    root->addStretch(1); // 余量吸收（行少时上对齐）
}

void ClipInspector::onEdited(int row, int comp, double v) {
    if (!store_ || row < 0 || row >= static_cast<int>(rows_.size())) return;
    // 双模直写（通道打包 auto-key / 静态基准）+ on_edited 落定全在 store。
    // row = rows_ 渲染行下标；key 经 Row.fd 反查（Row.fd 指向 fields_ 原
    // 条目——fields_ 是值语义 vector 且构造后不再变更，指针稳定）。
    store_->set(QString::fromUtf8(rows_[static_cast<std::size_t>(row)].fd->key),
                comp, v);
}

void ClipInspector::refresh() {
    if (!store_) return;
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        Row& r = rows_[i];
        for (int c = 0; c < static_cast<int>(r.value.size()); ++c) {
            auto* sp = qobject_cast<FloatDragValueWidget*>(r.value[c]);
            if (!sp || sp->hasFocus()) continue; // 焦点保护（编辑中不灌）
            double v = 0.0;
            if (!store_->get(QString::fromUtf8(r.fd->key), c, v))
                continue;
            const QSignalBlocker b(sp); // 灌显示不写回
            sp->setValue(v);
        }
    }
}

QWidget* ClipInspector::valueWidget(int row, int comp) const {
    if (row < 0 || row >= static_cast<int>(rows_.size())) return nullptr;
    const Row& r = rows_[row];
    if (comp < 0 || comp >= static_cast<int>(r.value.size())) return nullptr;
    return r.value[comp];
}

} // namespace ms
