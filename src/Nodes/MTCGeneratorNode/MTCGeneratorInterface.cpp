#include "MTCGeneratorInterface.h"
#include <QDebug>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QSpacerItem>
#include <QVBoxLayout>
#include "Common/Devices/MtcSender/mtcsender.h"

using namespace Nodes;

MTCGeneratorInterface::MTCGeneratorInterface(QWidget *parent)
    : QWidget(parent)
    , timeCodeLabel(new QLabel(this))
    , timeCodeStatusLabel(new QLabel(this))
    , timeCodeOffsetSpinBox(new IntDragValueWidget(this))
    , timeCodeTypeComboBox(new QComboBox(this))
    , midiDeviceComboBox(new QComboBox(this))
    , startButton(new QPushButton(QStringLiteral("Start"), this))
    , resetButton(new QPushButton(QStringLiteral("Reset"), this))
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

    addRow(tr("MIDI输出:"), midiDeviceComboBox);
    addRow(tr("偏移:"), timeCodeOffsetSpinBox);
    addRow(tr("类型:"), timeCodeTypeComboBox);

    startButton->setCheckable(true);
    startButton->setAutoExclusive(false);
    layout->addWidget(startButton);
    layout->addWidget(resetButton);

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    timeCodeOffsetSpinBox->setRange(-100, 100);
    timeCodeOffsetSpinBox->setValue(0);

    timeCodeTypeComboBox->addItems(timecodeTypeLabels());
    timeCodeTypeComboBox->setCurrentText(timecodeTypeToLabel(TimeCodeType::PAL));

    // 枚举系统 MIDI 输出端口
    midiDeviceComboBox->addItems(MtcSender::availableDevices());

    connect(startButton, &QPushButton::toggled, this, &MTCGeneratorInterface::runningToggled);
    connect(resetButton, &QPushButton::clicked, this, &MTCGeneratorInterface::resetRequested);
    connect(timeCodeTypeComboBox, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        emit timeCodeTypeChanged(timecodeTypeFromLabel(text, TimeCodeType::PAL));
    });
    connect(midiDeviceComboBox, &QComboBox::currentTextChanged,
            this, &MTCGeneratorInterface::midiDeviceChanged);
}

void MTCGeneratorInterface::setRunningChecked(bool running)
{
    QSignalBlocker blocker(startButton);
    startButton->setChecked(running);
    startButton->setText(running ? QStringLiteral("Stop") : QStringLiteral("Start"));
}

QStringList MTCGeneratorInterface::timecodeTypeLabels() const
{
    return {
        "Film (24fps)",
        "Film Drop (23.976fps)",
        "NTSC (30fps)",
        "NTSC Drop (29.97fps)",
        "PAL (25fps)"
    };
}

QString MTCGeneratorInterface::timecodeTypeToLabel(TimeCodeType type) const
{
    switch (type) {
        case TimeCodeType::Film:    return "Film (24fps)";
        case TimeCodeType::Film_DF: return "Film Drop (23.976fps)";
        case TimeCodeType::NTSC:    return "NTSC (30fps)";
        case TimeCodeType::NTSC_DF: return "NTSC Drop (29.97fps)";
        case TimeCodeType::PAL:     return "PAL (25fps)";
        default:                    return "PAL (25fps)";
    }
}

TimeCodeType MTCGeneratorInterface::timecodeTypeFromLabel(const QString& label, TimeCodeType defaultType) const
{
    if (label == "Film (24fps)")          return TimeCodeType::Film;
    if (label == "Film Drop (23.976fps)") return TimeCodeType::Film_DF;
    if (label == "NTSC (30fps)")          return TimeCodeType::NTSC;
    if (label == "NTSC Drop (29.97fps)")  return TimeCodeType::NTSC_DF;
    if (label == "PAL (25fps)")           return TimeCodeType::PAL;
    return defaultType;
}

MTCGeneratorInterface::~MTCGeneratorInterface()
{}

void MTCGeneratorInterface::setTimeStamp(TimeCodeFrame frame)
{
    frame = timecode_frame_add(frame, timeCodeOffsetSpinBox->value());
    QString ts = QString("%1:%2:%3.%4")
                     .arg(frame.hours, 2, 10, QLatin1Char('0'))
                     .arg(frame.minutes, 2, 10, QLatin1Char('0'))
                     .arg(frame.seconds, 2, 10, QLatin1Char('0'))
                     .arg(frame.frames, 2, 10, QLatin1Char('0'));
    timeCodeLabel->setText(ts);
}

void MTCGeneratorInterface::setStatus(bool error, QString message)
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
