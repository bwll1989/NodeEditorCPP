//
// Created by Administrator on 2023/12/13.
//

#ifndef LUASCRIPTINTERFACE_HPP
#define LUASCRIPTINTERFACE_HPP

#include "QWidget"
#include "QLabel"
#include "QHBoxLayout"
#include "QPushButton"
#include "QVBoxLayout"

#include "CodeEditor.h"
using namespace std;
class LuaScriptInterface: public QFrame{
Q_OBJECT
public:
    explicit LuaScriptInterface(QWidget *parent = nullptr){
        this->setLayout(main_layout);
        main_layout->setContentsMargins(0,0,0,0);
        // 创建主布局
        main_layout->addWidget(codeWidget);
        // 连接信号和槽
        connect(codeWidget->detach, &QPushButton::clicked, this, &LuaScriptInterface::toggleEditorMode);
    }
    
    /**
     * @brief 切换编辑器显示模式（嵌入/独立窗口）
     */
    void toggleEditorMode() {
        if (isDetached) {
            // 如果当前是独立窗口，则切换回嵌入模式
            codeWidget->setParent(this);
            
            // 清除现有布局中的所有控件
            while (main_layout->count() > 0) {
                QLayoutItem* item = main_layout->takeAt(0);
                if (item->widget()) {
                    item->widget()->hide();
                }
                delete item;
            }
            
            // 重新添加编辑器到布局
            main_layout->addWidget(codeWidget);
            
            // 确保编辑器可见
            codeWidget->show();
            codeWidget->detach->setText("编辑");
            codeWidget->setReadOnly(true);
            isDetached = false;
            
            // 更新布局
            updateGeometry();
            update();
        } else {
            // 如果当前是嵌入模式，则切换到独立窗口
            codeWidget->setParent(nullptr);
            codeWidget->setWindowTitle("Lua脚本编辑器");
            codeWidget->setWindowFlags(Qt::Window);
            codeWidget->resize(800, 600); // 设置独立窗口的初始大小
            codeWidget->show();
            codeWidget->detach->setText("关闭编辑");
            codeWidget->setReadOnly(false);
            isDetached = true;
             // 更新布局
             updateGeometry();
        }
    }
    
    /**
     * @brief 析构函数，确保在销毁时关闭独立窗口
     */
    ~LuaScriptInterface() {
        if (isDetached) {
            codeWidget->close();
        }
    }
    
public:
    QHBoxLayout *main_layout = new QHBoxLayout(this);
    CodeEditor *codeWidget = new CodeEditor(this);
    bool isDetached = false; // 标记编辑器是否处于独立窗口模式
};

#endif // LUASCRIPTINTERFACE_HPP

