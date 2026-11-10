#pragma once

#include <QDialog>

// 全局命名空间类（与 TimeLineNode / GUI Elements 保持一致）
class QCheckBox;
class QLabel;
class QPushButton;
class QSlider;
class Curve2DEditor;
class FloatDragValueWidget;
class IntDragValueWidget;

namespace Nodes
{
    class Curve2DDataModel;

    /**
     * @brief Curve2D 编辑弹窗（GUI 层，从数据模型中分离）。
     *
     * 独立顶层窗口（parent=nullptr + Qt::Window），布局：
     * 工具行（启动 / +曲线 / -曲线 / 闭合 / 添加点 / 删除点 / 适应视图 / 周期 / 采样率）
     * → 进度条 → 点编辑行（视口上方，紧凑单行：当前选中点 X / Y）→ 平面编辑视口。
     *
     * 控件交互直接调用模型公有 setter / 曲线编辑方法；模型状态变化由模型
     * 显式推送（syncCurves / syncClosedBox / syncPointEditors / setProgressView 等），
     * 播放态按钮经 playChanged 信号驱动。本类不持有曲线数据所有权。
     */
    class Curve2DDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit Curve2DDialog(Curve2DDataModel* model);

    public slots:
        /** 把当前曲线集合与选中项推给编辑器（增删曲线/切换选中后由模型调用）。 */
        void syncCurves();

        /** 选中曲线的闭合态 → 闭合复选框（切换选中/改闭合后由模型调用）。 */
        void syncClosedBox();

        /** 选中点 → 点编辑行（X/Y，随选中与数据变化由模型调用）。 */
        void syncPointEditors();

        /** 弹窗进度条跟随（同节点面板进度条，模型 publishOutputs 推送）。 */
        void setProgressView(double progress);

        /** 周期数值框跟随（OSC / load 改动周期时由模型调用）。 */
        void setPeriodView(double period);

        /** 采样率数值框跟随（OSC / load 改动采样率时由模型调用）。 */
        void setRateView(int rate);

        /** 编辑器播放头指示（模型 publishOutputs 推送）。 */
        void setPlaybackFraction(double fraction);

        /** 适应视图（帧覆盖全部曲线）。 */
        void fitView();

        /** 解除视口对当前主路径的引用（模型删除曲线前调用，防悬空）。 */
        void detachViewport();

    private:
        QPushButton* m_dialogPlayButton = nullptr;
        QSlider* m_dialogSlider = nullptr;
        QCheckBox* m_closedBox = nullptr;      // 闭合/开放当前选中曲线
        FloatDragValueWidget* m_periodSpin = nullptr;
        IntDragValueWidget* m_rateSpin = nullptr;
        QLabel* m_pointLabel = nullptr;
        FloatDragValueWidget* m_x = nullptr;   // 选中点 X
        FloatDragValueWidget* m_y = nullptr;   // 选中点 Y
        Curve2DEditor* m_editViewport = nullptr;

        Curve2DDataModel* m_model = nullptr;   // 模型由节点图拥有，弹窗析构先于模型
    };
}
