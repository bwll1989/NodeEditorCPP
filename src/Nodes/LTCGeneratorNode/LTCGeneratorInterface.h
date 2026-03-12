#ifndef TIMECODEINTERFACE_H
#define TIMECODEINTERFACE_H

#include <QWidget>
#include "TimeCodeDefines.h"
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
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
        QLabel *timeCodeStatusLabel;
        IntDragValueWidget *timeCodeOffsetSpinBox;
        QComboBox* timeCodeTypeComboBox;
        QPushButton* startButton;
        QPushButton* stopButton;
        QPushButton* resetButton;
        FloatDragValueWidget* volumeSlider;

    signals:
        /**
         * 函数级注释：请求开始生成
         */
        void startRequested();
        /**
         * 函数级注释：请求停止生成
         */
        void stopRequested();
        /**
         * 函数级注释：请求时间码归零
         */
        void resetRequested();
        void timeCodeTypeChanged(TimeCodeType type);
        void volumeChanged(float volume);
    public slots:
        void setStatus(bool error,QString message);
        void setTimeStamp(TimeCodeFrame frame);
    private:
        int timeCodeOffset;
    };

    // Helper functions for TimeCodeType string conversion
    QStringList timecode_type_labels();
    QString timecode_type_to_label(TimeCodeType type);
    TimeCodeType timecode_type_from_label(const QString& label, TimeCodeType defaultType = TimeCodeType::PAL);
}
#endif // TIMECODEINTERFACE_H
