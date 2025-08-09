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

namespace Nodes
{
    /**
     * @brief Spout输入节点界面类
     * 
     * 提供Spout图像输入的用户界面，包括：
     * - 发送器选择和刷新
     * - 连接状态显示
     */
    class SpoutInInterface: public QWidget
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit SpoutInInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_isReceiving(false)
            , m_autoConnect(true)
        {
            // 创建界面组件
            createSenderGroup();
            createControlGroup();
            setupLayout();
            setupConnections();
            setupStyles();
            
            // 设置窗口属性
            setMinimumSize(250, 150);
            setMaximumWidth(300);
        }

    signals:
        /**
         * @brief 开始接收信号
         */
        void startReceiving();
        
        /**
         * @brief 停止接收信号
         */
        void stopReceiving();
        
        /**
         * @brief 选择发送器信号
         * @param senderName 发送器名称
         */
        void senderSelected(const QString& senderName);
        
        /**
         * @brief 刷新发送器列表信号
         */
        void refreshRequested();

    public slots:
        /**
         * @brief 更新连接状态显示
         * @param connected 连接状态
         */
        void updateConnectionStatus(bool connected) {
            if (connected) {
                m_connectionStatusLabel->setText("状态: 已连接");
                m_connectionStatusLabel->setStyleSheet("color: green; font-weight: bold;");
                m_startStopButton->setText("停止");
                m_isReceiving = true;
            } else {
                m_connectionStatusLabel->setText("状态: 未连接");
                m_connectionStatusLabel->setStyleSheet("color: red; font-weight: bold;");
                m_startStopButton->setText("开始");
                m_isReceiving = false;
            }
        }
        
        /**
         * @brief 更新发送器列表
         * @param senders 可用的发送器列表
         */
        void updateSenderList(const QStringList& senders) {
            QString currentSelection = m_senderComboBox->currentText();
            
            m_senderComboBox->clear();
            if (senders.isEmpty()) {
                m_senderComboBox->addItem("无可用发送器");
                m_senderComboBox->setEnabled(false);
            } else {
                m_senderComboBox->addItems(senders);
                m_senderComboBox->setEnabled(true);
                
                // 尝试恢复之前的选择
                int index = m_senderComboBox->findText(currentSelection);
                if (index >= 0) {
                    m_senderComboBox->setCurrentIndex(index);
                } else if (m_autoConnect && senders.size() > 0) {
                    // 自动连接第一个发送器
                    m_senderComboBox->setCurrentIndex(0);
                    emit senderSelected(senders.first());
                }
            }
        }

    private slots:
        /**
         * @brief 处理开始/停止按钮点击
         */
        void onStartStopClicked() {
            if (m_isReceiving) {
                emit stopReceiving();
            } else {
                emit startReceiving();
            }
        }
        
        /**
         * @brief 处理刷新按钮点击
         */
        void onRefreshClicked() {
            emit refreshRequested();
        }
        
        /**
         * @brief 处理发送器选择变化
         * @param index 选择的索引
         */
        void onSenderSelectionChanged(int index) {
            if (index >= 0 && m_senderComboBox->isEnabled()) {
                QString senderName = m_senderComboBox->itemText(index);
                if (senderName != "无可用发送器") {
                    emit senderSelected(senderName);
                }
            }
        }
        
        /**
         * @brief 处理自动连接复选框状态变化
         * @param checked 是否选中
         */
        void onAutoConnectChanged(bool checked) {
            m_autoConnect = checked;
        }

    private:
        // 主布局
        QVBoxLayout *m_mainLayout;
        
        // 发送器选择组
        QGroupBox *m_senderGroup;
        QComboBox *m_senderComboBox;
        QPushButton *m_refreshButton;
        QCheckBox *m_autoConnectCheckBox;
        
        // 控制组
        QGroupBox *m_controlGroup;
        QPushButton *m_startStopButton;
        QLabel *m_connectionStatusLabel;
        
        // 状态变量
        bool m_isReceiving;
        bool m_autoConnect;
        
        /**
         * @brief 创建发送器选择组
         */
        void createSenderGroup() {
            m_senderGroup = new QGroupBox("Spout发送器", this);
            QVBoxLayout *layout = new QVBoxLayout(m_senderGroup);
            
            // 发送器选择
            QHBoxLayout *senderLayout = new QHBoxLayout();
            m_senderComboBox = new QComboBox();
            m_senderComboBox->addItem("无可用发送器");
            m_senderComboBox->setEnabled(false);
            m_refreshButton = new QPushButton("刷新");
            m_refreshButton->setMaximumWidth(50);
            
            senderLayout->addWidget(m_senderComboBox);
            senderLayout->addWidget(m_refreshButton);
            
            // 自动连接选项
            m_autoConnectCheckBox = new QCheckBox("自动连接");
            m_autoConnectCheckBox->setChecked(true);
            
            layout->addLayout(senderLayout);
            layout->addWidget(m_autoConnectCheckBox);
        }
        
        /**
         * @brief 创建控制组
         */
        void createControlGroup() {
            m_controlGroup = new QGroupBox("连接控制", this);
            QVBoxLayout *layout = new QVBoxLayout(m_controlGroup);
            
            m_startStopButton = new QPushButton("开始");
            m_connectionStatusLabel = new QLabel("状态: 未连接");
            
            layout->addWidget(m_startStopButton);
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
            m_mainLayout->addWidget(m_controlGroup);
            m_mainLayout->addStretch();
        }
        
        /**
         * @brief 设置信号连接
         */
        void setupConnections() {
            connect(m_startStopButton, &QPushButton::clicked, this, &SpoutInInterface::onStartStopClicked);
            connect(m_refreshButton, &QPushButton::clicked, this, &SpoutInInterface::onRefreshClicked);
            connect(m_senderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
                    this, &SpoutInInterface::onSenderSelectionChanged);
            connect(m_autoConnectCheckBox, &QCheckBox::toggled, this, &SpoutInInterface::onAutoConnectChanged);
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