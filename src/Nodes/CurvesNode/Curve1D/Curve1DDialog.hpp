#pragma once

#include <QDialog>

// 全局命名空间类（与 TimeLineNode / GUI Elements 保持一致）
class QLabel;
class QPushButton;
class QSlider;
class Curve1DEditor;
class FloatDragValueWidget;
class IntDragValueWidget;

namespace Nodes
{
    class Curve1DDataModel;

    /**
     * @brief Curve1D 编辑弹窗（GUI 层，从数据模型中分离）。
     *
     * 独立顶层窗口（parent=nullptr + Qt::Window），布局：
     * 工具行（启动 / +曲线 / -曲线 / 添加点 / 删除点 / 适应视图 / 周期 / 采样率）
     * → 进度条 → 点编辑行（视口上方，紧凑单行：当前选中点 时间 / 数值）→ 函数图视口。
     *
     * 控件交互直接调用模型公有 setter / 曲线编辑方法；模型状态变化由模型
     * 显式推送（setPeriodView / setProgressView / syncCurves / syncPointEditors 等），
     * 播放态按钮经 playChanged 信号驱动。本类不持有曲线数据所有权。
     */
    class Curve1DDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit Curve1DDialog(Curve1DDataModel* model);

    public slots:
        /** 把当前曲线集合与选中项推给编辑器（增删曲线/切换选中后由模型调用）。 */
        void syncCurves();

        /** 选中点 → 点编辑行（时间/数值，随选中与数据变化由模型调用）。 */
        void syncPointEditors();

        /** 弹窗进度条跟随（同节点面板进度条，模型 publishOutputs 推送）。 */
        void setProgressView(double progress);

        /** 周期显示联动：数值框 / 点编辑行时间范围 / 编辑器 X 轴刻度（QSignalBlocker 防回写）。 */
        void setPeriodView(double period);

        /** 采样率数值框跟随（OSC / load 改动采样率时由模型调用）。 */
        void setRateView(int rate);

        /** 编辑器播放头指示（模型 publishOutputs 推送）。 */
        void setPlaybackFraction(double fraction);

        /** 适应视图（Y 轴自适应当前曲线值域）。 */
        void fitView();

    private:
        QPushButton* m_dialogPlayButton = nullptr;
        QSlider* m_dialogSlider = nullptr;
        FloatDragValueWidget* m_periodSpin = nullptr;
        IntDragValueWidget* m_rateSpin = nullptr;
        QLabel* m_pointLabel = nullptr;
        FloatDragValueWidget* m_x = nullptr;   // 选中点时间（秒，0~周期）
        FloatDragValueWidget* m_y = nullptr;   // 选中点数值
        Curve1DEditor* m_editViewport = nullptr;

        Curve1DDataModel* m_model = nullptr;   // 模型由节点图拥有，弹窗析构先于模型
    };
}
