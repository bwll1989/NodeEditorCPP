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
    class SpoutInInterface: public QGroupBox
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit SpoutInInterface(QWidget *parent = nullptr)
            : QGroupBox("Spout发送器",parent)
            , m_isReceiving(false)
        {
            // 创建界面组件
            initInterface();
            setupConnections();
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
            m_senderComboBox->blockSignals(true);
            const QString currentSelection = m_senderComboBox->currentText();

            m_senderComboBox->clear();
            if (senders.isEmpty()) {
                m_senderComboBox->addItem("无可用发送器");
                m_senderComboBox->setEnabled(false);
            } else {
                m_senderComboBox->addItems(senders);
                m_senderComboBox->setEnabled(true);

                const int index = m_senderComboBox->findText(currentSelection);
                if (index >= 0) {
                    m_senderComboBox->setCurrentIndex(index);
                } else {
                    m_senderComboBox->setCurrentIndex(0);
                }
            }
            m_senderComboBox->blockSignals(false);

            if (m_senderComboBox->isEnabled()) {
                emit senderSelected(m_senderComboBox->currentText());
            }
        }

    private slots:
        /**
         * @brief 处理开始/停止按钮点击
         */
        void onStartStopClicked(bool checked) {
            if (!checked) {
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
         * @brief 创建发送器选择组
         */
        void initInterface() {
            m_senderComboBox = new QComboBox();
            m_senderComboBox->addItem("无可用发送器");
            m_senderComboBox->setEditable(true);
            m_senderComboBox->setInsertPolicy(QComboBox::NoInsert);
            m_senderComboBox->addItem("请输入或选择发送器名称");
            m_refreshButton = new QPushButton("刷新");
            m_refreshButton->setMaximumWidth(50);
            m_startStopButton = new QPushButton("开始");
            m_startStopButton->setCheckable(true);
            m_connectionStatusLabel = new QLabel("状态: 未连接");

            m_mainLayout = new QVBoxLayout(this);
            m_mainLayout->setContentsMargins(5, 5, 5, 5);
            m_mainLayout->setSpacing(6);

            auto* senderRow = new QWidget(this);
            auto* senderRowLayout = new QHBoxLayout(senderRow);
            senderRowLayout->setContentsMargins(0, 0, 0, 0);
            senderRowLayout->setSpacing(8);
            senderRowLayout->addWidget(m_senderComboBox, 1);
            senderRowLayout->addWidget(m_refreshButton, 0);

            m_mainLayout->addWidget(senderRow);
            m_mainLayout->addWidget(m_startStopButton);
            m_mainLayout->addWidget(m_connectionStatusLabel);
            m_mainLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }

        /**
         * @brief 设置信号连接
         */
        void setupConnections() {
            connect(m_startStopButton, &QPushButton::toggled, this, &SpoutInInterface::onStartStopClicked);
            connect(m_refreshButton, &QPushButton::clicked, this, &SpoutInInterface::onRefreshClicked);
            connect(m_senderComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &SpoutInInterface::onSenderSelectionChanged);
        }

    private:
        // 主布局
        QVBoxLayout *m_mainLayout;

        QPushButton *m_refreshButton;

        QLabel *m_connectionStatusLabel;
        
        // 状态变量
        bool m_isReceiving;
    public:

        // 发送器选择组
        QComboBox *m_senderComboBox;
        QPushButton *m_startStopButton;

    };
}
