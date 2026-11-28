//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QGridLayout>
#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include <QPushButton>
#include <QCoreApplication>
#include <QVariantMap>
#include <QComboBox>
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
namespace Nodes
{
class ImageLayoutInterface final : public QWidget{
        Q_OBJECT
    public:
        /**
         * @brief 构造函数：初始化图像布局界面控件（尺寸、布局模式、网格参数、间距与颜色）
         */
        explicit ImageLayoutInterface(QWidget *parent = nullptr){
            main_layout=new QGridLayout();
            // main_layout->addWidget(display,0,0,2,2);
            main_layout->addWidget(sizeWidthLabel,2,0,1,1);
            main_layout->addWidget(widthEdit,2,1,1,1);
            main_layout->addWidget(sizeHeightLabel,3,0,1,1);
            main_layout->addWidget(heightEdit,3,1,1,1);
            main_layout->addWidget(layoutLabel,4,0,1,1);
            main_layout->addWidget(layoutCombo,4,1,1,1);
            main_layout->addWidget(rowsLabel,5,0,1,1);
            main_layout->addWidget(rowsSpin,5,1,1,1);
            main_layout->addWidget(colsLabel,6,0,1,1);
            main_layout->addWidget(colsSpin,6,1,1,1);
            main_layout->addWidget(spacingLabel,7,0,1,1);
            main_layout->addWidget(spacingSpin,7,1,1,1);
            main_layout->addWidget(colorEditButton,8,0,1,2);
            main_layout->setColumnStretch(0,1);
            main_layout->setColumnStretch(1,1);
            // 空行
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 9, 0, 1, 2);
            main_layout->setRowStretch(9, 1);
            this->setLayout(main_layout);

            /* 函数级注释：设置最小尺寸并允许扩展 */
            this->setMinimumSize(220, 260);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            // 基本初始化
            layoutCombo->addItems({QCoreApplication::translate("Nodes::ImageLayoutInterface", "水平"),
                                   QCoreApplication::translate("Nodes::ImageLayoutInterface", "垂直"),
                                   QCoreApplication::translate("Nodes::ImageLayoutInterface", "网格")});
            rowsSpin->setRange(1, 64);
            colsSpin->setRange(1, 64);
            spacingSpin->setRange(0, 1024);

            // 数值校验
            // auto *wValidator = new QIntValidator(1, 32768, widthEdit);
            // widthEdit->setValidator(wValidator);
            // auto *hValidator = new QIntValidator(1, 32768, heightEdit);
            // heightEdit->setValidator(hValidator);
        }

    public:
        QGridLayout *main_layout;
        IntDragValueWidget *widthEdit=new IntDragValueWidget();
        IntDragValueWidget *heightEdit=new IntDragValueWidget();
        // QLabel *display=new QLabel();
        QPushButton *colorEditButton=new QPushButton(QCoreApplication::translate("Nodes::ImageLayoutInterface", "颜色"));
        QComboBox *layoutCombo = new QComboBox();
        IntDragValueWidget *rowsSpin = new IntDragValueWidget();
        IntDragValueWidget *colsSpin = new IntDragValueWidget();
        IntDragValueWidget *spacingSpin = new IntDragValueWidget();
    private:
        QLabel *sizeWidthLabel=new QLabel(QCoreApplication::translate("Nodes::ImageLayoutInterface", "宽度："));
        QLabel *sizeHeightLabel=new QLabel(QCoreApplication::translate("Nodes::ImageLayoutInterface", "高度："));
        QLabel *layoutLabel = new QLabel(QCoreApplication::translate("Nodes::ImageLayoutInterface", "布局："));
        QLabel *rowsLabel = new QLabel(QCoreApplication::translate("Nodes::ImageLayoutInterface", "行数："));
        QLabel *colsLabel = new QLabel(QCoreApplication::translate("Nodes::ImageLayoutInterface", "列数："));
        QLabel *spacingLabel = new QLabel(QCoreApplication::translate("Nodes::ImageLayoutInterface", "间距："));


    };
}
