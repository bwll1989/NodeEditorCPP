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
// COCO数据集类别名称
static const std::vector<std::string> classNames = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
    "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
    "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
    "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
    "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
    "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
    "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
    "toothbrush"
};
// 预定义颜色（为每个类别分配不同颜色）
static const std::vector<cv::Scalar> classColors = {
    cv::Scalar(255, 0, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 0, 255),
    cv::Scalar(255, 255, 0), cv::Scalar(255, 0, 255), cv::Scalar(0, 255, 255),
    cv::Scalar(128, 0, 0), cv::Scalar(0, 128, 0), cv::Scalar(0, 0, 128),
    cv::Scalar(128, 128, 0), cv::Scalar(128, 0, 128), cv::Scalar(0, 128, 128),
    cv::Scalar(255, 128, 0), cv::Scalar(255, 0, 128), cv::Scalar(128, 255, 0),
    cv::Scalar(0, 255, 128), cv::Scalar(128, 0, 255), cv::Scalar(0, 128, 255),
    cv::Scalar(192, 192, 192), cv::Scalar(128, 128, 128), cv::Scalar(255, 165, 0),
    cv::Scalar(255, 20, 147), cv::Scalar(0, 191, 255), cv::Scalar(50, 205, 50),
    cv::Scalar(220, 20, 60), cv::Scalar(255, 105, 180), cv::Scalar(75, 0, 130),
    cv::Scalar(255, 215, 0), cv::Scalar(32, 178, 170), cv::Scalar(255, 69, 0)
};
namespace Nodes
{
class ObjectDetectionInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit ObjectDetectionInterface(
            const std::vector<std::string>& names = classNames,
            QWidget *parent = nullptr)
        {
            main_layout = new QGridLayout(this);
            main_layout->setContentsMargins(4, 2, 4, 4);
            main_layout->setSpacing(6);

            ConfidenceFilter = new FloatDragValueWidget(this);
            ConfidenceFilter->setRange(0, 1);
            ConfidenceFilter->setSingleStep(0.01);
            ConfidenceFilter->setDecimals(2);
            ConfidenceFilter->setValue(0.4);
            main_layout->addWidget(new QLabel("置信度阈值:", this), 0, 0);
            main_layout->addWidget(ConfidenceFilter, 0, 1);

            ClassSelectorComboBox = new QComboBox(this);
            for (const auto &name : names) {
                ClassSelectorComboBox->addItem(QString::fromStdString(name));
            }
            ClassSelectorComboBox->setCurrentIndex(0);
            main_layout->addWidget(new QLabel("检测对象:", this), 1, 0);
            main_layout->addWidget(ClassSelectorComboBox, 1, 1);

            MaxFpsFilter = new FloatDragValueWidget(this);
            MaxFpsFilter->setRange(1, 30);
            MaxFpsFilter->setSingleStep(1);
            MaxFpsFilter->setDecimals(0);
            MaxFpsFilter->setValue(15);
            main_layout->addWidget(new QLabel("最大帧率 (FPS):", this), 2, 0);
            main_layout->addWidget(MaxFpsFilter, 2, 1);

            DrawOverlayCheck = new QCheckBox("绘制检测框 (关闭可降 CPU)", this);
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
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;
        FloatDragValueWidget* ConfidenceFilter;
        QComboBox *ClassSelectorComboBox;
        FloatDragValueWidget* MaxFpsFilter;
        QCheckBox* DrawOverlayCheck;
        QPushButton *EnableBtn;

    };
}

