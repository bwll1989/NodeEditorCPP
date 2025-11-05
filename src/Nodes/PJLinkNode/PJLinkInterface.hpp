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

using namespace std;

namespace Nodes
{
    /**
     * @brief PJLink接口界面类
     * 
     * 提供PJLink触发事件收集系统的用户界面，包括：
     * - 连接设置（IP地址、端口）
     * - 连接状态显示
     * - 最新信号显示
     * - 消息统计信息
     */
    class PJLinkInterface: public QGroupBox{
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit PJLinkInterface(QWidget *parent = nullptr){
            // 创建主布局
            main_layout = new QVBoxLayout(this);
            main_layout->setContentsMargins(5, 5, 5, 5);
            main_layout->setSpacing(5);
            
            // 连接设置组
            createConnectionGroup();
            hostEdit->setText("192.168.1.4");
            // 设置默认值
            portSpinBox->setRange(100, 65535);
            portSpinBox->setValue(4352);
            customCommandLineEdit->setText("LAMP ?");
            this->setLayout(main_layout);

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
            connectionLayout->addWidget(passwordLabel, 2, 0, 1, 1);
            connectionLayout->addWidget(passwordLineEdit, 2, 1, 1, 1);
            connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
            connectionStatusLabel->setFlat(true);
            connectionStatusLabel->setCheckable(true);
            connectionStatusLabel->setEnabled(false);
            connectionLayout->addWidget(connectionStatusLabel,3,0,1,2);
            connectionLayout->addWidget(powerOnButton, 4, 0, 1, 1);
            connectionLayout->addWidget(powerOffButton, 4, 1, 1, 1);
            connectionLayout->addWidget(muteOnButton, 5, 0, 1, 1);
            connectionLayout->addWidget(muteOffButton, 5, 1, 1, 1);
            connectionLayout->addWidget(customCommandLineEdit, 6, 0, 1, 2);
            connectionLayout->addWidget(customCommandButton, 7, 0, 1, 2);
            main_layout->addLayout(connectionLayout);
        }

    private:
        QVBoxLayout *main_layout;

        QLabel *hostLabel = new QLabel("主机地址:");
        QLabel *portLabel = new QLabel("端口:");
        QLabel *passwordLabel = new QLabel("Password:");
    public:
        QPushButton* powerOnButton = new QPushButton("PowerOn");
        QPushButton* powerOffButton = new QPushButton("PowerOff");
        QPushButton* muteOnButton = new QPushButton("MuteOn");
        QPushButton* muteOffButton = new QPushButton("MuteOff");
        QLineEdit *passwordLineEdit = new QLineEdit("");
        QSpinBox* portSpinBox = new QSpinBox();
        QLineEdit *customCommandLineEdit = new QLineEdit("");
        QPushButton* customCommandButton = new QPushButton("CustomCommand");
        QPushButton *connectionStatusLabel = new QPushButton("Disconnect");
    };
}
