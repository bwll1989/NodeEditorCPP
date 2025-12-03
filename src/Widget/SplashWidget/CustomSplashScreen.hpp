//
// Created by bwll1 on 2024/8/29.
//

#pragma once
#include <QSplashScreen>
#include <QPainter>
#include <QMainWindow>
#include <QPixmap>
#include <QString>

class CustomSplashScreen : public QObject {
    Q_OBJECT

public:
    /**
     * 构造函数
     * 初始化无边框置顶的启动界面，并加载底图。
     */
    CustomSplashScreen();

    /**
     * 析构函数
     * 释放内部创建的 QSplashScreen 资源。
     */
    ~CustomSplashScreen();

public slots:
    /**
     * 更新状态文本
     * 在底图下方白色区域绘制状态文字，并立即刷新显示。
     * @param status 状态描述文本
     */
    void updateStatus(const QString &status);

    /**
     * 隐藏并关闭启动界面
     * 通常在主窗口准备好后调用。
     * @param mainWindow 主窗口指针
     */
    void finish(QMainWindow *mainWindow);

private:
    /**
     * 渲染当前状态到图片
     * 复制底图，在其下方白色区域居中绘制状态文字，再设置到 QSplashScreen。
     */
    void render();

private:
    // 开屏动画部件
    QSplashScreen *splash;
    // 底图（不含文字）
    QPixmap basePixmap;
    // 当前提示文本
    QString statusText;
};

