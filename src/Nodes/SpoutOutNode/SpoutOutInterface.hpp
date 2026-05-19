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
#include <QSpacerItem>

namespace Nodes
{
    /**
     * @brief Spout输入节点界面类
     * 
     * 提供Spout图像输入的用户界面，包括：
     * - 发送器选择和刷新
     * - 连接状态显示
     */
    class SpoutOutInterface: public QWidget
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit SpoutOutInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_isSending(false)
        {
            // 创建界面组件
            createSenderGroup();
            setupLayout();
            setupStyles();
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
         * @brief 发送器名称改变信号
         * @param senderName 发送器名称
         */
        void senderNameChanged(const QString& senderName);

    public slots:
        /**
         * @brief 更新连接状态显示
         * @param connected 连接状态
         */
        void updateConnectionStatus(bool connected) {
            if (connected) {
                m_connectionStatusLabel->setText("状态: 发送中");
                m_connectionStatusLabel->setStyleSheet("color: green; font-weight: bold;");
                m_startStopButton->setText("停止");
                m_isSending = true;
                m_senderNameEdit->setEnabled(false); // 发送时不可更改名称
            } else {
                m_connectionStatusLabel->setText("状态: 已停止");
                m_connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
                m_startStopButton->setText("开始");
                m_isSending = false;
                m_senderNameEdit->setEnabled(true);
            }
        }

    private slots:
        /**
         * @brief 处理开始/停止按钮点击
         */
        void onStartStopClicked() {
            if (m_isSending) {
                emit stopSending();
            } else {
                emit startSending();
            }
        }
        
        /**
         * @brief 处理发送器名称变化
         */
        void onSenderNameEditingFinished() {
            QString name = m_senderNameEdit->text().trimmed();
            if (!name.isEmpty() && name != m_currentSenderName) {
                m_currentSenderName = name;
                emit senderNameChanged(name);
            }
        }

    private:
        // 主布局
        QVBoxLayout *m_mainLayout;
        
        // 发送器设置组
        QGroupBox *m_senderGroup;
        QLineEdit *m_senderNameEdit;
        QPushButton *m_startStopButton;
        QLabel *m_connectionStatusLabel;
        
        // 状态变量
        bool m_isSending;
        QString m_currentSenderName;
        
        /**
         * @brief 创建发送器设置组
         */
        void createSenderGroup() {
            m_senderGroup = new QGroupBox("Spout发送设置", this);
            auto *layout = new QVBoxLayout(m_senderGroup);
            layout->setContentsMargins(6, 6, 6, 6);
            layout->setSpacing(6);

            auto *nameRow = new QWidget(m_senderGroup);
            auto *nameLayout = new QHBoxLayout(nameRow);
            nameLayout->setContentsMargins(0, 0, 0, 0);
            nameLayout->setSpacing(8);

            nameLayout->addWidget(new QLabel("发送器名称:", nameRow), 0);
            m_senderNameEdit = new QLineEdit("NodeEditor Spout", nameRow);
            m_currentSenderName = "NodeEditor Spout";
            nameLayout->addWidget(m_senderNameEdit, 1);
            layout->addWidget(nameRow);
            
            connect(m_senderNameEdit, &QLineEdit::editingFinished, 
                    this, &SpoutOutInterface::onSenderNameEditingFinished);

            // 开始/停止按钮
            m_startStopButton = new QPushButton("开始");
            connect(m_startStopButton, &QPushButton::clicked,
                    this, &SpoutOutInterface::onStartStopClicked);
            layout->addWidget(m_startStopButton);
            m_connectionStatusLabel = new QLabel("状态: 未发送");
            m_connectionStatusLabel->setAlignment(Qt::AlignCenter);
            m_connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
            layout->addWidget(m_connectionStatusLabel);
        }

        
        /**
         * @brief 设置布局
         */
        void setupLayout() {
            m_mainLayout = new QVBoxLayout(this);
            m_mainLayout->setContentsMargins(5, 5, 5, 5);
            m_mainLayout->setSpacing(5);
            m_mainLayout->addWidget(m_senderGroup);
            m_mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }
        
        /**
         * @brief 设置样式
         */
        void setupStyles() {
            // 连接状态标签样式
            m_connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
            // 按钮样式
            m_startStopButton->setStyleSheet(
                "QPushButton { font-weight: bold; padding: 5px; }"
                "QPushButton:pressed { background-color: #ddd; }"
            );
        }
    };
}
