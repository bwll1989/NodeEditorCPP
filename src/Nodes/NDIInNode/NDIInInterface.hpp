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
    class NDIInInterface: public QWidget
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit NDIInInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_isReceiving(false)
        {
            // 创建界面组件
            createSenderGroup();
            setupLayout();
            setupConnections();
            // 设置窗口属性
            setMinimumSize(200, 100);

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
            // 保存当前输入的文本（可能是用户手动输入的）
            QString currentText = m_senderComboBox->currentText();
            m_senderComboBox->blockSignals(true);
            // 清空并重新填充列表
            m_senderComboBox->clear();
            
            if (senders.isEmpty()) {
                // m_senderComboBox->addItem("无可用发送器");
            } else {
                // 添加发现的发送器
                m_senderComboBox->addItems(senders);
            }
            m_senderComboBox->blockSignals(false);
            // 恢复之前的文本（无论是否在列表中）
            m_senderComboBox->setCurrentText(currentText);
            emit m_senderComboBox->currentTextChanged(currentText);

        }

    private slots:
        /**
         * @brief 处理开始/停止按钮点击
         */
        void onStartStopClicked(bool state) {
            if (m_isReceiving&&!state) {
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
         * @brief 处理发送器文本变化
         * @param text 当前文本
         */
        void onSenderTextChanged(const QString& text) {
            if (!text.isEmpty() && text != "请输入或选择发送器名称") {
                emit senderSelected(text);
            }
        }

    public:
        QPushButton *m_startStopButton;
        QComboBox *m_senderComboBox;
        QPushButton *m_refreshButton;
    private:
        // 主布局
        QVBoxLayout *m_mainLayout;
        
        // 发送器选择组
        QGroupBox *m_senderGroup;

        QLabel *m_connectionStatusLabel;
        
        // 状态变量
        bool m_isReceiving;
        
        /**
         * @brief 创建发送器选择组
         */
        void createSenderGroup() {
            m_senderGroup = new QGroupBox("NDI发送器", this);
            QVBoxLayout *layout = new QVBoxLayout(m_senderGroup);
            
            // 发送器选择
            QHBoxLayout *senderLayout = new QHBoxLayout();
            m_senderComboBox = new QComboBox();
            
            // 设置为可编辑模式，允许用户手动输入
            m_senderComboBox->setEditable(true);
            m_senderComboBox->setInsertPolicy(QComboBox::NoInsert);
            m_senderComboBox->addItem("请输入或选择发送器名称");
            
            // 设置占位符文本
            m_senderComboBox->lineEdit()->setPlaceholderText("输入NDI发送器名称");
            
            m_refreshButton = new QPushButton("刷新");
            m_refreshButton->setMaximumWidth(50);
            
            senderLayout->addWidget(m_senderComboBox);
            senderLayout->addWidget(m_refreshButton);

            layout->addLayout(senderLayout);

            m_startStopButton = new QPushButton("开始");
            m_startStopButton->setCheckable(true);
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

        }
        
        /**
         * @brief 设置信号连接
         */
        void setupConnections() {
            connect(m_startStopButton, &QPushButton::toggled, this, &NDIInInterface::onStartStopClicked);
            connect(m_refreshButton, &QPushButton::clicked, this, &NDIInInterface::onRefreshClicked);
            // 监听文本变化而不是索引变化
            connect(m_senderComboBox, &QComboBox::currentTextChanged, this, &NDIInInterface::onSenderTextChanged);
        }


    };
}