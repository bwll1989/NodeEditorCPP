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
#include <QProgressBar>
#include <algorithm>
#include "DelayListWidget/DelayMessageListWidget.hpp"
using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class DelayInterface: public QFrame{
    public:
        // 函数级注释：构建 Delay 节点界面，并在顶部显示“定时器进度条”（用于确认长延时仍在运行）。
        explicit DelayInterface(QWidget *parent = nullptr){

            progressBar = new QProgressBar(this);
            progressBar->setRange(0, 100);
            progressBar->setValue(0);
            progressBar->setTextVisible(true);
            progressBar->setFormat("空闲");

            main_layout->addWidget(progressBar,0,0,1,2);
            main_layout->addWidget(delay_message_list_widget,1,0,1,2);
            ignoreRepeatCheckBox->setText("忽略重复信号");
            ignoreRepeatCheckBox->setChecked(true); // 默认忽略重复信号
            ignoreRepeatCheckBox->setToolTip("选中：保持期间忽略新信号\n取消：立即停止当前保持并处理新信号");
            main_layout->addWidget(ignoreRepeatCheckBox, 2, 0,1,2); // 添加重复信号处理模式选择框
            main_layout->setContentsMargins(4,2,4,4);
            this->setLayout(main_layout);
        }

        // 函数级注释：设置进度（0-100），用于表示当前延时队列整体进度。
        void setProgressPercent(int percent)
        {
            if (!progressBar) {
                return;
            }
            progressBar->setFormat("%p%");
            progressBar->setValue(std::max(0, std::min(100, percent)));
        }

        // 函数级注释：设置为空闲状态显示。
        void setProgressIdle()
        {
            if (!progressBar) {
                return;
            }
            progressBar->setValue(0);
            progressBar->setFormat("空闲");
        }

    public:
        QGridLayout *main_layout=new QGridLayout(this);
        DelayMessageListWidget *delay_message_list_widget=new DelayMessageListWidget(this);
        QCheckBox *ignoreRepeatCheckBox = new QCheckBox(this); // 重复信号处理模式选择框
        QProgressBar *progressBar = nullptr;
    };
}