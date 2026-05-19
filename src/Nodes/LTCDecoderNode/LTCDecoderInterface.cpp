#include "LTCDecoderInterface.h"
#include "QtDebug"
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>

using namespace Nodes;
TimeCodeInterface::TimeCodeInterface(QWidget *parent)
    : QWidget(parent)
    , timeCodeLabel(new QLabel(this))
    , timeCodeTypeLabel(new QLabel(this))
    , timeCodeStatusLabel(new QLabel(this))
    , timeCodeOffsetSpinBox(new IntDragValueWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    const auto addRow = [this, layout](const QString& labelText, QWidget* editor) {
        auto *row = new QWidget(this);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(8);

        auto *label = new QLabel(labelText, row);
        rowLayout->addWidget(label, 0);
        rowLayout->addWidget(editor, 1);

        layout->addWidget(row);
    };

    timeCodeLabel->setAlignment(Qt::AlignCenter);
    timeCodeLabel->setFont(QFont("Arial", 16, QFont::Bold));
    timeCodeLabel->setText("00:00:00.00");
    layout->addWidget(timeCodeLabel);

    addRow("type:", timeCodeTypeLabel);
    timeCodeTypeLabel->setText("      ");
    addRow("status:", timeCodeStatusLabel);
    addRow("offset:", timeCodeOffsetSpinBox);
    timeCodeOffsetSpinBox->setRange(-100, 100);
    timeCodeOffsetSpinBox->setValue(0);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    setMinimumSize(200,100);

}   

TimeCodeInterface::~TimeCodeInterface()
{}


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
