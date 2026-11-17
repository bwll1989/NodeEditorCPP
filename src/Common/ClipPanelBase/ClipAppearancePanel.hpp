// ============================================================================
//  ClipAppearancePanel —— 片段外观属性通用面板（「通用」页的颜色/备注区块）
//
//  出处：收编自合并前 QtTimeLine 体系（原 AbstractClipModel::initPropertyWidget
//  内嵌 appearanceGroupBox 独立成类；2026-11-15 注释/命名现代化对齐新风格
//  ——成员 m_xxx → 尾下划线，注释统一 ==== 框）。
//
//  【P3 面板迁库 2026-11-17】自 BaseClass 迁入 ClipPanelBase（通用片段 UI
//  框架库）——BaseClass 回归零 UI 纯模型/接口库；导出宏换
//  CLIPPANELBASE_EXPORT。
//  （src/Nodes/TimeLineNode/TimelineEngine.h 的 titleColorPresets 内嵌副本
//   为节点引擎冻结胶囊，独立演化不同步。）
// ============================================================================
//  样式约定（QSS qproperty-* 可配置项）见 .cpp 同名类注释。

#pragma once

#include "ClipPanelBaseExport.h"
#include <QGroupBox>
#include <QColor>
#include <QList>

class AbstractClipModel;
class QButtonGroup;
class QLineEdit;
class QLabel;

/// 标题颜色预设表（8 色；画布/时间轴/面板三处配色视觉分组同源）
inline QList<QColor> titleColorPresets()
{
    return {
        QColor(255, 140, 0),   // orange
        QColor(0, 170, 255),   // bright blue
        QColor(255, 105, 180), // pink
        QColor(160, 120, 220), // purple
        QColor(0, 180, 170),   // teal
        QColor(220, 50, 60),   // red
        QColor(240, 190, 40),  // yellow
        QColor(100, 180, 70),  // green
    };
}

/// 片段外观属性通用面板（颜色/备注名；owner 指针直连模型信号）
///  样式约定：
///  - 色块的动态背景色来自片段颜色数据（titleColorPresets / owner->color()），
///    属于数据而非样式；
///  - 边框等装饰样式通过 Q_PROPERTY 暴露，可被 QSS 覆盖：
///      ClipAppearancePanel {
///          qproperty-swatchBorderColor: #666666;
///          qproperty-swatchCheckedBorderColor: #ffffff;
///          qproperty-currentColorBorderColor: #888888;
///      }
///  - 主题切换（QEvent::StyleChange）时自动重建色块样式。
class CLIPPANELBASE_EXPORT ClipAppearancePanel : public QGroupBox {
    Q_OBJECT
    // 颜色预设按钮边框（未选中）
    Q_PROPERTY(QColor swatchBorderColor READ swatchBorderColor WRITE setSwatchBorderColor)
    // 颜色预设按钮边框（选中态）
    Q_PROPERTY(QColor swatchCheckedBorderColor READ swatchCheckedBorderColor WRITE setSwatchCheckedBorderColor)
    // 当前颜色指示块边框
    Q_PROPERTY(QColor currentColorBorderColor READ currentColorBorderColor WRITE setCurrentColorBorderColor)
public:
    explicit ClipAppearancePanel(AbstractClipModel* owner, QWidget* parent = nullptr);

    QColor swatchBorderColor() const { return swatch_border_color_; }
    void setSwatchBorderColor(const QColor& color) { swatch_border_color_ = color; }

    QColor swatchCheckedBorderColor() const { return swatch_checked_border_color_; }
    void setSwatchCheckedBorderColor(const QColor& color) { swatch_checked_border_color_ = color; }

    QColor currentColorBorderColor() const { return current_color_border_color_; }
    void setCurrentColorBorderColor(const QColor& color) { current_color_border_color_ = color; }

protected:
    /// 样式变化事件：QSS 重新生效后重建色块样式，保证 qproperty- 新值即时生效
    void changeEvent(QEvent* event) override;

private:
    void syncFromOwner();

    AbstractClipModel* owner_ {nullptr};
    QButtonGroup* colors_ {nullptr};
    QLineEdit* note_edit_ {nullptr};
    QLabel* current_color_ {nullptr};
    bool syncing_ {false};

    // 边框样式默认值，可被 QSS qproperty- 覆盖
    QColor swatch_border_color_ {QColor(102, 102, 102)};        // #666666
    QColor swatch_checked_border_color_ {Qt::white};            // 选中描边
    QColor current_color_border_color_ {QColor(136, 136, 136)}; // #888888
};
