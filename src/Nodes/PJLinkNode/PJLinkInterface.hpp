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
#include <QSpacerItem>
#include "Common/GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include <QCoreApplication>
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
            // 连接设置组
            createConnectionGroup();
            hostEdit->setText("192.168.1.4");
            // 设置默认值
            portSpinBox->setRange(100, 65535);
            portSpinBox->setValue(4352);
            customCommandLineEdit->setText("LAMP ?");


            setMinimumSize(200, 150);

        }

    public:
        QLineEdit* hostEdit = new QLineEdit("127.0.0.1");
        /**
         * @brief 更新连接状态显示
         * @param connected 连接状态
         */
        void updateConnectionStatus(bool connected) {
            connectionStatusLabel->setText(connected ? QCoreApplication::translate("Nodes::PJLinkInterface", "状态: 已连接") : QCoreApplication::translate("Nodes::PJLinkInterface", "状态: 未连接"));
            connectionStatusLabel->setStyleSheet(connected?"color: green; font-weight: bold;":"color: red; font-weight: bold;");
            connectionStatusLabel->setChecked(connected);
        }
        
        /**
        * @brief 创建连接设置组
        */
        void createConnectionGroup()
        {

            QGridLayout *connectionLayout = new QGridLayout(this);
            connectionLayout->setContentsMargins(0, 0, 0, 0);
            connectionLayout->setSpacing(6);
            connectionLayout->setColumnStretch(0, 1);
            connectionLayout->setColumnStretch(1, 2);
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
            connectionLayout->addWidget(powerOnButton, 4, 0, 1, 2);
            connectionLayout->addWidget(powerOffButton, 5, 0, 1, 2);
            connectionLayout->addWidget(muteOnButton, 6, 0, 1, 2);
            connectionLayout->addWidget(muteOffButton, 7, 0, 1, 2);
            connectionLayout->addWidget(customCommandLineEdit, 8, 0, 1, 2);
            connectionLayout->addWidget(customCommandButton, 9, 0, 1, 2);
            connectionLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 10, 0, 1, 2);
            connectionLayout->setRowStretch(10, 1);

        }

    private:
        QLabel *hostLabel = new QLabel(QCoreApplication::translate("Nodes::PJLinkInterface", "主机地址:"));
        QLabel *portLabel = new QLabel(QCoreApplication::translate("Nodes::PJLinkInterface", "端口:"));
        QLabel *passwordLabel = new QLabel(QCoreApplication::translate("Nodes::PJLinkInterface", "密码："));
    public:
        QPushButton* powerOnButton = new QPushButton(QCoreApplication::translate("Nodes::PJLinkInterface", "开机"));
        QPushButton* powerOffButton = new QPushButton(QCoreApplication::translate("Nodes::PJLinkInterface", "关机"));
        QPushButton* muteOnButton = new QPushButton(QCoreApplication::translate("Nodes::PJLinkInterface", "静音开"));
        QPushButton* muteOffButton = new QPushButton(QCoreApplication::translate("Nodes::PJLinkInterface", "静音关"));
        QLineEdit *passwordLineEdit = new QLineEdit("");
        IntDragValueWidget* portSpinBox = new IntDragValueWidget();
        QLineEdit *customCommandLineEdit = new QLineEdit("");
        QPushButton* customCommandButton = new QPushButton(QCoreApplication::translate("Nodes::PJLinkInterface", "自定义命令"));
        QPushButton *connectionStatusLabel = new QPushButton(QCoreApplication::translate("Nodes::PJLinkInterface", "断开"));
    };
}
