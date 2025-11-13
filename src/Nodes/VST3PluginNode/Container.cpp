//
// Created by WuBin on 2024/11/28.
//

#include "Container.h"
#include "qdebug.h"
#include "public.sdk/samples/vst-hosting/editorhost/source/platform/iwindow.h"
#include <QKeyEvent>
#include <QGuiApplication>
#include <QScreen>
using namespace Steinberg;
using namespace Vst;
using namespace Nodes;

// 函数级注释：构造宿主窗口并按当前 DPI 缩放初始化插件界面
// 该构造函数读取插件的物理像素尺寸，转换为 Qt 逻辑尺寸后设置窗口大小，
// 同时将插件视图附加到本窗口的原生句柄，并建立 DPI 变化的监听。
Container::Container(Steinberg::IPtr<Steinberg::IPlugView> &view1, QWidget *parent): view(view1)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_QuitOnClose, false);

    // 初始化当前 DPI 缩放因子
    updateDpiScale();

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
    // 将插件的“物理像素”尺寸转换为 Qt 的“逻辑尺寸”
    const QSize logicalSize = physicalToLogical(QSize(viewRect.size.width, viewRect.size.height));
    setWindowSize(logicalSize.width(), logicalSize.height());

    // 获取 QWidget 的平台窗口句柄并附加插件视图
    if (view->attached((void*)getPlatformWindowHandle(), Steinberg::kPlatformTypeHWND) != Steinberg::kResultOk) {
        qDebug() << "Failed to attach editor view to QWidget window";
        return;
    }

    // 监听屏幕变化与 DPI 改变，动态更新缩放并重新应用当前窗口尺寸
    if (QWindow* w = windowHandle()) {
        QObject::connect(w, &QWindow::screenChanged, this, [this](QScreen*){
            updateDpiScale();
            const QSize curLogical = size();
            setWindowSize(curLogical.width(), curLogical.height());
            // 同步通知插件物理尺寸
            QSize physical = logicalToPhysical(curLogical);
            Steinberg::ViewRect Size {0,0,physical.width(),physical.height()};
            view->onSize (&Size);
        });
        if (w->screen()) {
            QObject::connect(w->screen(), &QScreen::physicalDotsPerInchChanged, this, [this](){
                updateDpiScale();
                const QSize curLogical = size();
                setWindowSize(curLogical.width(), curLogical.height());
                QSize physical = logicalToPhysical(curLogical);
                Steinberg::ViewRect Size {0,0,physical.width(),physical.height()};
                view->onSize (&Size);
            });
            QObject::connect(w->screen(), &QScreen::logicalDotsPerInchChanged, this, [this](){
                updateDpiScale();
                const QSize curLogical = size();
                setWindowSize(curLogical.width(), curLogical.height());
                QSize physical = logicalToPhysical(curLogical);
                Steinberg::ViewRect Size {0,0,physical.width(),physical.height()};
                view->onSize (&Size);
            });
        }
    }
}
Container::~Container(){

}

// 函数级注释：获取原生窗口句柄（HWND）
WId Container::getPlatformWindowHandle() {
    return winId();
}

// 函数级注释：设置窗口逻辑尺寸（Qt 坐标，自动应用到最小和当前大小）
void Container::setWindowSize(int width, int height) {
    setMinimumSize(QSize(width, height));
    resize(width, height);
}

// 函数级注释：获取窗口当前逻辑尺寸
QSize Container::getWindowSize() const {
    return size();
}

// 函数级注释：更新当前 DPI 缩放因子
// 优先使用窗口的 devicePixelRatio，其次使用主屏幕的 DPR。
// DPR 为物理像素/逻辑像素，故存储为 m_dpiScale，用于：
//  - 逻辑->物理（乘）
//  - 物理->逻辑（除）
void Container::updateDpiScale() {
    qreal dpr = 1.0;
    if (QWindow* w = windowHandle()) {
        dpr = w->devicePixelRatio();
        if (w->screen()) {
            // 某些环境 devicePixelRatio 返回 1，但屏幕 DPR 实际大于 1
            dpr = std::max<qreal>(dpr, w->screen()->devicePixelRatio());
        }
    } else if (QScreen* s = QGuiApplication::primaryScreen()) {
        dpr = s->devicePixelRatio();
    }
    if (dpr <= 0) dpr = 1.0;
    m_dpiScale = dpr;
}

// 函数级注释：窗口逻辑尺寸变化时，通知插件物理像素尺寸
void Container::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (!view)
        return;

    const QSize newLogical = event->size();
    const QSize newPhysical = logicalToPhysical(newLogical);

    Steinberg::ViewRect Size {0,0,newPhysical.width(),newPhysical.height()};
    view->onSize (&Size);
}

// 函数级注释：按键处理（Esc 关闭）
void Container::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        close();
    } else {
        QWidget::keyPressEvent(event);
    }
}

// 函数级注释：窗口关闭通知插件
void Container::closeEvent(QCloseEvent *event) {
    view->removed();
    event->accept();
}