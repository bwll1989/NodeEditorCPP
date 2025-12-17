#pragma once
#include <QLineEdit>
#include <QLabel>
#include <QWidget>
#include <QLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QVariantMap>
#include <QToolTip>

/**
 * ArtnetOutInterface - Artnet发送节点的用户界面（简化版）
 * 只保留目标主机设置和状态显示
 */
class ArtnetOutInterface: public QGroupBox {
    Q_OBJECT
    
public:
    /**
     * 构造函数 - 初始化界面组件
     * @param parent 父控件
     */
    explicit ArtnetOutInterface(QWidget *parent = nullptr) {
        this->setTitle("Artnet发送器");
        
        // 设置提示信息
        targetHostEdit->setToolTip("输入目标主机IP地址，如：192.168.1.100 或 192.168.1.255（广播）");
        targetHostEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$")));
        // 设置默认值
        targetHostEdit->setText("192.168.1.255");
        statusLabel->setText("就绪");
        statusLabel->setStyleSheet("color: green;");
        
        // 连接信号槽
        connect(targetHostEdit, &QLineEdit::textChanged, this, &ArtnetOutInterface::onTargetHostTextChanged);
        
        // 布局设置
        main_layout->addWidget(targetHostLabel, 0, 0);
        main_layout->addWidget(targetHostEdit, 0, 1);
        main_layout->addWidget(statusLabel, 1, 0, 1, 2);
        
        this->setLayout(main_layout);
    }
    
    /**
     * 设置目标主机地址
     * @param host 主机地址
     */
    void setTargetHost(const QString &host) {
        targetHostEdit->setText(host);
    }
    
    /**
     * 获取目标主机地址
     * @return 主机地址
     */
    QString getTargetHost() const {
        return targetHostEdit->text();
    }
    
    /**
     * 设置状态信息
     * @param message 状态消息
     * @param isSuccess 是否成功状态
     */
    void setStatus(const QString &message, bool isSuccess) {
        statusLabel->setText(message);
        if (isSuccess) {
            statusLabel->setStyleSheet("color: green;");
        } else {
            statusLabel->setStyleSheet("color: red;");
        }
    }

signals:
    /**
     * 目标主机变化信号
     * @param host 新的主机地址
     */
    void TargetHostChanged(const QString &host);

private slots:
    /**
     * 目标主机文本变化处理
     */
    void onTargetHostTextChanged() {
        QString host = targetHostEdit->text().trimmed();
        if (!host.isEmpty()) {
            emit TargetHostChanged(host);
        }
    }

public:
    QLineEdit* targetHostEdit = new QLineEdit();
    QLabel* statusLabel = new QLabel();

private:
    QGridLayout *main_layout = new QGridLayout(this);
    QLabel *targetHostLabel = new QLabel("目标主机: ");
};

