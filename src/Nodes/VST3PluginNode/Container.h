//
// Created by WuBin on 2024/11/28.
//

#ifndef NODEEDITORCPP_CONTAINER_H
#define NODEEDITORCPP_CONTAINER_H

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"

#include <QMetaObject>
#include <QPointer>
#include <QScreen>
#include <QSize>
#include <QWindow>
#include <QWidget>

#include <atomic>

namespace Nodes
{
    // 函数级注释：VST3 插件编辑器宿主窗口容器
    // 继承 QWidget 提供原生 HWND 载体，同时实现 Steinberg::IPlugFrame，
    // 使插件可以主动请求改尺寸（resizeView）——这是插件 UI 与宿主窗口
    // 尺寸保持一致的关键通道。
    class Container : public QWidget, public Steinberg::IPlugFrame {
        Q_OBJECT
public:
        Container(Steinberg::IPtr<Steinberg::IPlugView>& view1, QWidget* parent = nullptr);
        ~Container();

        // 获取窗口的 WId（平台窗口句柄）
        WId getPlatformWindowHandle();
        // 设置窗口大小（逻辑尺寸；不可缩放插件会转为固定尺寸）
        void setWindowSize(int width, int height);
        // 获取窗口的当前尺寸（逻辑尺寸）
        QSize getWindowSize() const;

        // ---- Steinberg::IPlugFrame ----
        // 插件通过 plugFrame->resizeView() 主动请求调整编辑器尺寸
        Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView* plugView,
                                                 Steinberg::ViewRect* newSize) override;
        Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid,
                                                     void** obj) override;
        Steinberg::uint32 PLUGIN_API addRef() override;
        Steinberg::uint32 PLUGIN_API release() override;

    protected:
        void closeEvent(QCloseEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        // show 之后 windowHandle/屏幕才就绪，此时 dpr 才可靠
        void showEvent(QShowEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

    private:
        // 更新当前窗口/屏幕的 devicePixelRatio（物理像素/逻辑像素）
        void updateDpiScale();
        // 按插件报告的物理尺寸设置窗口（物理 -> 逻辑 -> resize + onSize 通知）
        void applyPluginSizeToWindow(const QSize& physical);
        // 把窗口当前逻辑尺寸换算为物理尺寸并 onSize 通知插件
        void notifyPluginCurrentSize();
        // 若插件支持 IPlugViewContentScaleSupport，则把 dpr 同步给插件内部缩放
        void notifyPluginContentScale();
        // DPI/屏幕变化统一处理：更新 dpr、同步插件缩放与尺寸
        void handleDpiChanged();
        // 连接当前屏幕的 DPI 变化信号（跨屏拖动时重连到新屏幕）
        void connectScreenSignals();

        inline QSize logicalToPhysical(const QSize& logical) const {
            return QSize(qRound(logical.width() * m_dpiScale),
                         qRound(logical.height() * m_dpiScale));
        }

        inline QSize physicalToLogical(const QSize& physical) const {
            return QSize(qRound(physical.width() / m_dpiScale),
                         qRound(physical.height() / m_dpiScale));
        }

    private:
        Steinberg::IPtr<Steinberg::IPlugView> view;
        // 当前窗口的 DPI 缩放因子（devicePixelRatio）
        qreal m_dpiScale = 1.0;
        // IPlugFrame 引用计数（对象生命周期由 Qt 父子关系管理，仅做计数）
        std::atomic<Steinberg::int32> m_refCount{1};
        // 插件报告的初始物理尺寸，show 后再应用
        QSize m_initialPhysical;
        bool m_initialApplied = false;
        bool m_attached = false;
        // 当前已连接 DPI 信号的屏幕
        QPointer<QScreen> m_connectedScreen;
        QMetaObject::Connection m_physDpiConn;
        QMetaObject::Connection m_logicDpiConn;
    };
}

#endif //NODEEDITORCPP_CONTAINER_H
