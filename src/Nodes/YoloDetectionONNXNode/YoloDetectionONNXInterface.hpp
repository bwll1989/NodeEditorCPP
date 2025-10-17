//
// Created by Administrator on 2025/07/13.
//
#pragma once
#include <QComboBox>

#include "QWidget"
#include "QLayout"
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>
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
class YoloDetectionONNXInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit YoloDetectionONNXInterface(QWidget *parent = nullptr) {
            main_layout=new QGridLayout();

            ConfidenceFilterSpinBox=new QDoubleSpinBox();
            ConfidenceFilterSpinBox->setMinimum(0);
            ConfidenceFilterSpinBox->setMaximum(1);
            ConfidenceFilterSpinBox->setSingleStep(0.1);
            ConfidenceFilterSpinBox->setDecimals(2);
            ConfidenceFilterSpinBox->setValue(0.4);
            main_layout->addWidget(new QLabel("置信度阈值:"),0,0,1,1);
            main_layout->addWidget(ConfidenceFilterSpinBox,0,1,1,2);
            ClassSelectorComboBox=new QComboBox();
            for (const auto &name : classNames) {
                ClassSelectorComboBox->addItem(QString::fromStdString(name));
            }
            ClassSelectorComboBox->setCurrentIndex(0);
            main_layout->addWidget(new QLabel("检测对象:"),1,0,1,1);
            main_layout->addWidget(ClassSelectorComboBox,1,1,1,2);
            EnableBtn=new QPushButton("Enable");
            EnableBtn->setCheckable(true);
            main_layout->addWidget(EnableBtn,2,0,1,3);
            this->setLayout(main_layout);
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;
        QDoubleSpinBox *ConfidenceFilterSpinBox;
        QComboBox *ClassSelectorComboBox;
        QPushButton *EnableBtn;

    };
}

