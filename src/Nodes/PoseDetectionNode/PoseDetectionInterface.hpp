//
// Created by Administrator on 2025/07/13.
//
#pragma once
#include <QComboBox>

#include "QWidget"
#include "QLayout"
#include "QLabel"
#include <QGridLayout>
#include <QSpacerItem>
#include <QVariantMap>
#include <QPushButton>
#include <QCheckBox>
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
namespace Nodes
{
class PoseDetectionInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit PoseDetectionInterface(QWidget *parent = nullptr) {
            main_layout = new QGridLayout(this);
            main_layout->setContentsMargins(4, 2, 4, 4);
            main_layout->setSpacing(6);

            ConfidenceFilter = new FloatDragValueWidget(this);
            ConfidenceFilter->setRange(0, 1);
            ConfidenceFilter->setSingleStep(0.01);
            ConfidenceFilter->setDecimals(2);
            ConfidenceFilter->setValue(0.25);
            main_layout->addWidget(new QLabel("置信度阈值:", this), 0, 0);
            main_layout->addWidget(ConfidenceFilter, 0, 1);

            NmsFilter = new FloatDragValueWidget(this);
            NmsFilter->setRange(0, 1);
            NmsFilter->setSingleStep(0.01);
            NmsFilter->setDecimals(2);
            NmsFilter->setValue(0.45);
            main_layout->addWidget(new QLabel("NMS阈值:", this), 1, 0);
            main_layout->addWidget(NmsFilter, 1, 1);

            MaxFpsFilter = new FloatDragValueWidget(this);
            MaxFpsFilter->setRange(1, 30);
            MaxFpsFilter->setSingleStep(1);
            MaxFpsFilter->setDecimals(0);
            MaxFpsFilter->setValue(15);
            main_layout->addWidget(new QLabel("最大帧率 (FPS):", this), 2, 0);
            main_layout->addWidget(MaxFpsFilter, 2, 1);

            DrawOverlayCheck = new QCheckBox("绘制姿态 (关闭可降 CPU)", this);
            DrawOverlayCheck->setChecked(true);
            main_layout->addWidget(DrawOverlayCheck, 3, 0, 1, 2);

            main_layout->addWidget(new QLabel("启停 (ENABLE 端口):", this), 4, 0);
            EnableBtn = new QPushButton("启动检测", this);
            EnableBtn->setCheckable(true);
            main_layout->addWidget(EnableBtn, 4, 1);
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 5, 0, 1, 2);
            main_layout->setRowStretch(5, 1);
            main_layout->setColumnStretch(0, 1);
            main_layout->setColumnStretch(1, 2);
            this->setLayout(main_layout);
        };

    public:
        QGridLayout *main_layout;
        FloatDragValueWidget* ConfidenceFilter;
        FloatDragValueWidget* NmsFilter;
        FloatDragValueWidget* MaxFpsFilter;
        QCheckBox* DrawOverlayCheck;
        QPushButton *EnableBtn;
    };
}
