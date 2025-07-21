//
// Created by Administrator on 2025/07/13.
//
#ifndef IMAGECOMPAREINTERFACEFACE_HPP
#define IMAGECOMPAREINTERFACEFACE_HPP
#include "QWidget"
#include "QLayout"
#include <QSpinBox>
#include <QVariantMap>
#include <QPushButton>
namespace Nodes
{
class ImageCompareInterface final : public QWidget{
        Q_OBJECT
    public:
        explicit ImageCompareInterface(QWidget *parent = nullptr) {
            main_layout=new QGridLayout();
            methodEdit=new QComboBox();
            methodEdit->addItem("(均方差)MSE");
            methodEdit->addItem("(结构相似性指数)SSIM");
            methodEdit->addItem("(峰值信噪比)PSNR");
            methodEdit->addItem("(图片相似度)直方图");
            methodEdit->setCurrentIndex(1);
            main_layout->addWidget(methodEdit,0,0,1,1);

            main_layout->setColumnStretch(0,1);
            this->setLayout(main_layout);
            // this->setFixedSize(400,200);

        };

    public:
        QGridLayout *main_layout;
        QComboBox *methodEdit;

    };
}
#endif
