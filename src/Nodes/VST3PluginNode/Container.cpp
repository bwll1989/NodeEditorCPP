//
// Created by WuBin on 2024/11/28.
//

#include "Container.h"

#include <QDebug>
#include <QGuiApplication>
#include <QResizeEvent>
#include <QShowEvent>

#include <algorithm>
#include <cstring>

using namespace Steinberg;
using namespace Nodes;

// 函数级注释：构造宿主窗口并附加插件视图
// 关键顺序（VST3 规范）：setFrame(IPlugFrame) 必须在 attached() 之前调用，
// 否则插件后续的 resizeView() 请求无处投递，插件 UI 与窗口尺寸会脱节。
// 初始尺寸在 show() 之后再应用（见 showEvent），因为此时 devicePixelRatio
// 才能可靠取到（构造期窗口句柄/屏幕可能尚未确定）。
Container::Container(Steinberg::IPtr<Steinberg::IPlugView>& view1, QWidget* parent)
    : view(view1)
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_QuitOnClose, false);

    if (!view)
        return;

    // 记录插件报告的初始物理尺寸（Windows 下 HWND 约定为物理像素）
    Steinberg::ViewRect plugViewSize{};
    if (view->getSize(&plugViewSize) != kResultTrue)
        qDebug() << "Could not get editor view size";
    m_initialPhysical = QSize(plugViewSize.getWidth(), plugViewSize.getHeight());

    if (view->isPlatformTypeSupported(Steinberg::kPlatformTypeHWND) != Steinberg::kResultTrue)
    {
        qDebug() << "editor view does not support HWND";
        return;
    }

    // 先注册 IPlugFrame（宿主帧），再附加视图
    view->setFrame(this);

    if (view->attached((void*)getPlatformWindowHandle(), Steinberg::kPlatformTypeHWND) == Steinberg::kResultOk)
    {
        m_attached = true;

        // 初始 dpr（可能不准确，show 后会重新校准）
        updateDpiScale();
        notifyPluginContentScale();
        connectScreenSignals();

        // 屏幕变化（跨屏拖动）时重连 DPI 信号并同步插件尺寸/缩放
        if (QWindow* w = windowHandle())
        {
            QObject::connect(w, &QWindow::screenChanged, this, [this](QScreen*) {
                handleDpiChanged();
                connectScreenSignals();
            });
        }
    }
    else
    {
        qDebug() << "Failed to attach editor view to QWidget window";
    }
}

Container::~Container()
{
    if (view && m_attached)
        view->removed();
    m_attached = false;
}

// 函数级注释：获取原生窗口句柄（HWND）
WId Container::getPlatformWindowHandle()
{
    return winId();
}

// 函数级注释：设置窗口逻辑尺寸
// 不可缩放的插件直接固定窗口尺寸；可缩放插件允许后续被用户/插件双向调整
void Container::setWindowSize(int width, int height)
{
    const QSize sz(width, height);
    if (view && view->canResize() != Steinberg::kResultTrue)
        setFixedSize(sz);
    else
        resize(sz);
}

// 函数级注释：获取窗口当前逻辑尺寸
QSize Container::getWindowSize() const
{
    return size();
}

// 函数级注释：更新当前 DPI 缩放因子
// 优先使用窗口的 devicePixelRatio，其次使用主屏幕的 DPR。
void Container::updateDpiScale()
{
    qreal dpr = 1.0;
    if (QWindow* w = windowHandle())
    {
        dpr = w->devicePixelRatio();
        if (w->screen())
            // 某些环境 devicePixelRatio 返回 1，但屏幕 DPR 实际大于 1
            dpr = std::max<qreal>(dpr, w->screen()->devicePixelRatio());
    }
    else if (QScreen* s = QGuiApplication::primaryScreen())
    {
        dpr = s->devicePixelRatio();
    }
    if (dpr <= 0)
        dpr = 1.0;
    m_dpiScale = dpr;
}

// 函数级注释：show 后应用插件初始尺寸
// 构造期 dpr 可能尚未就绪（窗口句柄/屏幕未定，或弹窗开在副屏），
// 故把"取 dpr + 定窗口尺寸 + onSize 通知插件"整体推迟到首次显示时执行。
void Container::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    updateDpiScale();
    notifyPluginContentScale();

    if (!m_initialApplied && view && m_initialPhysical.width() > 0 && m_initialPhysical.height() > 0)
    {
        m_initialApplied = true;
        applyPluginSizeToWindow(m_initialPhysical);
    }
}

// 函数级注释：窗口逻辑尺寸变化时通知插件物理像素尺寸
// 拖拽缩放时先经 checkSizeConstraint 修正为插件支持的尺寸，
// 避免宿主窗口与插件实际渲染区域不一致。
void Container::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!view || !m_attached)
        return;

    if (view->canResize() == Steinberg::kResultTrue)
    {
        const QSize phys = logicalToPhysical(event->size());
        Steinberg::ViewRect constraint{0, 0, phys.width(), phys.height()};
        view->checkSizeConstraint(&constraint);
        const QSize constrainedLogical = physicalToLogical(QSize(constraint.getWidth(), constraint.getHeight()));

        // 插件不接受该尺寸：排队修正窗口（避免在 resizeEvent 内递归 resize）
        if (constrainedLogical.width() > 0 && constrainedLogical.height() > 0
            && constrainedLogical != event->size())
        {
            QMetaObject::invokeMethod(
                this,
                [this, constrainedLogical]() {
                    setMinimumSize(0, 0);
                    resize(constrainedLogical);
                },
                Qt::QueuedConnection);
        }
    }

    notifyPluginCurrentSize();
}

