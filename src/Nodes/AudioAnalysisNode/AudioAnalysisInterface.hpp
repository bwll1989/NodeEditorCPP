#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

namespace Nodes
{
    class AudioAnalysisInterface : public QWidget
    {
    public:
        explicit AudioAnalysisInterface(QWidget *parent = nullptr)
        {
            auto *mainLayout = new QVBoxLayout(this);
            mainLayout->setContentsMargins(0, 0, 0, 5);

            auto *bandGroup = new QGroupBox(QStringLiteral("频段 (Hz)"), this);
            auto *bandLayout = new QGridLayout(bandGroup);

            int row = 0;
            bandLayout->addWidget(new QLabel(QStringLiteral("LOW 最小")), row, 0);
            lowMinHz->setRange(10.0, 20000.0);
            lowMinHz->setDecimals(0);
            lowMinHz->setValue(20.0);
            lowMinHz->setSuffix(QStringLiteral(" Hz"));
            bandLayout->addWidget(lowMinHz, row++, 1);

            bandLayout->addWidget(new QLabel(QStringLiteral("LOW 最大")), row, 0);
            lowMaxHz->setRange(10.0, 20000.0);
            lowMaxHz->setDecimals(0);
            lowMaxHz->setValue(250.0);
            lowMaxHz->setSuffix(QStringLiteral(" Hz"));
            bandLayout->addWidget(lowMaxHz, row++, 1);

            bandLayout->addWidget(new QLabel(QStringLiteral("MID 最小")), row, 0);
            midMinHz->setRange(10.0, 20000.0);
            midMinHz->setDecimals(0);
            midMinHz->setValue(250.0);
            midMinHz->setSuffix(QStringLiteral(" Hz"));
            bandLayout->addWidget(midMinHz, row++, 1);

            bandLayout->addWidget(new QLabel(QStringLiteral("MID 最大")), row, 0);
            midMaxHz->setRange(10.0, 20000.0);
            midMaxHz->setDecimals(0);
            midMaxHz->setValue(4000.0);
            midMaxHz->setSuffix(QStringLiteral(" Hz"));
            bandLayout->addWidget(midMaxHz, row++, 1);

            bandLayout->addWidget(new QLabel(QStringLiteral("HIGH 最小")), row, 0);
            highMinHz->setRange(10.0, 20000.0);
            highMinHz->setDecimals(0);
            highMinHz->setValue(4000.0);
            highMinHz->setSuffix(QStringLiteral(" Hz"));
            bandLayout->addWidget(highMinHz, row++, 1);

            bandLayout->addWidget(new QLabel(QStringLiteral("HIGH 最大")), row, 0);
            highMaxHz->setRange(10.0, 20000.0);
            highMaxHz->setDecimals(0);
            highMaxHz->setValue(20000.0);
            highMaxHz->setSuffix(QStringLiteral(" Hz"));
            bandLayout->addWidget(highMaxHz, row++, 1);

            auto *detectGroup = new QGroupBox(QStringLiteral("检测参数"), this);
            auto *detectLayout = new QGridLayout(detectGroup);
            row = 0;

            detectLayout->addWidget(new QLabel(QStringLiteral("分析帧长")), row, 0);
            frameSize->setRange(256, 8192);
            frameSize->setSingleStep(256);
            frameSize->setValue(2048);
            frameSize->setSuffix(QStringLiteral(" samples"));
            detectLayout->addWidget(frameSize, row++, 1);

            detectLayout->addWidget(new QLabel(QStringLiteral("节拍间隔")), row, 0);
            beatIntervalMs->setRange(50, 2000);
            beatIntervalMs->setSingleStep(10);
            beatIntervalMs->setValue(120);
            beatIntervalMs->setSuffix(QStringLiteral(" ms"));
            detectLayout->addWidget(beatIntervalMs, row++, 1);

            detectLayout->addWidget(new QLabel(QStringLiteral("频段 Attack")), row, 0);
            bandAttackMs->setRange(1.0, 1000.0);
            bandAttackMs->setDecimals(1);
            bandAttackMs->setValue(20.0);
            bandAttackMs->setSuffix(QStringLiteral(" ms"));
            detectLayout->addWidget(bandAttackMs, row++, 1);

            detectLayout->addWidget(new QLabel(QStringLiteral("频段 Release")), row, 0);
            bandReleaseMs->setRange(1.0, 2000.0);
            bandReleaseMs->setDecimals(1);
            bandReleaseMs->setValue(120.0);
            bandReleaseMs->setSuffix(QStringLiteral(" ms"));
            detectLayout->addWidget(bandReleaseMs, row++, 1);

            mainLayout->addWidget(bandGroup);
            mainLayout->addWidget(detectGroup);

            enableButton->setText(QStringLiteral("Enable"));
            enableButton->setCheckable(true);
            enableButton->setChecked(false);
            enableButton->setMinimumWidth(80);
            mainLayout->addWidget(enableButton);

            setMinimumWidth(260);
        }

        QPushButton *enableButton = new QPushButton();
        FloatDragValueWidget *lowMinHz = new FloatDragValueWidget();
        FloatDragValueWidget *lowMaxHz = new FloatDragValueWidget();
        FloatDragValueWidget *midMinHz = new FloatDragValueWidget();
        FloatDragValueWidget *midMaxHz = new FloatDragValueWidget();
        FloatDragValueWidget *highMinHz = new FloatDragValueWidget();
        FloatDragValueWidget *highMaxHz = new FloatDragValueWidget();
        IntDragValueWidget *frameSize = new IntDragValueWidget();
        IntDragValueWidget *beatIntervalMs = new IntDragValueWidget();
        FloatDragValueWidget *bandAttackMs = new FloatDragValueWidget();
        FloatDragValueWidget *bandReleaseMs = new FloatDragValueWidget();
    };
}
