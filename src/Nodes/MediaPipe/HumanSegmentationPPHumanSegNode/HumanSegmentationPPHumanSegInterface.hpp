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
class HumanSegmentationPPHumanSegInterface final : public QWidget
{
    Q_OBJECT
public:
    explicit HumanSegmentationPPHumanSegInterface(QWidget* parent = nullptr)
    {
        main_layout = new QGridLayout(this);
        main_layout->setContentsMargins(4, 2, 4, 4);
        main_layout->setSpacing(6);

        MaxFpsFilter = new FloatDragValueWidget(this);
        MaxFpsFilter->setRange(1, 30);
        MaxFpsFilter->setSingleStep(1);
        MaxFpsFilter->setDecimals(0);
        MaxFpsFilter->setValue(15);
        main_layout->addWidget(new QLabel("最大帧率 (FPS):", this), 0, 0);
        main_layout->addWidget(MaxFpsFilter, 0, 1);

        UseGpuCheck = new QCheckBox("使用 GPU (OpenCV CUDA DNN)", this);
        UseGpuCheck->setChecked(true);
        main_layout->addWidget(UseGpuCheck, 1, 0, 1, 2);

        main_layout->addWidget(new QLabel("启停 (ENABLE 端口):", this), 2, 0);
        EnableBtn = new QPushButton("启动分割", this);
        EnableBtn->setCheckable(true);
        main_layout->addWidget(EnableBtn, 2, 1);
        main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 3, 0, 1, 2);
        main_layout->setRowStretch(3, 1);
        main_layout->setColumnStretch(0, 1);
        main_layout->setColumnStretch(1, 2);
        setLayout(main_layout);
    }

    QGridLayout* main_layout = nullptr;
    FloatDragValueWidget* MaxFpsFilter = nullptr;
    QCheckBox* UseGpuCheck = nullptr;
    QPushButton* EnableBtn = nullptr;
};
} // namespace Nodes
