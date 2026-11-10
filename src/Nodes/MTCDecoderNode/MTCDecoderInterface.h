#ifndef MTCDECODERINTERFACE_H
#define MTCDECODERINTERFACE_H

#include <QWidget>
#include "Common/AppConfig/TimeCodeDefines.h"
#include <QComboBox>
#include <QLabel>
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    /**
     * @brief MTC 解码节点 UI 面板
     * - MIDI 输入设备选择 / 时间码显示 / 偏移 / 状态
     */
    class MTCDecoderInterface : public QWidget
    {
        Q_OBJECT

    public:
        explicit MTCDecoderInterface(QWidget *parent = nullptr);
        ~MTCDecoderInterface();

        QLabel *timeCodeLabel {nullptr};
        QLabel *timeCodeStatusLabel {nullptr};
        IntDragValueWidget *timeCodeOffsetSpinBox {nullptr};
        QComboBox *midiDeviceComboBox {nullptr};

    signals:
        void midiDeviceChanged(const QString &device);

    public slots:
        void setStatus(bool error, QString message);
        void setTimeStamp(TimeCodeFrame frame);
    };
}
#endif // MTCDECODERINTERFACE_H
