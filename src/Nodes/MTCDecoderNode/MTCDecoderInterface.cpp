#include "MTCDecoderInterface.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QSpacerItem>
#include <QVBoxLayout>
#include "Common/Devices/MtcDevices/mtcreceiver.h"

using namespace Nodes;

MTCDecoderInterface::MTCDecoderInterface(QWidget *parent)
    : QWidget(parent)
    , timeCodeLabel(new QLabel(this))
    , timeCodeStatusLabel(new QLabel(this))
    , timeCodeOffsetSpinBox(new IntDragValueWidget(this))
    , midiDeviceComboBox(new QComboBox(this))
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

    timeCodeStatusLabel->setAlignment(Qt::AlignCenter);
    timeCodeStatusLabel->setText("Status: Idle");
    layout->addWidget(timeCodeStatusLabel);

    addRow(QObject::tr("MIDI输入:"), midiDeviceComboBox);
    addRow(QObject::tr("偏移:"), timeCodeOffsetSpinBox);

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    timeCodeOffsetSpinBox->setRange(-100, 100);
    timeCodeOffsetSpinBox->setValue(0);

    // 枚举系统 MIDI 输入端口
    midiDeviceComboBox->addItems(MtcReceiver::availableDevices());

    connect(midiDeviceComboBox, &QComboBox::currentTextChanged,
            this, &MTCDecoderInterface::midiDeviceChanged);
}

MTCDecoderInterface::~MTCDecoderInterface()
{}

void MTCDecoderInterface::setTimeStamp(TimeCodeFrame frame)
{
    frame = timecode_frame_add(frame, timeCodeOffsetSpinBox->value());
    QString ts = QString("%1:%2:%3.%4")
                     .arg(frame.hours, 2, 10, QLatin1Char('0'))
                     .arg(frame.minutes, 2, 10, QLatin1Char('0'))
                     .arg(frame.seconds, 2, 10, QLatin1Char('0'))
                     .arg(frame.frames, 2, 10, QLatin1Char('0'));
    timeCodeLabel->setText(ts);
}

void MTCDecoderInterface::setStatus(bool error, QString message)
{
    // 去重：文本未变化时跳过 setPalette/setText，避免高频 setStatus 引发无谓重绘
    const QString text = QStringLiteral("Status: %1").arg(message);
    if (timeCodeStatusLabel->text() == text) {
        return;
    }
    QColor color = error ? Qt::red : Qt::green;
    QPalette pal = timeCodeStatusLabel->palette();
    pal.setColor(timeCodeStatusLabel->foregroundRole(), color);
    timeCodeStatusLabel->setPalette(pal);
    timeCodeStatusLabel->setText(text);
}
