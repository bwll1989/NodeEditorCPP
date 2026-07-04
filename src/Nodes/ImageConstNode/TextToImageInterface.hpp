//
// Created by Administrator on 2023/12/13.
//
#pragma once

#include <QTextEdit>
#include <QFontComboBox>
#include <QComboBox>
#include <QCheckBox>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include <QPushButton>
#include <QVariantMap>
namespace Nodes
{
class TextToImageInterface final : public QWidget{
        Q_OBJECT
    public:
        /**
         * @brief 构造函数：创建并布局文本渲染参数控件
         * @param parent 父控件
         */
        explicit TextToImageInterface(QWidget *parent = nullptr){
            main_layout=new QGridLayout();
            main_layout->setContentsMargins(6, 6, 6, 6);
            main_layout->setSpacing(4);

            display->setMinimumHeight(90);
            display->setAlignment(Qt::AlignCenter);
            display->setScaledContents(false);

            widthEdit->setRange(1, 32768);
            widthEdit->setValue(512);
            heightEdit->setRange(1, 32768);
            heightEdit->setValue(256);

            fontSizeSpin->setRange(1, 300);
            fontSizeSpin->setValue(48);

            paddingSpin->setRange(0, 200);
            paddingSpin->setValue(10);

            alignHCombo->addItems({"Left","Center","Right"});
            alignHCombo->setCurrentIndex(0);

            alignVCombo->addItems({"Top","Center","Bottom"});
            alignVCombo->setCurrentIndex(0);

            wrapCheck->setChecked(true);
            antialiasCheck->setChecked(true);

            boldCheck->setText("Bold");
            italicCheck->setText("Italic");
            underlineCheck->setText("Underline");
            wrapCheck->setText("Wrap");
            antialiasCheck->setText("AA");

            textEdit->setPlaceholderText("输入文本，支持换行");
            textEdit->setPlainText("Hello\nText TOP");

            main_layout->addWidget(display,0,0,1,4);

            main_layout->addWidget(sizeWidthLabel,1,0,1,1);
            main_layout->addWidget(widthEdit,1,1,1,1);
            main_layout->addWidget(sizeHeightLabel,1,2,1,1);
            main_layout->addWidget(heightEdit,1,3,1,1);

            main_layout->addWidget(fontLabel,2,0,1,1);
            main_layout->addWidget(fontCombo,2,1,1,3);
            main_layout->addWidget(fontSizeLabel,3,0,1,1);
            main_layout->addWidget(fontSizeSpin,3,1,1,3);

            main_layout->addWidget(styleLabel,4,0,1,1);
            main_layout->addWidget(boldCheck,4,1,1,1);
            main_layout->addWidget(italicCheck,4,2,1,1);
            main_layout->addWidget(underlineCheck,4,3,1,1);

            main_layout->addWidget(alignLabel,5,0,1,1);
            main_layout->addWidget(alignHCombo,5,1,1,1);
            main_layout->addWidget(alignVLabel,5,2,1,1);
            main_layout->addWidget(alignVCombo,5,3,1,1);

            main_layout->addWidget(paddingLabel,6,0,1,1);
            main_layout->addWidget(paddingSpin,6,1,1,3);

            main_layout->addWidget(textColorButton,7,0,1,2);
            main_layout->addWidget(bgColorButton,7,2,1,2);

            main_layout->addWidget(wrapCheck,8,0,1,1);
            main_layout->addWidget(antialiasCheck,8,1,1,1);

            main_layout->addWidget(textLabel,9,0,1,1);
            main_layout->addWidget(textEdit,10,0,4,4);

            this->setLayout(main_layout);

      
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }

    public:
        QGridLayout *main_layout;
        IntDragValueWidget *widthEdit = new IntDragValueWidget();
        IntDragValueWidget *heightEdit = new IntDragValueWidget();
        QLabel *display=new QLabel();
        QTextEdit *textEdit = new QTextEdit();

        QFontComboBox *fontCombo = new QFontComboBox();
        IntDragValueWidget *fontSizeSpin = new IntDragValueWidget();

        QCheckBox *boldCheck = new QCheckBox();
        QCheckBox *italicCheck = new QCheckBox();
        QCheckBox *underlineCheck = new QCheckBox();
        QCheckBox *wrapCheck = new QCheckBox();
        QCheckBox *antialiasCheck = new QCheckBox();

        QComboBox *alignHCombo = new QComboBox();
        QComboBox *alignVCombo = new QComboBox();
        IntDragValueWidget *paddingSpin = new IntDragValueWidget();

        QPushButton *textColorButton = new QPushButton("Text Color");
        QPushButton *bgColorButton = new QPushButton("Background Color");
    private:
        QLabel *sizeWidthLabel=new QLabel("Width: ");
        QLabel *sizeHeightLabel=new QLabel("Height: ");
        QLabel *textLabel = new QLabel("Text:");
        QLabel *fontLabel = new QLabel("Font:");
        QLabel *fontSizeLabel = new QLabel("Size:");
        QLabel *styleLabel = new QLabel("Style:");
        QLabel *alignLabel = new QLabel("H Align:");
        QLabel *alignVLabel = new QLabel("V Align:");
        QLabel *paddingLabel = new QLabel("Padding:");
    };
}
