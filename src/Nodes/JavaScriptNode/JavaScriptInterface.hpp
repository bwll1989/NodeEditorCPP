//
// Created by Administrator on 2023/12/13.
//

#ifndef JAVASCRIPTINTERFACE_HPP
#define JAVASCRIPTINTERFACE_HPP

#include "QWidget"
#include "QLabel"
#include "QHBoxLayout"
#include "QPushButton"
#include "QVBoxLayout"
#include "QGridLayout"
#include "QSplitter"
#include "JsCodeEditor.h"
#include "QSpacerItem"
using namespace std;
namespace Nodes
{
    class JavaScriptInterface: public QWidget{
        Q_OBJECT
        public:
        explicit JavaScriptInterface(QWidget *parent = nullptr){
            this->setLayout(new QVBoxLayout(this));
            this->layout()->setContentsMargins(0,0,0,0);
            buttom_layout->setContentsMargins(0,0,0,0);
            // 创建主布局
            buttom_Widget->setLayout(buttom_layout);
            buttom_layout->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Minimum,QSizePolicy::Expanding));
            buttom_layout->addWidget(run);
            buttom_layout->addWidget(detach);
            top_layout->setContentsMargins(0,0,0,0);
            top_Widget->setLayout(top_layout);
            // right_layout->addWidget(codeWidget);
            this->layout()->addWidget(top_Widget);
            this->layout()->addWidget(buttom_Widget);
            // 连接信号和槽
            connect(detach, &QPushButton::clicked, this, &JavaScriptInterface::toggleEditorMode);

        }

        /**
         * @brief 切换编辑器显示模式（嵌入/独立窗口）
         */
        void toggleEditorMode() {

            // 如果当前是嵌入模式，则切换到独立窗口
            codeWidget->setWindowTitle("JS脚本编辑器");
            codeWidget->setWindowFlags(Qt::Window);
            codeWidget->resize(800, 600); // 设置独立窗口的初始大小
            codeWidget->show();
            codeWidget->setReadOnly(false);
            // 更新布局
            updateGeometry();

        }

        /**
         * @brief 析构函数，确保在销毁时关闭独立窗口
         */
        ~JavaScriptInterface() {
            codeWidget->close();
        }

    public:
        QVBoxLayout *buttom_layout = new QVBoxLayout();
        QGridLayout *top_layout = new QGridLayout();
        QWidget *buttom_Widget = new QWidget();
        QWidget *top_Widget = new QWidget();
        QPushButton *run = new QPushButton("运行");
        QPushButton *detach = new QPushButton("编辑");
        JsCodeEditor *codeWidget = new JsCodeEditor("",this);


    };
}
#endif // JAVASCRIPTINTERFACE_HPP

