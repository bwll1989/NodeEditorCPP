#ifndef TIMECODEINTERFACE_H
#define TIMECODEINTERFACE_H

#include <QWidget>
#include "TimeCodeDefines.h"
#include "../../Common/Devices/LtcReceiver/ltcreceiver.h"
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>

namespace Nodes
{
    class TimeCodeInterface : public QWidget
    {
        Q_OBJECT

    public:
        TimeCodeInterface(QWidget *parent = nullptr);
        ~TimeCodeInterface();
        int audio_device_index;
        void deviceListInit();
        QComboBox *deviceComboBox;
        QComboBox *channelComboBox;
        QLabel *timeCodeLabel;
        QLabel *timeCodeTypeLabel;
        QLabel *timeCodeStatusLabel;
        QSpinBox *timeCodeOffsetSpinBox;
    public slots:
        void initChannelList(const QString &deviceName);
        void setStatus(bool error,QString message);
        void setTimeStamp(TimeCodeFrame frame);
    private:
        int timeCodeOffset;
    };
}
#endif // TIMECODEINTERFACE_H
