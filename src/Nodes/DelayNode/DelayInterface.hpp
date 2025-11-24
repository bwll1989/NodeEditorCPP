//
// Created by Administrator on 2023/12/13.
//

#include <QCheckBox>

#include "QWidget"
#include "QLabel"
#include "QGridLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include "DelayListWidget/DelayMessageListWidget.hpp"
using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class DelayInterface: public QFrame{
    public:
        explicit DelayInterface(QWidget *parent = nullptr){

            auto label=new QLabel("ms");
            // la->setAlignment(Qt::AlignCenter );
            //        this->setStyleSheet("QFrame{background-color:transparent}");

            main_layout->addWidget(delay_message_list_widget,0,0,1,2);
            ignoreRepeatCheckBox->setText("忽略重复信号");
            ignoreRepeatCheckBox->setChecked(true); // 默认忽略重复信号
            ignoreRepeatCheckBox->setToolTip("选中：保持期间忽略新信号\n取消：立即停止当前保持并处理新信号");
            main_layout->addWidget(ignoreRepeatCheckBox, 1, 0,1,2); // 添加重复信号处理模式选择框
            main_layout->setContentsMargins(4,2,4,4);
            this->setLayout(main_layout);
        }
    public:
        QGridLayout *main_layout=new QGridLayout(this);
        DelayMessageListWidget *delay_message_list_widget=new DelayMessageListWidget(this);
        QCheckBox *ignoreRepeatCheckBox = new QCheckBox(this); // 重复信号处理模式选择框
    };
}