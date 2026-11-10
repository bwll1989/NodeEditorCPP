#pragma once

#include <QCheckBox>
#include <QPushButton>
#include <QSlider>
#include <QTimeEdit>
#include <QWidget>

namespace Nodes
{
    class Curve2DDataModel;

    /**
     * @brief Curve2D 节点内嵌面板（GUI 层，从数据模型中分离）。
     *
     * 只负责控件构建与显示同步：大号时间读数 + 进度条 + 启动/循环 + 编辑曲线按钮。
     * 控件交互直接调用模型公有 setter；模型状态变化经信号驱动本类刷新
     * （启动/循环按钮，QSignalBlocker 防回写）。本类不持有任何曲线数据。
     */
    class Curve2DPanel : public QWidget
    {
        Q_OBJECT
    public:
        explicit Curve2DPanel(Curve2DDataModel* model, QWidget* parent = nullptr);

        /** OSC 外部绑定控件访问器（模型注册 ExternalBinding 用）。 */
        QPushButton* playButton() const { return m_playButton; }
        QCheckBox* loopBox() const { return m_loopBox; }

    public slots:
        /** 进度条跟随（播放推进/拖拽/OSC 全经模型 setProgress → publishOutputs 到此）。 */
        void setProgressView(double progress);

        /** 时间读数：已播时间 = progress × 周期（周期或进度变化时刷新）。 */
        void setTimeView(double progress, double period);

    private:
        Curve2DDataModel* m_model = nullptr;   // 模型由节点图拥有，本面板不参与其生命周期
        QTimeEdit* m_timeReadout = nullptr;
        QSlider* m_progressSlider = nullptr;
        QPushButton* m_playButton = nullptr;   // 启动/停止（checkable）
        QCheckBox* m_loopBox = nullptr;
    };
}
