//
// Created by Administrator on 2023/12/13.
//

#include <QCheckBox>
#include <QDoubleSpinBox>

#include <QComboBox>
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include <QFileDialog>  // 新增文件对话框支持
#include <QScrollArea>
#include <QVBoxLayout>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
///
namespace Nodes
{
    class FmodDecoderInterface: public QWidget{
    public:
        /*构造
         *
         */
        explicit FmodDecoderInterface(QWidget *parent = nullptr) {
            main_layout = new QGridLayout(this);  // 正确初始化网格布局


            main_layout->addWidget(fileSelectComboBox, 0, 0, 1, 2);
            main_layout->addWidget(selectButton, 0, 2, 1, 1);
            
            // Scroll Area for buttons
            scrollArea = new QScrollArea();
            scrollArea->setWidgetResizable(true);
            scrollWidget = new QWidget();
            buttonLayout = new QVBoxLayout(scrollWidget);
            buttonLayout->setAlignment(Qt::AlignTop);
            buttonLayout->setContentsMargins(0,0,0,0);
            scrollArea->setWidget(scrollWidget);

            main_layout->addWidget(scrollArea, 1, 0, 1, 3);


            main_layout->setContentsMargins(4,2,4,4);


            this->setLayout(main_layout);
            this->setMinimumSize(QSize(250, 300));

        }

    public:
        // 修正后的成员变量
        QGridLayout *main_layout;  // 统一使用网格布局
        QLineEdit *fileSelectComboBox = new QLineEdit();
        QPushButton *selectButton=new QPushButton("Select");
        
        QScrollArea *scrollArea;
        QWidget *scrollWidget;
        QVBoxLayout *buttonLayout;
    };
}


