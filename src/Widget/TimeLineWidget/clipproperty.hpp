#ifndef CLIPPROPERTY_HPP
#define CLIPPROPERTY_HPP

#include <QDialog>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QDialogButtonBox>
#include "AbstractClipModel.hpp"
#include "AbstractClipDelegate.hpp"
#include "timelinemodel.hpp"

class ClipProperty : public QDialog
{
    Q_OBJECT
public:
    explicit ClipProperty(AbstractClipModel* model, TimelineModel* timelineModel, QWidget *parent = nullptr);
    ~ClipProperty() override {
        if (m_delegate) {
            delete m_delegate;
        }
    }

Q_SIGNALS:
    //属性改变信号
    void propertyChanged();

protected:
    /**
     * 设置UI
     */
    void setupUI();
    /**
     * 连接信号
     */
    void connectSignals();
    /**
     * 设置代理
     */
    void setupDelegate();
    /**
     * 更新UI
     */
    void updateUI();

protected slots:
    /**
     * 开始帧改变信号
     */
    void onStartFrameChanged(int value);
    /**
     * 结束帧改变信号
     */
    void onEndFrameChanged(int value);
    /**
     * 剪辑数据改变信号
     */
    void onClipDataChanged();
    /**
     * 应用点击信号
     */
    void onApplyClicked();

private:
    //片段模型
    AbstractClipModel* m_model;
    //时间线模型
    TimelineModel* m_timelineModel;
    //布局
    QVBoxLayout* m_layout;
    //开始帧
    QSpinBox* m_startFrameSpinBox;
    //结束帧
    QSpinBox* m_endFrameSpinBox;
    //代理控件
    QWidget* m_delegateWidget = nullptr;
    //代理
    AbstractClipDelegate* m_delegate = nullptr;
    //按钮框
    QDialogButtonBox* m_buttonBox = nullptr;
};

#endif // CLIPPROPERTY_HPP 