#ifndef TIMECODEINTERFACE_H
#define TIMECODEINTERFACE_H

#include <QWidget>
#include "TimeCodeDefines.h"
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
namespace Nodes
{
    class TimeCodeInterface : public QWidget
    {
        Q_OBJECT

    public:
        TimeCodeInterface(QWidget *parent = nullptr);
        ~TimeCodeInterface();
        QLabel *timeCodeLabel;
        QLabel *timeCodeTypeLabel;
        QLabel *timeCodeStatusLabel;
        IntDragValueWidget *timeCodeOffsetSpinBox;
    public slots:
        void setStatus(bool error,QString message);
        void setTimeStamp(TimeCodeFrame frame);
    private:
        int timeCodeOffset;
    };
}
#endif // TIMECODEINTERFACE_H
