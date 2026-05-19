#include "LTCGeneratorInterface.h"
#include "QtDebug"
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>

using namespace Nodes;
TimeCodeInterface::TimeCodeInterface(QWidget *parent)
    : QWidget(parent)
    , timeCodeLabel(new QLabel(this))
    , timeCodeStatusLabel(new QLabel(this))
    , timeCodeOffsetSpinBox(new IntDragValueWidget(this))
    , timeCodeTypeComboBox(new QComboBox(this))
    , startButton(new QPushButton("Start", this))
    , stopButton(new QPushButton("Stop", this))
    , resetButton(new QPushButton("Reset", this))
    , volumeSlider(new FloatDragValueWidget(this))
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

    addRow("Offset:", timeCodeOffsetSpinBox);
    addRow("Type:", timeCodeTypeComboBox);
    addRow("Volume:", volumeSlider);

    layout->addWidget(startButton);
    layout->addWidget(stopButton);
    layout->addWidget(resetButton);

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    timeCodeOffsetSpinBox->setRange(-100, 100);
    timeCodeOffsetSpinBox->setValue(0);
    
    
    timeCodeTypeComboBox->addItems(timecode_type_labels());
    timeCodeTypeComboBox->setCurrentText(timecode_type_to_label(TimeCodeType::PAL));

    volumeSlider->setRange(-60, 24);
    volumeSlider->setValue(-25);
    volumeSlider->setSuffix(" dB");
    connect(startButton, &QPushButton::clicked, this, &TimeCodeInterface::startRequested);
    connect(stopButton, &QPushButton::clicked, this, &TimeCodeInterface::stopRequested);
    connect(resetButton, &QPushButton::clicked, this, &TimeCodeInterface::resetRequested);
    connect(timeCodeTypeComboBox, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        emit timeCodeTypeChanged(timecode_type_from_label(text, TimeCodeType::PAL));
    });
    connect(volumeSlider, &FloatDragValueWidget::valueChanged, this, &TimeCodeInterface::volumeChanged);
}

QStringList Nodes::timecode_type_labels()
{
    return {
        "Film (24fps)",
        "Film Drop (23.976fps)",
        "NTSC (30fps)",
        "NTSC Drop (29.97fps)",
        "PAL (25fps)"
    };
}

QString Nodes::timecode_type_to_label(TimeCodeType type)
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

TimeCodeType Nodes::timecode_type_from_label(const QString& label, TimeCodeType defaultType)
{
    if (label == "Film (24fps)")          return TimeCodeType::Film;
    if (label == "Film Drop (23.976fps)") return TimeCodeType::Film_DF;
    if (label == "NTSC (30fps)")          return TimeCodeType::NTSC;
    if (label == "NTSC Drop (29.97fps)")  return TimeCodeType::NTSC_DF;
    if (label == "PAL (25fps)")           return TimeCodeType::PAL;
    // 兼容旧工程：历史版本可能保存过"HD (50fps)"，此处回退到默认类型（通常为 PAL）
    if (label == "HD (50fps)")            return defaultType;
    return defaultType;
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
