//
// Created by Administrator on 2023/12/13.
//

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QWidget>
#include <QtWidgets/QLabel>


using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class MqttInterface: public QWidget
    {
    public:
        explicit MqttInterface(QWidget *parent = nullptr)
        {
            hostEdit->setText("127.0.0.1");
            portSpinBox->setRange(0, 65535);
            portSpinBox->setValue(1883);
            passwordEdit->setEchoMode(QLineEdit::Password);
            qosCombo->addItem("QoS 0", 0);
            qosCombo->addItem("QoS 1", 1);
            qosCombo->addItem("QoS 2", 2);
            statusButton->setFlat(true);
            statusButton->setCheckable(true);
            statusButton->setEnabled(false);
            statusButton->setText("Disconnect");
            statusButton->setStyleSheet("color: red; font-weight: bold;");

            main_layout->addWidget(hostLabel, 0, 0);
            main_layout->addWidget(hostEdit, 0, 1);
            main_layout->addWidget(portLabel, 1, 0);
            main_layout->addWidget(portSpinBox, 1, 1);
            main_layout->addWidget(userLabel, 2, 0);
            main_layout->addWidget(usernameEdit, 2, 1);
            main_layout->addWidget(passLabel, 3, 0);
            main_layout->addWidget(passwordEdit, 3, 1);
            main_layout->addWidget(topicLabel, 4, 0);
            main_layout->addWidget(topicEdit, 4, 1);
            main_layout->addWidget(payloadLabel, 5, 0);
            main_layout->addWidget(payloadEdit, 5, 1);
            main_layout->addWidget(qosCombo, 6, 0, 1, 2);
            main_layout->addWidget(statusButton, 7, 0, 1, 2);
            main_layout->addWidget(publishButton, 8, 0, 1, 2);
            main_layout->setContentsMargins(4, 2, 4, 4);
            this->setLayout(main_layout);
        }
        
    public:
        QGridLayout *main_layout = new QGridLayout(this);
        QLineEdit *hostEdit = new QLineEdit(this);
        QSpinBox *portSpinBox = new QSpinBox(this);
        QLineEdit *usernameEdit = new QLineEdit(this);
        QLineEdit *passwordEdit = new QLineEdit(this);
        QLineEdit *topicEdit = new QLineEdit(this);
        QLineEdit *payloadEdit = new QLineEdit(this);
        QComboBox *qosCombo = new QComboBox(this);
        QPushButton *publishButton = new QPushButton("Publish", this);
        QPushButton *statusButton = new QPushButton("Disconnect", this);
    private:
        QLabel *hostLabel = new QLabel("Host", this);
        QLabel *portLabel = new QLabel("Port", this);
        QLabel *userLabel = new QLabel("Username", this);
        QLabel *passLabel = new QLabel("Password", this);
        QLabel *topicLabel = new QLabel("Topic", this);
        QLabel *payloadLabel = new QLabel("Payload", this);
    };
}
