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
#include <QVariantMap>
#include <QComboBox>
#include <QSpinBox>
#include <QIntValidator>
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
            this->setLayout(main_layout);

            /* 函数级注释：设置最小尺寸并允许扩展 */
            this->setMinimumSize(220, 260);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            // 基本初始化
            layoutCombo->addItems({"Horizontal", "Vertical", "Grid"});
            rowsSpin->setRange(1, 64);
            colsSpin->setRange(1, 64);
            spacingSpin->setRange(0, 1024);

            // 数值校验
            auto *wValidator = new QIntValidator(1, 32768, widthEdit);
            widthEdit->setValidator(wValidator);
            auto *hValidator = new QIntValidator(1, 32768, heightEdit);
            heightEdit->setValidator(hValidator);
        }

    public:
        QGridLayout *main_layout;
        QLineEdit *widthEdit=new QLineEdit("100");
        QLineEdit *heightEdit=new QLineEdit("100");
        // QLabel *display=new QLabel();
        QPushButton *colorEditButton=new QPushButton("Color");
        QComboBox *layoutCombo = new QComboBox();
        QSpinBox *rowsSpin = new QSpinBox();
        QSpinBox *colsSpin = new QSpinBox();
        QSpinBox *spacingSpin = new QSpinBox();
    private:
        QLabel *sizeWidthLabel=new QLabel("Width: ");
        QLabel *sizeHeightLabel=new QLabel("Height: ");
        QLabel *layoutLabel = new QLabel("Layout: ");
        QLabel *rowsLabel = new QLabel("Rows: ");
        QLabel *colsLabel = new QLabel("Cols: ");
        QLabel *spacingLabel = new QLabel("Spacing: ");


    };
}
