#include "CustomScriptInterface.hpp"

/**
 * @brief 构造函数，初始化界面
 */
CustomScriptInterface::CustomScriptInterface(QWidget *parent)
    : QWidget(parent)
{
    // 创建主布局
    mainLayout = new QVBoxLayout(this);

    // 控件布局区域
    controlLayout = new QGridLayout();
    mainLayout->addLayout(controlLayout);
    mainLayout->addStretch();
    
    setLayout(mainLayout);
}

/**
 * @brief 添加控件到布局
 */
void CustomScriptInterface::addWidgetToLayout(QWidget* widget, int row, int col, int rowSpan, int colSpan)
{
    if (widget && controlLayout) {
        controlLayout->addWidget(widget, row, col, rowSpan, colSpan);
    }
}

/**
 * @brief 清除所有控件
 */
void CustomScriptInterface::clearAllWidgets()
{
    // 清除布局中的所有控件
    QLayoutItem* item;
    while ((item = controlLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}