// 函数级注释：按键处理（Esc 关闭）
void Container::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
        close();
    else
        QWidget::keyPressEvent(event);
}

// 函数级注释：窗口关闭通知插件并解除帧引用
void Container::closeEvent(QCloseEvent* event)
{
    if (view && m_attached)
    {
        view->setFrame(nullptr);
        view->removed();
        m_attached = false;
    }
    event->accept();
}

// ---- Steinberg::IPlugFrame ----

// 函数级注释：插件主动请求调整编辑器尺寸（IPlugFrame::resizeView）
// 经 checkSizeConstraint 约束后应用到宿主窗口；窗口 resize 会再触发
// resizeEvent -> onSize，把最终实际尺寸同步回插件，形成闭环。
Steinberg::tresult PLUGIN_API Container::resizeView(Steinberg::IPlugView* plugView, Steinberg::ViewRect* newSize)
{
    if (!newSize)
        return kInvalidArgument;
    Steinberg::IPlugView* target = plugView ? plugView : view.get();
    if (!target)
        return kInvalidArgument;

    target->checkSizeConstraint(newSize);
    applyPluginSizeToWindow(QSize(newSize->getWidth(), newSize->getHeight()));
    return kResultTrue;
}

// 函数级注释：引用计数（对象生命周期由 Qt 父子关系管理，仅维护计数值）
Steinberg::uint32 PLUGIN_API Container::addRef()
{
    return ++m_refCount;
}

Steinberg::uint32 PLUGIN_API Container::release()
{
    return --m_refCount;
}

// 函数级注释：接口查询：对外暴露 IPlugFrame / FUnknown
Steinberg::tresult PLUGIN_API Container::queryInterface(const Steinberg::TUID _iid, void** obj)
{
    if (!obj)
        return kInvalidArgument;
    *obj = nullptr;
    if (memcmp(_iid, IPlugFrame::iid, sizeof(TUID)) == 0
        || memcmp(_iid, FUnknown::iid, sizeof(TUID)) == 0)
    {
        *obj = static_cast<IPlugFrame*>(this);
        addRef();
        return kResultTrue;
    }
    return kResultFalse;
}

// ---- 内部工具 ----

// 函数级注释：按插件物理尺寸设置窗口，并把最终尺寸回传插件
void Container::applyPluginSizeToWindow(const QSize& physical)
{
    if (physical.width() <= 0 || physical.height() <= 0)
        return;
    const QSize logical = physicalToLogical(physical);
    setWindowSize(logical.width(), logical.height());
    // resize 可能未触发事件（尺寸相同），显式通知一次，保证闭环
    notifyPluginCurrentSize();
}

// 函数级注释：把窗口当前逻辑尺寸换算为物理尺寸并 onSize 通知插件
void Container::notifyPluginCurrentSize()
{
    if (!view || !m_attached)
        return;
    const QSize phys = logicalToPhysical(size());
    if (phys.width() <= 0 || phys.height() <= 0)
        return;
    Steinberg::ViewRect r{0, 0, phys.width(), phys.height()};
    view->onSize(&r);
}

// 函数级注释：同步内容缩放系数（IPlugViewContentScaleSupport）
// 支持该接口的插件（如 JUCE/VSTGUI 系）会按此系数缩放内部 UI 渲染
void Container::notifyPluginContentScale()
{
    if (!view || !m_attached)
        return;
    FUnknownPtr<IPlugViewContentScaleSupport> scaleSupport(view);
    if (scaleSupport.getInterface())
        scaleSupport->setContentScaleFactor((IPlugViewContentScaleSupport::ScaleFactor)m_dpiScale);
}

// 函数级注释：DPI/屏幕变化统一处理
// 保持窗口逻辑尺寸不变，把新的物理尺寸与缩放系数同步给插件
void Container::handleDpiChanged()
{
    updateDpiScale();
    notifyPluginContentScale();
    notifyPluginCurrentSize();
}

// 函数级注释：连接当前屏幕的 DPI 变化信号（跨屏拖动时重连到新屏幕）
void Container::connectScreenSignals()
{
    QWindow* w = windowHandle();
    QScreen* scr = w ? w->screen() : QGuiApplication::primaryScreen();
    if (m_connectedScreen == scr)
        return;

    if (m_physDpiConn)
        QObject::disconnect(m_physDpiConn);
    if (m_logicDpiConn)
        QObject::disconnect(m_logicDpiConn);

    m_connectedScreen = scr;
    if (scr)
    {
        m_physDpiConn = QObject::connect(scr, &QScreen::physicalDotsPerInchChanged, this, [this]() {
            handleDpiChanged();
        });
        m_logicDpiConn = QObject::connect(scr, &QScreen::logicalDotsPerInchChanged, this, [this]() {
            handleDpiChanged();
        });
    }
}
