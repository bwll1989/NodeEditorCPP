//
// Created by Administrator on 2023/12/13.
//

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>

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
    class DAWControllerInterface: public QGroupBox{
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit DAWControllerInterface(QWidget *parent = nullptr){
            // 连接设置组
            createConnectionGroup();
            hostEdit->setText("127.0.0.1");

            customCommandLineEdit->setText("{\"type\": \"player\",\"id\": \"1\",\"operate\": \"play\"}/0");


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

            connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
            connectionStatusLabel->setFlat(true);
            connectionStatusLabel->setCheckable(true);
            connectionStatusLabel->setEnabled(false);
            connectionLayout->addWidget(connectionStatusLabel,1,0,1,2);
            connectionLayout->addWidget(customCommandLineEdit, 2, 0, 1, 2);
            connectionLayout->addWidget(customCommandButton, 3, 0, 1, 2);

        }

    private:
        QLabel *hostLabel = new QLabel("主机地址:");

    public:

        QLineEdit *customCommandLineEdit = new QLineEdit("");
        QPushButton* customCommandButton = new QPushButton("Send");
        QPushButton *connectionStatusLabel = new QPushButton("Disconnect");
    };
}
