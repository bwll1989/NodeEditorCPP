#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>

/**
 * @brief CustomScript插件界面
 * 提供插件选择和控件布局功能
 */
class CustomScriptInterface : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     */
    explicit CustomScriptInterface(QWidget *parent = nullptr);
    

    
    /**
     * @brief 添加控件到布局
     */
    void addWidgetToLayout(QWidget* widget, int row, int col, int rowSpan = 1, int colSpan = 1);
    
    /**
     * @brief 清除所有控件
     */
    void clearAllWidgets();

public:
    // 只保留脚本执行相关的UI
    QGridLayout* controlLayout;
    
private:
    QVBoxLayout* mainLayout;        // 主布局
   
};

