//
// Created by WuBin on 2024/11/28.
//

#include "Container.h"
#include "qdebug.h"
#include "public.sdk/samples/vst-hosting/editorhost/source/platform/iwindow.h"
#include <QKeyEvent>
using namespace Steinberg;
using namespace Vst;
Container::Container(Steinberg::IPtr<Steinberg::IPlugView> &view1, QWidget *parent): view(view1)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_QuitOnClose, false);
    Steinberg::ViewRect plugViewSize {};
    auto result = view->getSize (&plugViewSize);
    if (result != kResultTrue)
    {
        qDebug()<< "Could not get editor view size";
    }

    auto viewRect = Vst::EditorHost::ViewRectToRect (plugViewSize);

    if (view->isPlatformTypeSupported(Steinberg::kPlatformTypeHWND) != Steinberg::kResultTrue)
    {
        qDebug()<< "editor view does not support HWND";
        return;
    }
//    if(!view->canResize())
//    {
//        setFixedSize(viewRect.size.width, viewRect.size.height);
//    }
    setWindowSize(viewRect.size.width, viewRect.size.height);
    // 获取 QWidget 的平台窗口句柄
    if (view->attached((void*)getPlatformWindowHandle(), Steinberg::kPlatformTypeHWND) != Steinberg::kResultOk) {
        qDebug() << "Failed to attach editor view to QWidget window";
        return;
    }
}
Container::~Container(){

}
// 获取窗口的 WId（平台窗口句柄）
WId Container::getPlatformWindowHandle() {
    return winId();  // 返回平台特定的窗口句柄
}

// 设置窗口大小
void Container::setWindowSize(int width, int height) {
    setMinimumSize(QSize(width, height));
    resize(width, height);  // 调整窗口的尺寸
}

// 获取窗口的当前尺寸
QSize Container::getWindowSize() const {
    return size();
}

// 重载 resizeEvent 以打印调整后的尺寸（可选）
void Container::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    auto newSize=QWidget::size();
    if (!view)
        return;
    Steinberg::ViewRect Size {0,0,newSize.width(),newSize.height()};
    view->onSize (&Size);
}


void Container::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        // 按下 Esc 键时关闭窗口
        close();  // 调用 close() 方法关闭窗口
    } else {
        // 如果是其他按键，调用基类的处理
        QWidget::keyPressEvent(event);
    }
}

void Container::closeEvent(QCloseEvent *event) {
    view->removed();
    event->accept();
}