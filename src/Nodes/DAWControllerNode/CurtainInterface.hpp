//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QGroupBox"
#include "QVBoxLayout"
#include "QHBoxLayout"
#include "QGridLayout"
#include "QDateTime"
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
using namespace std;

namespace Nodes
{
    /**
     * @brief FT_Curtain接口界面类
     * 
     * 提供电动幕帘系统的用户界面，包括：
     * - 连接设置（IP地址、端口）
     * - 连接状态显示
     * - 控制按钮（开、关、复位）
     */
    class CurtainInterface: public QGroupBox{
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit CurtainInterface(QWidget *parent = nullptr){
            // 连接设置组
            createConnectionGroup();
            hostEdit->setText("192.168.0.10");
            // 设置默认值
            portSpinBox->setRange(100, 65535);
            portSpinBox->setValue(10001); // 假设默认端口
            setMinimumSize(200, 150);
        }

    public:
        QLineEdit* hostEdit = new QLineEdit("127.0.0.1");
        /**
         * @brief 更新连接状态显示
         * @param connected 连接状态
         */
        void updateConnectionStatus(bool connected) {
            connectionStatusLabel->setText(connected?"状态: 已连接":"状态: 未连接");
            connectionStatusLabel->setStyleSheet(connected?"color: green; font-weight: bold;":"color: red; font-weight: bold;");
            connectionStatusLabel->setChecked(connected);
        }
        
        /**
        * @brief 创建连接设置组
        */
        void createConnectionGroup()
        {
            QGridLayout *connectionLayout = new QGridLayout(this);
            connectionLayout->addWidget(hostLabel, 0, 0, 1, 1);
            connectionLayout->addWidget(hostEdit, 0, 1, 1, 1);
            connectionLayout->addWidget(portLabel, 1, 0, 1, 1);
            connectionLayout->addWidget(portSpinBox, 1, 1, 1, 1);
            connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
            connectionStatusLabel->setFlat(true);
            connectionStatusLabel->setCheckable(true);
            connectionStatusLabel->setEnabled(false);
            connectionLayout->addWidget(connectionStatusLabel,2,0,1,2);
            connectionLayout->addWidget(openButton, 3, 0, 1, 1);
            connectionLayout->addWidget(closeButton, 3, 1, 1, 1);
            connectionLayout->addWidget(resetButton, 4, 0, 1, 2);
        }

    private:
        QLabel *hostLabel = new QLabel("主机地址:");
        QLabel *portLabel = new QLabel("端口:");
    public:
        QPushButton* openButton = new QPushButton("Open");
        QPushButton* closeButton = new QPushButton("Close");
        QPushButton* resetButton = new QPushButton("Reset");
        IntDragValueWidget* portSpinBox = new IntDragValueWidget();
        QPushButton *connectionStatusLabel = new QPushButton("Disconnect");
    };
}
