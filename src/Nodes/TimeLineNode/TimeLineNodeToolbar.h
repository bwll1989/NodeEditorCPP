//
// Created by WuBin on 2025/11/1.
//

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
class TimeLineNodeToolBar : public BaseTimelineToolbar{
    Q_OBJECT
public:
    explicit TimeLineNodeToolBar(QWidget* parent = nullptr);
    ~TimeLineNodeToolBar() override;

    std::unordered_map<QString,QAction*> allActions();

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
    // QAction* m_fullscreenAction;
    //设置动作
    // QAction* m_settingsAction;
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
    // QAction* m_outputAction;
    //是否播放
    bool m_isPlaying = false;
    //所有动作
    std::unordered_map<QString, QAction *> m_allActions;
};



