//
// Created by Administrator on 2023/12/13.
//

#ifndef JAVASCRIPTINTERFACE_HPP
#define JAVASCRIPTINTERFACE_HPP

#include "QWidget"
#include "QVBoxLayout"
#include "QGridLayout"
#include "QSplitter"
#include "JsCodeEditor.h"
#include "QToolButton"
#include "QIcon"
#include "QStyle"
using namespace std;
namespace Nodes
{
    class JavaScriptInterface: public QWidget{
        Q_OBJECT
        public:
        explicit JavaScriptInterface(QWidget *parent = nullptr){
            this->setLayout(new QVBoxLayout(this));
            this->layout()->setContentsMargins(0,0,0,0);
            
            // 创建主内容区域
            top_Widget->setLayout(top_layout);
            top_layout->setContentsMargins(0,0,0,0);
            
            // 创建底部布局，用于放置右下角的编辑按钮
            QHBoxLayout *bottomLayout = new QHBoxLayout();
            bottomLayout->setContentsMargins(0,0,5,5);
            bottomLayout->setSpacing(0);
            
            // 创建编辑图标按钮（更小尺寸）
            editButton = new QToolButton();
            editButton->setIcon(QIcon(":/icons/icons/js.png"));
            editButton->setToolTip("编辑脚本");
            editButton->setFixedSize(18, 18); // 减小尺寸到18x18

            
            // 将按钮放置在右下角
            bottomLayout->addStretch(); // 添加弹性空间，使按钮保持在右侧
            bottomLayout->addWidget(editButton);
            
            // 创建底部容器
            QWidget *bottomWidget = new QWidget();
            bottomWidget->setLayout(bottomLayout);
            bottomWidget->setFixedHeight(18); // 减小高度
            
            // 添加到主布局
            this->layout()->addWidget(top_Widget);
            this->layout()->addWidget(bottomWidget);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            // 连接信号和槽
            connect(editButton, &QToolButton::clicked, this, &JavaScriptInterface::toggleEditorMode);
        }

        /**
         * @brief 切换编辑器显示模式（嵌入/独立窗口）
         * 将编辑器显示为独立窗口，并确保跟随主程序退出
         */
        void toggleEditorMode() {
            // 移除父子关系，使其成为独立窗口
            codeWidget->setParent(nullptr);
            
            // 设置为独立窗口
            codeWidget->setWindowTitle("JS脚本编辑器");
            
            // 设置窗口图标
            codeWidget->setWindowIcon(QIcon(":/icons/icons/js.png"));
            
            // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
            codeWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
            
            // 设置窗口属性：当关闭时自动删除
            codeWidget->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
            codeWidget->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序
            
            // 设置窗口大小和显示
            codeWidget->resize(800, 600);
            codeWidget->show();
            codeWidget->setReadOnly(false);

            
            // 激活窗口并置于前台
            codeWidget->activateWindow();
            codeWidget->raise();
        }

        /**
         * @brief 析构函数，确保在销毁时强制关闭独立窗口
         */
        ~JavaScriptInterface() {
            // 强制关闭编辑器窗口
            if (codeWidget && codeWidget->isVisible()) {
                codeWidget->close();
            }
            // 如果窗口仍然存在，强制隐藏
            if (codeWidget) {
                codeWidget->hide();
                codeWidget->setParent(this); // 重新设置父子关系，确保随对象销毁
            }
        }

    public:
        QGridLayout *top_layout = new QGridLayout();
        QWidget *top_Widget = new QWidget();
        QToolButton *editButton; // 编辑按钮
        JsCodeEditor *codeWidget = new JsCodeEditor("",this);
    };
}
#endif // JAVASCRIPTINTERFACE_HPP

