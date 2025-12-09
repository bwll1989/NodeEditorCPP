#pragma once

#include <QWidget>
#include <QColor>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QMouseEvent>
#include <QKeyEvent>

#include "QtColorWidgets/color_wheel.hpp"
#include "QtColorWidgets/hue_slider.hpp"
#include "QtColorWidgets/gradient_slider.hpp"
#include "QtColorWidgets/color_preview.hpp"
#include "QtColorWidgets/color_line_edit.hpp"
#include "QtColorWidgets/color_utils.hpp"
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

class GUI_ELEMENTS_EXPORT ColorEditorWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * 构造函数：创建一个可嵌入的颜色编辑器（类似 ColorDialog，但继承 QWidget）
     * @param parent 父级控件
     */
    explicit ColorEditorWidget(QWidget* parent = nullptr);

    /**
     * 析构函数：释放内部资源
     */
    ~ColorEditorWidget() override;

    /**
     * 获取当前颜色（当禁用 alpha 编辑时，返回的不透明色）
     * @return 当前 QColor
     */
    QColor color() const;

    /**
     * 设置当前颜色（会同步到所有控件并发出 colorChanged 信号）
     * @param c 目标颜色
     */
    void setColor(const QColor& c);

    /**
     * 设置是否启用 Alpha 编辑
     * @param enabled 为 true 显示并允许编辑 alpha；否则隐藏并固定为 255
     */
    void setAlphaEnabled(bool enabled);

    /**
     * 返回是否启用 Alpha 编辑
     * @return 是否启用 alpha
     */
    bool alphaEnabled() const;

    /**
     * 设置选择器造型（三角/方形），与 ColorDialog 一致
     * @param shape 选择器形状
     */
    void setWheelShape(color_widgets::ColorWheel::ShapeEnum shape);

    /**
     * 获取选择器造型
     * @return 选择器形状
     */
    color_widgets::ColorWheel::ShapeEnum wheelShape() const;

    /**
     * 设置颜色空间（HSV/HSL/LCH），与 ColorDialog 一致
     * @param space 颜色空间
     */
    void setColorSpace(color_widgets::ColorWheel::ColorSpaceEnum space);

    /**
     * 获取颜色空间
     * @return 颜色空间
     */
    color_widgets::ColorWheel::ColorSpaceEnum colorSpace() const;

    /**
     * 设置是否旋转内部选择器
     * @param rotating 是否旋转
     */
    void setWheelRotating(bool rotating);

    /**
     * 获取是否旋转内部选择器
     * @return 是否旋转
     */
    bool wheelRotating() const;

    /**
     * 设置是否镜像内部选择器
     * @param mirrored 是否镜像
     */
    void setWheelMirrored(bool mirrored);

    /**
     * 获取是否镜像内部选择器
     * @return 是否镜像
     */
    bool wheelMirrored() const;
    /**
     * 获取当前颜色（当禁用 alpha 编辑时，返回的不透明色）
     * @return 当前 QColor
     */
    QColor getColor() const;

signals:
    /**
     * 颜色变化信号：当用户操作或 setColor() 时触发
     * @param c 当前颜色
     */
    void colorChanged(const QColor& c);

    /**
     * 颜色“选定”信号：当用户点击预览或完成编辑时触发（可用于“应用”）
     * @param c 当前颜色
     */
    void colorSelected(const QColor& c);

    /**
     * Alpha 可用状态变化
     * @param enabled 是否可用
     */
    void alphaEnabledChanged(bool enabled);

    /**
     * 以下信号与 ColorWheel 保持一致（转发），用于外部联动
     */
    void colorSpaceChanged(color_widgets::ColorWheel::ColorSpaceEnum space);
    void wheelShapeChanged(color_widgets::ColorWheel::ShapeEnum shape);
    void wheelRotatingChanged(bool rotating);
    void wheelMirroredChanged(bool mirrored);

public slots:
    /**
     * HSV 三要素变化时的同步更新（Hue/Saturation/Value 之一变化）
     */
    void set_hsv();

    /**
     * Alpha 变化时的同步更新
     */
    void set_alpha();

    /**
     * RGB 三要素变化时的同步更新（Red/Green/Blue 之一变化）
     */
    void set_rgb();

    /**
     * Hex 输入框（ColorLineEdit）内容变化
     * @param c 变化后的颜色
     */
    void on_edit_hex_colorChanged(const QColor& c);

    /**
     * Hex 输入框编辑完成
     * @param c 编辑完成时的颜色
     */
    void on_edit_hex_colorEditingFinished(const QColor& c);

    /**
     * 点击“拾取屏幕颜色”按钮后开始拾色
     */
    void on_pick_color();

protected:
    /**
     * 鼠标释放事件：拾色模式下，结束拾色并更新颜色
     * @param event 鼠标事件
     */
    void mouseReleaseEvent(QMouseEvent* event) override;

    /**
     * 鼠标移动事件：拾色模式下，实时更新颜色
     * @param event 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    /**
     * 内部：统一更新颜色到控件（阻塞信号），并发出 colorChanged
     * @param col 目标颜色
     * @param updateWheel 是否更新色轮（来自色轮时应为 false，避免回环）
     */
    void setColorInternal(const QColor& col, bool updateWheel = true);

    /**
     * 内部：创建并布置所有子控件
     */
    void buildUI();

    /**
     * 内部：连接所有信号槽
     */
    void connectSignals();

private:
    // 组件
    color_widgets::ColorWheel* wheel =new color_widgets::ColorWheel(this);
    color_widgets::ColorPreview* preview =new color_widgets::ColorPreview(this);
    color_widgets::ColorLineEdit* editHex =new color_widgets::ColorLineEdit(this);

    color_widgets::HueSlider* slideHue =new color_widgets::HueSlider(this);
    color_widgets::GradientSlider* slideSaturation =new color_widgets::GradientSlider(this);
    color_widgets::GradientSlider* slideValue =new color_widgets::GradientSlider(this);

    color_widgets::GradientSlider* slideRed =new color_widgets::GradientSlider(this);
    color_widgets::GradientSlider* slideGreen =new color_widgets::GradientSlider(this);
    color_widgets::GradientSlider* slideBlue =new color_widgets::GradientSlider(this);
    color_widgets::GradientSlider* slideAlpha =new color_widgets::GradientSlider(this);
public :
    QSpinBox* spinHue =new QSpinBox(this);
    QSpinBox* spinSaturation =new QSpinBox(this);
    QSpinBox* spinValue =new QSpinBox(this);
    QSpinBox* spinRed =new QSpinBox(this);
    QSpinBox* spinGreen =new QSpinBox(this);
    QSpinBox* spinBlue =new QSpinBox(this);
    QSpinBox* spinAlpha =new QSpinBox(this);

    QPushButton* pickButton =new QPushButton(tr("Pick Color"),this);

    // 状态
    bool m_alphaEnabled {true};
    bool m_picking {false};
    QColor m_color;
};