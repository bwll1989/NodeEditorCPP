#pragma once

#include "BaseClassExport.h"
#include <QGroupBox>
#include <QColor>
#include <QList>

class AbstractClipModel;
class QButtonGroup;
class QLineEdit;
class QLabel;

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

/**
 * @brief 片段外观面板（颜色/备注名）
 * @details
 * 样式约定：
 * - 色块的动态背景色来自片段颜色数据（titleColorPresets / owner->color()），属于数据而非样式；
 * - 边框等装饰样式通过 Q_PROPERTY 暴露，可被 QSS 覆盖：
 *     ClipAppearancePanel {
 *         qproperty-swatchBorderColor: #666666;
 *         qproperty-swatchCheckedBorderColor: #ffffff;
 *         qproperty-currentColorBorderColor: #888888;
 *     }
 * - 主题切换（QEvent::StyleChange）时自动重建色块样式。
 */
class BASECLASS_EXPORT ClipAppearancePanel : public QGroupBox {
    Q_OBJECT
    // 颜色预设按钮边框（未选中）
    Q_PROPERTY(QColor swatchBorderColor READ swatchBorderColor WRITE setSwatchBorderColor)
    // 颜色预设按钮边框（选中态）
    Q_PROPERTY(QColor swatchCheckedBorderColor READ swatchCheckedBorderColor WRITE setSwatchCheckedBorderColor)
    // 当前颜色指示块边框
    Q_PROPERTY(QColor currentColorBorderColor READ currentColorBorderColor WRITE setCurrentColorBorderColor)
public:
    explicit ClipAppearancePanel(AbstractClipModel* owner, QWidget* parent = nullptr);

    QColor swatchBorderColor() const { return m_swatchBorderColor; }
    void setSwatchBorderColor(const QColor& color) { m_swatchBorderColor = color; }

    QColor swatchCheckedBorderColor() const { return m_swatchCheckedBorderColor; }
    void setSwatchCheckedBorderColor(const QColor& color) { m_swatchCheckedBorderColor = color; }

    QColor currentColorBorderColor() const { return m_currentColorBorderColor; }
    void setCurrentColorBorderColor(const QColor& color) { m_currentColorBorderColor = color; }

protected:
    /**
     * 样式变化事件：QSS 重新生效后重建色块样式，保证 qproperty- 新值即时生效
     */
    void changeEvent(QEvent* event) override;

private:
    void syncFromOwner();

    AbstractClipModel* m_owner {nullptr};
    QButtonGroup* m_colors {nullptr};
    QLineEdit* m_noteEdit {nullptr};
    QLabel* m_currentColor {nullptr};
    bool m_syncing {false};

    // 边框样式默认值，可被 QSS qproperty- 覆盖
    QColor m_swatchBorderColor {QColor(102, 102, 102)};        // #666666
    QColor m_swatchCheckedBorderColor {Qt::white};             // 选中描边
    QColor m_currentColorBorderColor {QColor(136, 136, 136)};  // #888888
};
