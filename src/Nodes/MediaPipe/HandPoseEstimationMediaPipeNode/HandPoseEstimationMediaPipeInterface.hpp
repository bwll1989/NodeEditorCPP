#pragma once

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QWidget>

namespace Nodes
{
class HandPoseEstimationMediaPipeInterface final : public QWidget
{
    Q_OBJECT
public:
    explicit HandPoseEstimationMediaPipeInterface(QWidget* parent = nullptr)
    {
        main_layout = new QGridLayout(this);
        main_layout->setContentsMargins(4, 2, 4, 4);
        main_layout->setSpacing(6);

        PalmScoreFilter = new FloatDragValueWidget(this);
        PalmScoreFilter->setRange(0, 1);
        PalmScoreFilter->setSingleStep(0.01);
        PalmScoreFilter->setDecimals(2);
        PalmScoreFilter->setValue(0.3);
        main_layout->addWidget(new QLabel("手掌检测阈值:", this), 0, 0);
        main_layout->addWidget(PalmScoreFilter, 0, 1);

        NmsFilter = new FloatDragValueWidget(this);
        NmsFilter->setRange(0, 1);
        NmsFilter->setSingleStep(0.01);
        NmsFilter->setDecimals(2);
        NmsFilter->setValue(0.3);
        main_layout->addWidget(new QLabel("NMS阈值:", this), 1, 0);
        main_layout->addWidget(NmsFilter, 1, 1);

        ConfidenceFilter = new FloatDragValueWidget(this);
        ConfidenceFilter->setRange(0, 1);
        ConfidenceFilter->setSingleStep(0.01);
        ConfidenceFilter->setDecimals(2);
        ConfidenceFilter->setValue(0.3);
        main_layout->addWidget(new QLabel("手部置信度:", this), 2, 0);
        main_layout->addWidget(ConfidenceFilter, 2, 1);

        MaxFpsFilter = new FloatDragValueWidget(this);
        MaxFpsFilter->setRange(1, 30);
        MaxFpsFilter->setSingleStep(1);
        MaxFpsFilter->setDecimals(0);
        MaxFpsFilter->setValue(15);
        main_layout->addWidget(new QLabel("最大帧率 (FPS):", this), 3, 0);
        main_layout->addWidget(MaxFpsFilter, 3, 1);

        UseGpuCheck = new QCheckBox("使用 GPU (OpenCV CUDA DNN)", this);
        UseGpuCheck->setChecked(true);
        main_layout->addWidget(UseGpuCheck, 4, 0, 1, 2);

        DrawOverlayCheck = new QCheckBox("绘制手部骨架 (关闭可降 CPU)", this);
        DrawOverlayCheck->setChecked(true);
        main_layout->addWidget(DrawOverlayCheck, 5, 0, 1, 2);

        main_layout->addWidget(new QLabel("启停 (ENABLE 端口):", this), 6, 0);
        EnableBtn = new QPushButton("启动检测", this);
        EnableBtn->setCheckable(true);
        main_layout->addWidget(EnableBtn, 6, 1);
        main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 7, 0, 1, 2);
        main_layout->setRowStretch(7, 1);
        main_layout->setColumnStretch(0, 1);
        main_layout->setColumnStretch(1, 2);
        setLayout(main_layout);
    }

    QGridLayout* main_layout = nullptr;
    FloatDragValueWidget* PalmScoreFilter = nullptr;
    FloatDragValueWidget* NmsFilter = nullptr;
    FloatDragValueWidget* ConfidenceFilter = nullptr;
    FloatDragValueWidget* MaxFpsFilter = nullptr;
    QCheckBox* UseGpuCheck = nullptr;
    QCheckBox* DrawOverlayCheck = nullptr;
    QPushButton* EnableBtn = nullptr;
};
} // namespace Nodes
