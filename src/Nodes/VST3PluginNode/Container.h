//
// Created by WuBin on 2024/11/28.
//

#ifndef NODEEDITORCPP_CONTAINER_H
#define NODEEDITORCPP_CONTAINER_H
#include "pluginterfaces/gui/iplugview.h"
#include "QWindow"
#include "QWidget"

namespace Nodes
{
    class Container: public QWidget {
        Q_OBJECT
public:
        Container(Steinberg::IPtr<Steinberg::IPlugView> &view1, QWidget *parent = nullptr);
        ~Container();
        // 获取窗口的 WId（平台窗口句柄）
        WId getPlatformWindowHandle();
        // 设置窗口大小（逻辑尺寸，已按 DPI 缩放）
        void setWindowSize(int width, int height) ;
        // 获取窗口的当前尺寸（逻辑尺寸）
        QSize getWindowSize() const ;

    protected:
        // 重载 closeEvent 以处理窗口关闭时的行为
        void closeEvent(QCloseEvent* event) override;
        //    // 重载 resizeEvent 以打印调整后的尺寸（可选）
        void resizeEvent(QResizeEvent *event) override ;
        void keyPressEvent(QKeyEvent *event) override;

    private:
        Steinberg::IPtr<Steinberg::IPlugView> view ;
        // 新增：当前窗口的 DPI 缩放因子（devicePixelRatio）
        qreal m_dpiScale = 1.0;

        // 新增：根据当前窗口或屏幕更新 DPI 缩放因子
        void updateDpiScale();

        // 新增：工具函数，逻辑尺寸 -> 物理像素
        inline QSize logicalToPhysical(const QSize& logical) const {
            return QSize(qRound(logical.width() * m_dpiScale),
                         qRound(logical.height() * m_dpiScale));
        }

        // 新增：工具函数，物理像素 -> 逻辑尺寸
        inline QSize physicalToLogical(const QSize& physical) const {
            return QSize(qRound(physical.width() / m_dpiScale),
                         qRound(physical.height() / m_dpiScale));
        }
    };
}

#endif //NODEEDITORCPP_CONTAINER_H
