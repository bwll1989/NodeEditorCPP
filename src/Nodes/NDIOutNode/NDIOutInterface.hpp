#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextBrowser>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDateTime>
#include <QCheckBox>
#include <QDoubleSpinBox>

namespace Nodes
{
    /**
     * @brief NDI输出节点界面类
     * 
     * 提供NDI图像输出的用户界面，包括：
     * - 发送器名称设置
     * - 视频参数配置
     * - 发送状态控制
     */
    class NDIOutInterface: public QWidget
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit NDIOutInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_isSending(false)
        {
            // 创建界面组件
            createSenderGroup();
            // createControlGroup();
            setupLayout();
            setupConnections();
            // 设置窗口属性
            setMinimumSize(200, 100);
        }

    signals:
        /**
         * @brief 开始发送信号
         */
        void startSending();
        
        /**
         * @brief 停止发送信号
         */
        void stopSending();
        
        /**
         * @brief 发送器名称变化信号
         * @param senderName 发送器名称
         */
        void senderNameChanged(const QString& senderName);

    public slots:
        /**
         * @brief 更新发送状态显示
         * @param sending 发送状态
         */
        void updateSendingStatus(bool sending) {
            if (sending) {
                m_statusLabel->setText("状态: 正在发送");
                m_statusLabel->setStyleSheet("color: green; font-weight: bold;");
                m_startStopButton->setText("停止");
                m_isSending = true;
            } else {
                m_statusLabel->setText("状态: 已停止");
                m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
                m_startStopButton->setText("开始");
                m_isSending = false;
            }
        }
        
        /**
         * @brief 显示错误信息
         * @param error 错误信息
         */
        void showError(const QString& error) {
            m_statusLabel->setText("错误: " + error);
            m_statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }

    private slots:
        /**
         * @brief 处理开始/停止按钮点击
         */
        void onStartStopClicked(bool state) {
            if (m_isSending&&!state) {
                emit stopSending();
            } else {
                emit startSending();
            }
        }
        
        /**
         * @brief 处理发送器名称变化
         */
        void onSenderNameChanged() {
            QString senderName = m_senderNameEdit->text().trimmed();
            if (!senderName.isEmpty()) {
                emit senderNameChanged(senderName);
            }
        }
        
        /**
         * @brief 创建发送器设置组
         */
        void createSenderGroup() {
        m_senderGroup = new QGroupBox("发送器设置", this);
        QVBoxLayout *layout = new QVBoxLayout(m_senderGroup);
        // 发送器名称
        QHBoxLayout *nameLayout = new QHBoxLayout();
        QLabel *nameLabel = new QLabel("名称:");
        m_senderNameEdit = new QLineEdit("NodeEditor NDI Output");
        nameLayout->addWidget(nameLabel);
        nameLayout->addWidget(m_senderNameEdit);
        m_startStopButton = new QPushButton("开始");
        m_startStopButton->setCheckable(true);
        m_statusLabel = new QLabel("状态: 已停止");

        layout->addLayout(nameLayout);
        layout->addWidget(m_startStopButton);
        layout->addWidget(m_statusLabel);
    }



        /**
         * @brief 设置布局
         */
        void setupLayout() {
        m_mainLayout = new QVBoxLayout(this);
        m_mainLayout->setContentsMargins(5, 5, 5, 5);
        m_mainLayout->setSpacing(5);
        m_mainLayout->addWidget(m_senderGroup);
    }

        /**
         * @brief 设置信号连接
         */
        void setupConnections() {
        connect(m_startStopButton, &QPushButton::toggled, this, &NDIOutInterface::onStartStopClicked);
        connect(m_senderNameEdit, &QLineEdit::editingFinished, this, &NDIOutInterface::onSenderNameChanged);

    }
    public:
        QPushButton *m_startStopButton;
        QLineEdit *m_senderNameEdit;
    private:
        // 主布局
        QVBoxLayout *m_mainLayout;
        // 发送器设置
        QGroupBox *m_senderGroup;
        QLabel *m_statusLabel;
        
        // 状态变量
        bool m_isSending;
        

        

    };
}