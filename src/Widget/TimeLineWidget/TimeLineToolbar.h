//
// Created by WuBin on 2025/11/1.
//

#pragma once

#include <QAbstractSlider>
#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include "OSCMessage.h"
#include "TimeLineStyle.h"
#include "TimelineExports.hpp"
#include <QMimeData>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include "DefaultTimeLineToolBar.h"
#include "Common/Devices/StatusContainer/StatusContainer.h"
#pragma once

#include <QToolBar>
#include <QAction>
#include <QApplication>
#include <QSpinBox>
#include <QLabel>
#include "OSCMessage.h"
#include "TimeLineStyle.h"
#include "TimelineExports.hpp"
#include <QMimeData>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include "BaseTimeLineToolBar.h"
class TimeLineToolBar : public BaseTimelineToolbar{
    Q_OBJECT
public:
    explicit TimeLineToolBar(QWidget* parent = nullptr);
    ~TimeLineToolBar() override;


signals:
    /**
     * 播放点击
     */
    void playClicked();
    /**
     * 停止点击
     */
    void stopClicked();
    /**
     * 暂停点击
     */
    void pauseClicked();
    /**
     * 循环点击
     * @param bool enabled 是否启用
     */
    void loopToggled(bool enabled);
    /**
     * 输出窗口点击
     * @param bool show 是否显示
     */
    void outputWindowToggled(bool show);
    /**
     * 设置点击
     */
    void settingsClicked();
    /**
     * 上一帧点击
     */
    void prevFrameClicked();
    /**
     * 下一帧点击
     */
    void nextFrameClicked();
    /**
     * 移动剪辑点击
     */
    void moveClipClicked(int dx);
    /**
     * 删除剪辑点击
     */
    void deleteClipClicked();
    /**
     * 放大点击
     */
    void zoomInClicked();
    /**
     * 缩小点击
     */
    void zoomOutClicked();
    /**
     *
     */
    void locationClicked();
public:
    void registerOSCControl(const QString& oscAddress, QAction* control) override
    {
        // 如果oscAddress不以"/"开头，则不注册
        if (!oscAddress.startsWith("/")) return;
        // 构建完整的OSC地址，自动给OSC地址添加前缀，包括节点ID
        if (!control) return;
        // 如果已存在相同地址的映射，先移除旧的
        auto it = _OscMapping.get()->find(oscAddress);
        if (it != _OscMapping.get()->end()) {
            _OscMapping.get()->erase(it);
        }

        // 添加新的映射
        QWidget* widget = widgetForAction(control);
        if (widget) {
            widget->installEventFilter(this);
        }
        (*_OscMapping)[oscAddress] = control;
        StatusContainer::instance()->registerWidget(widget,makeFullOscAddress(oscAddress));
        registerOSCFeedBack(makeFullOscAddress(oscAddress),widget);
    }

    void registerOSCFeedBack(const QString& oscAddress, QWidget* feedback)
    {

        // 绑定值变化信号到 stateFeedBack
        if (auto* button = qobject_cast<QAbstractButton*>(feedback)) {
            if (button->isCheckable()){
                this->stateFeedBack(oscAddress, QVariant(button->isChecked()));
                QObject::connect(button, &QAbstractButton::toggled, this, [this, oscAddress](bool checked) {
                    this->stateFeedBack(oscAddress, QVariant(checked));
                });
            }else {
                QObject::connect(button, &QAbstractButton::pressed, this, [this, oscAddress]() {
                this->stateFeedBack(oscAddress, QVariant(1));
            });
                QObject::connect(button, &QAbstractButton::released, this, [this, oscAddress]() {
                this->stateFeedBack(oscAddress, QVariant(0));
            });
            }

        } else if (auto* slider = qobject_cast<QAbstractSlider*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(slider->value()));
            QObject::connect(slider, &QAbstractSlider::valueChanged, this, [this, oscAddress](int value) {
                this->stateFeedBack(oscAddress, QVariant(value));
            });
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(spinBox->value()));
            QObject::connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, oscAddress](int value) {
                this->stateFeedBack(oscAddress, QVariant(value));
            });
        } else if (auto* lineEdit = qobject_cast<QLineEdit*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(lineEdit->text()));
            QObject::connect(lineEdit, &QLineEdit::textChanged, this, [this, oscAddress](const QString& text) {
                this->stateFeedBack(oscAddress, QVariant(text));
            });
        } else if (auto* comboBox = qobject_cast<QComboBox*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(comboBox->currentIndex()));
            QObject::connect(comboBox, &QComboBox::currentIndexChanged, this, [this, oscAddress](int value) {
                this->stateFeedBack(oscAddress, QVariant(value));
            });
        } else if (auto* checkBox = qobject_cast<QCheckBox*>(feedback)) {
            this->stateFeedBack(oscAddress, QVariant(checkBox->isChecked()));
            QObject::connect(checkBox, &QCheckBox::checkStateChanged, this, [this, oscAddress](int state) {
                this->stateFeedBack(oscAddress, QVariant(state));
            });
        }
    }


    QString makeFullOscAddress(const QString& oscAddress) const {
        // 函数级注释：根据节点ID添加前缀
        return  "/timeline/default/toolbar" + oscAddress;
    }
    void stateFeedBack(const QString& oscAddress, QVariant value) {
        // 函数级注释：默认将状态写入到 StatusContainer，仅更新值
        StatusContainer::instance()->updateState(oscAddress, value);
    }
public slots:
    /**
     * 设置播放状态
     * @param bool isPlaying 是否播放
     */
    void setPlaybackState(bool isPlaying);
    /**
    * 设置循环状态
    * @param bool isLooping 是否循环
    */
    void setLoopState(bool isLooping);
private:
    /**
     * 创建动作
     */
    void createActions() override;
    /**
     * 设置UI
     */
    void setupUI() override;


private:

    //播放动作
    QAction* m_playAction;
    //停止动作
    QAction* m_stopAction;
    //暂停动作
    QAction* m_pauseAction;
    //循环动作
    QAction* m_loopAction;
    //下一帧动作
    QAction* m_nextFrameAction;
    //上一帧动作
    QAction* m_previousFrameAction;
    //下一媒体动作
    QAction* m_nextMediaAction;
    //上一媒体动作
    QAction* m_previousMediaAction;
    //全屏动作
    QAction* m_fullscreenAction;
    //设置动作
    QAction* m_settingsAction;
    //移动剪辑左动作
    QAction* m_moveClipLeftAction;
    //移动剪辑右动作
    QAction* m_moveClipRightAction;
    //删除剪辑动作
    QAction* m_deleteClipAction;
    //放大动作
    QAction* m_zoomInAction;
    //缩小动作
    QAction* m_zoomOutAction;
    //输出窗口动作
    QAction* m_outputAction;
    //
    QAction* m_locationAction;
    //是否播放
    bool m_isPlaying = false;
};





