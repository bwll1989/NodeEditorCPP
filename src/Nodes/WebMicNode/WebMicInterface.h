#pragma once

#include <QLabel>
#include <QWidget>
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief Web Mic 参数面板：端口、增益、连接状态
     */
    class WebMicInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kDefaultPort = 9101;

        explicit WebMicInterface(QWidget *parent = nullptr);
        ~WebMicInterface() override;

        IntDragValueWidget *portSpin = nullptr;
        FloatDragValueWidget *gainSpin = nullptr;
        QLabel *statusLabel = nullptr;

        void setStatusText(const QString &text);
    };
}
