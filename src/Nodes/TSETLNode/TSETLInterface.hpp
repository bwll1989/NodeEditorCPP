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
     * @brief TSETL接口界面类
     * 
     * 提供TSETL触发事件收集系统的用户界面，包括：
     * - 连接设置（IP地址、端口）
     * - 连接状态显示
     * - 最新信号显示
     * - 消息统计信息
     */
    class TSETLInterface: public QWidget{
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit TSETLInterface(QWidget *parent = nullptr){
            // 创建主布局
            main_layout = new QVBoxLayout(this);
            main_layout->setContentsMargins(5, 5, 5, 5);
            main_layout->setSpacing(5);
            
            // 连接设置组
            createConnectionGroup();
            
            // 信号信息组
            createSignalGroup();
            
            // 设置默认值
            portSpinBox->setRange(1, 65535);
            portSpinBox->setValue(2001);
            hostEdit->setText("127.0.0.1");
            
            this->setLayout(main_layout);
            setMinimumSize(200, 150);

        }

    public:
        QLineEdit* hostEdit = new QLineEdit("127.0.0.1");
        QSpinBox* portSpinBox = new QSpinBox();
        
        /**
         * @brief 更新连接状态显示
         * @param connected 连接状态
         */
        void updateConnectionStatus(bool connected) {
            if (connected) {
                connectionStatusLabel->setText("状态: 已连接");
                connectionStatusLabel->setStyleSheet("color: green; font-weight: bold;");
            } else {
                connectionStatusLabel->setText("状态: 未连接");
                connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
                // 清空信号信息
                lastSignalLabel->setText("最新信号: 无");
                lastTimeLabel->setText("时间: --");
                signalCountLabel->setText("信号计数: 0");
            }
        }
        
        /**
         * @brief 更新最新信号信息
         * @param signalId 信号ID
         * @param dateTime 时间戳
         */
        void updateLastSignal(const QString &signalId, const QString &dateTime) {
            lastSignalLabel->setText(QString("最新信号: %1").arg(signalId));
            lastTimeLabel->setText(QString("时间: %1").arg(dateTime));
            
            // 更新计数器
            signalCount++;
            messageCount++;
            signalCountLabel->setText(QString("信号计数: %1").arg(signalCount));

        }
        


    private:
        QVBoxLayout *main_layout;
        
        // 连接设置相关
        QGroupBox *connectionGroup;
        QLabel *hostLabel = new QLabel("主机地址:");
        QLabel *portLabel = new QLabel("端口:");
        
        // 状态显示相关
        QGroupBox *statusGroup;
        QLabel *connectionStatusLabel = new QLabel("状态: 未连接");
        
        // 信号信息相关
        QGroupBox *signalGroup;
        QLabel *lastSignalLabel = new QLabel("最新信号: 无");
        QLabel *lastTimeLabel = new QLabel("时间: --");
        QLabel *signalCountLabel = new QLabel("信号计数: 0");
        
        // 计数器
        int messageCount = 0;
        int signalCount = 0;
        int heartbeatCount = 0;
        
        /**
         * @brief 创建连接设置组
         */
        void createConnectionGroup()
        {
            connectionGroup = new QGroupBox("连接设置", this);
            QGridLayout *connectionLayout = new QGridLayout(connectionGroup);
            
            connectionLayout->addWidget(hostLabel, 0, 0, 1, 1);
            connectionLayout->addWidget(hostEdit, 0, 1, 1, 1);
            connectionLayout->addWidget(portLabel, 1, 0, 1, 1);
            connectionLayout->addWidget(portSpinBox, 1, 1, 1, 1);
            
            main_layout->addWidget(connectionGroup);
        }
        
        /**
         * @brief 创建信号信息组
         */
        void createSignalGroup() {
            signalGroup = new QGroupBox("信号信息", this);
            QVBoxLayout *signalLayout = new QVBoxLayout(signalGroup);
            
            // 设置标签样式
            lastSignalLabel->setStyleSheet("font-weight: bold; color: orange;");
            lastTimeLabel->setStyleSheet("font-size: 10px; color: orange;");

            signalCountLabel->setStyleSheet("font-size: 10px; color: orange;");
            connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
            signalLayout->addWidget(connectionStatusLabel);
            signalLayout->addWidget(lastSignalLabel);
            signalLayout->addWidget(lastTimeLabel);
            signalLayout->addWidget(signalCountLabel);
            
            main_layout->addWidget(signalGroup);
        }
    };
}
