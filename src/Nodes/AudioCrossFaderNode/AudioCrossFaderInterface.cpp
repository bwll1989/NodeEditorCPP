#include "AudioCrossFaderInterface.h"
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>

using namespace Nodes;

/**
 * @brief 构造 AudioCrossFader 节点参数界面
 * 函数级注释：使用垂直弹簧（row stretch）吸收多余高度，避免窗口上下拉伸时控件间距被拉大。
 */
AudioCrossFaderInterface::AudioCrossFaderInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QGridLayout(this);
    
    // Mix
    mainLayout->addWidget(new QLabel("Mix (0=A, 1=B):"), 0, 0);
    mixSpin = new FloatDragValueWidget();
    mixSpin->setRange(0.0, 1.0);
    mixSpin->setDecimals(3);
    mixSpin->setSingleStep(0.01);
    mixSpin->setValue(0.5);
    mainLayout->addWidget(mixSpin, 0, 1);

    // Note: Direction and Curve comboboxes removed per request
    
    // Fade Duration
    mainLayout->addWidget(new QLabel("Fade Time (ms):"), 1, 0);
    fadeDurationSpin = new FloatDragValueWidget();
    fadeDurationSpin->setRange(10.0, 600000.0);
    fadeDurationSpin->setDecimals(1);
    fadeDurationSpin->setSingleStep(10.0);
    fadeDurationSpin->setValue(2000.0);
    fadeDurationSpin->setSuffix(" ms");
    mainLayout->addWidget(fadeDurationSpin, 1, 1);

    // Action Combo
    mainLayout->addWidget(new QLabel("Action:"), 2, 0);
    actionCombo = new QComboBox();
    actionCombo->addItem("Reset Mix");
    actionCombo->addItem("Start A -> B");
    actionCombo->addItem("Start B -> A");
    
    mainLayout->addWidget(actionCombo, 2, 1);

    mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 2);
    mainLayout->setRowStretch(3, 1);
    setMinimumWidth(250);
}

AudioCrossFaderInterface::~AudioCrossFaderInterface()
{}
