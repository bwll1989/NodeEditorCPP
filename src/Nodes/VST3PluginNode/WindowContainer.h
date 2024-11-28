//
// Created by WuBin on 2024/11/28.
//

#ifndef NODEEDITORCPP_WINDOWCONTAINER_H
#define NODEEDITORCPP_WINDOWCONTAINER_H
#include "pluginterfaces/gui/iplugview.h"
#include "QWindow"
#include "QWidget"
class WindowContainer:public QWidget {
        Q_OBJECT
public:
    WindowContainer(Steinberg::IPtr<Steinberg::IPlugView> &view1,QWidget *parent = nullptr);
    ~WindowContainer();
    // 获取窗口的 WId（平台窗口句柄）
    WId getPlatformWindowHandle();

    // 设置窗口大小
    void setWindowSize(int width, int height) ;

    // 获取窗口的当前尺寸
    QSize getWindowSize() const ;

protected:
//    // 重载 resizeEvent 以打印调整后的尺寸（可选）
    void resizeEvent(QResizeEvent *event) override ;
    void keyPressEvent(QKeyEvent *event) override;
private:
    Steinberg::IPtr<Steinberg::IPlugView> view ;
};




#endif //NODEEDITORCPP_WINDOWCONTAINER_H
