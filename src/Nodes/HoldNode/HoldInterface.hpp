//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QGridLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include "QCheckBox"

using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    /**
     * @brief 信号保持节点界面类
     * 提供保持时间设置界面和重复信号处理模式选择
     */
    class HoldInterface: public QFrame
    {
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit HoldInterface(QWidget *parent = nullptr)
        {
            // 创建标签
            auto timeLabel = new QLabel("Hold Time:");
            auto unitLabel = new QLabel("ms");
            auto modeLabel = new QLabel("重复信号处理:");
            
            // 设置输入框默认值
            value->setText("1000"); // 默认保持1秒
            value->setToolTip("信号保持时间（毫秒）");
            
            // 设置重复信号处理模式选择框
            ignoreRepeatCheckBox->setText("忽略重复信号");
            ignoreRepeatCheckBox->setChecked(true); // 默认忽略重复信号
            ignoreRepeatCheckBox->setToolTip("选中：保持期间忽略新信号\n取消：立即停止当前保持并处理新信号");
            
            // 布局设置
            main_layout->addWidget(timeLabel, 0, 0);
            main_layout->addWidget(value, 0, 1);
            main_layout->addWidget(unitLabel, 0, 2);
            
            main_layout->addWidget(modeLabel, 1, 0);
            main_layout->addWidget(ignoreRepeatCheckBox, 1, 1, 1, 2);
            main_layout->setContentsMargins(4, 2, 4, 4);
            this->setLayout(main_layout);
        }
        
    public:
        QGridLayout *main_layout = new QGridLayout(this);      // 主布局
        QLineEdit *value = new QLineEdit(this);               // 保持时间输入框
        QCheckBox *ignoreRepeatCheckBox = new QCheckBox(this); // 重复信号处理模式选择框
    };
}