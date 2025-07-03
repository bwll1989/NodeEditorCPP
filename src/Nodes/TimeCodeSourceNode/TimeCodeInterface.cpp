#include "TimeCodeInterface.h"
#include "QtDebug"
#include "portaudio.h"
#include <QGridLayout>

using namespace Nodes;
TimeCodeInterface::TimeCodeInterface(QWidget *parent)
    : QWidget(parent)
    , deviceComboBox(new QComboBox(this))
    , timeCodeLabel(new QLabel(this))
    , timeCodeTypeLabel(new QLabel(this))
    , timeCodeStatusLabel(new QLabel(this))
    , timeCodeOffsetSpinBox(new QSpinBox(this))
    , channelComboBox(new QComboBox(this))
{
    auto *layout = new QGridLayout(this);
    timeCodeLabel->setAlignment(Qt::AlignCenter);
    timeCodeLabel->setFont(QFont("Arial", 12, QFont::Bold));
    timeCodeLabel->setText("00:00:00.00");
    layout->addWidget(timeCodeLabel,0,0,1,4);

    layout->addWidget(timeCodeStatusLabel,1,0,1,1);
    timeCodeStatusLabel->setText("no signal");

    layout->addWidget(timeCodeTypeLabel,1,1,1,1);
    timeCodeTypeLabel->setText("      ");

    layout->addWidget(timeCodeOffsetSpinBox,1,2,1,2);
    timeCodeOffsetSpinBox->setRange(-100, 100);
    timeCodeOffsetSpinBox->setValue(0);

    layout->addWidget(deviceComboBox,2,0,1,3);
    layout->addWidget(channelComboBox,2,3,1,1);
    deviceListInit();
    deviceComboBox->setCurrentIndex(-1);
    this->setFixedSize(QSize(300,100));
    connect(deviceComboBox,&QComboBox::currentTextChanged,this,&TimeCodeInterface::initChannelList);
}   

TimeCodeInterface::~TimeCodeInterface()
{}

void TimeCodeInterface::deviceListInit(){
    PaError paError;
    int     i, numDevices;
    const   PaDeviceInfo *deviceInfo;
    QStringList deviceList;
    paError = Pa_Initialize();
    if (paError != paNoError) {
        return;
    }
    numDevices = Pa_GetDeviceCount();
    if( numDevices < 0 )
    {
        return;
    }
    for( i=0; i<numDevices; i++ )
    {
        deviceInfo = Pa_GetDeviceInfo( i );

        if (deviceInfo != nullptr && deviceInfo->maxInputChannels>0)
        {
            deviceList << QString("%1: %2").arg(i).arg(deviceInfo->name);
        }
    }
    deviceComboBox->addItems(deviceList);
}

void TimeCodeInterface::initChannelList(const QString &deviceName){
    channelComboBox->clear();
    QStringList channelList;
    int deviceIndex = deviceName.split(":")[0].toInt();
    auto deviceInfo = Pa_GetDeviceInfo(deviceIndex);
    for(int i=0;i<deviceInfo->maxInputChannels;i++){
        channelList << QString("CH%1").arg(i);
    }
    channelComboBox->addItems(channelList);
}
void TimeCodeInterface::setTimeStamp(TimeCodeFrame frame)
{
    frame = timecode_frame_add(frame, timeCodeOffsetSpinBox->value());
    QString ts = QString("%1:%2:%3.%4")
                     .arg(frame.hours, 2, 10, QLatin1Char('0'))
                     .arg(frame.minutes, 2, 10, QLatin1Char('0'))
                     .arg(frame.seconds, 2, 10, QLatin1Char('0'))
                     .arg(frame.frames, 2, 10, QLatin1Char('0'));
    if(frame.type==TimeCodeType::PAL){
        timeCodeTypeLabel->setText("PAL(25fps)");
    }else if(frame.type==TimeCodeType::Film){
        timeCodeTypeLabel->setText("FILM(24fps)");
    }else if(frame.type==TimeCodeType::NTSC){
        timeCodeTypeLabel->setText("NTSC(30fps)");
    }else if(frame.type==TimeCodeType::NTSC_DF){
        timeCodeTypeLabel->setText("NTSC_DF(29.97fps)");
    }else if(frame.type==TimeCodeType::Film_DF){
        timeCodeTypeLabel->setText("Film_DF(23.976fps)");
    }
    timeCodeLabel->setText(ts);
}

void TimeCodeInterface::setStatus(bool error,QString message)
//状态显示
{
    QColor color = error ? Qt::red : Qt::green;
    QPalette pal = timeCodeStatusLabel->palette();
    pal.setColor(timeCodeStatusLabel->foregroundRole(), color);
    timeCodeStatusLabel->setPalette(pal);
    timeCodeStatusLabel->setText(message);
}
