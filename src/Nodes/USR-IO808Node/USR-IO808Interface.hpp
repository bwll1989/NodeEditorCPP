#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QHBoxLayout>

/**
 * @brief Modbus主机界面类
 * 
 * 提供Modbus TCP主机的用户界面，包括：
 * - 连接设置（IP地址、端口、服务器ID）
 * - DI状态显示（8个离散输入）
 * - DO控制（8个数字输出）
 * - Read All按钮
 */
class USR_IO808Interface : public QWidget
{
    Q_OBJECT

public:
    USR_IO808Interface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        // 创建主布局
        auto layout = new QGridLayout(this);
        layout->setContentsMargins(5, 5, 5, 5);
        layout->setSpacing(5);
        
        // 创建连接设置组
        auto connectionGroup = new QGroupBox("连接设置", this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        
        // IP地址设置
        connectionLayout->addWidget(new QLabel("IP地址:"), 0, 0, 1, 1);
        _hostEdit = new QLineEdit("127.0.0.1", this);
        connectionLayout->addWidget(_hostEdit, 0, 1, 1, 1);
        
        // 端口设置
        connectionLayout->addWidget(new QLabel("端口:"), 1, 0, 1, 1);
        _portEdit = new QSpinBox(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(8080);  // Modbus TCP默认端口
        connectionLayout->addWidget(_portEdit, 1, 1, 1, 1);
        
        // Server ID设置
        connectionLayout->addWidget(new QLabel("ServerID(Useless):"), 2, 0, 1, 1);
        _serverId = new QSpinBox(this);
        _serverId->setRange(0, 255);
        _serverId->setValue(1);
        connectionLayout->addWidget(_serverId, 2, 1, 1, 1);
        
        layout->addWidget(connectionGroup, 0, 0, 1, 1);

        // 创建输入状态组（DI 0x0020~0x0027）
        auto inputGroup = new QGroupBox("DI 状态 (0x0020~0x0027)", this);
        auto inputLayout = new QGridLayout(inputGroup);

        for (int i = 0; i < 8; ++i) {
            _inputLabels[i] = new QLabel(QString("DI%1: 关").arg(i), this);
            _inputLabels[i]->setStyleSheet("color: gray; font-weight: bold;");
            _inputLabels[i]->setMinimumWidth(80);
            inputLayout->addWidget(_inputLabels[i], i / 4, i % 4);
        }

        layout->addWidget(inputGroup, 1, 0, 1, 1);

        // 创建输出控制组（DO 0x0000~0x0007）
        auto outputGroup = new QGroupBox("DO 控制 (0x0000~0x0007)", this);
        auto outputLayout = new QGridLayout(outputGroup);
        
        for (int i = 0; i < 8; ++i) {
            _outputCheckBoxes[i] = new QCheckBox(QString("DO%1").arg(i), this);
            _outputCheckBoxes[i]->setEnabled(false); // 初始禁用输出复选框
            outputLayout->addWidget(_outputCheckBoxes[i], i / 4, i % 4);
        }
        
        layout->addWidget(outputGroup, 2, 0, 1, 1);
        
        // Read All按钮
        _readAll = new QPushButton("Read All", this);
        _readAll->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
        _readAll->setEnabled(false); // 初始禁用
        layout->addWidget(_readAll, 3, 0, 1, 1);
        
        // 连接状态指示器
        _statusLabel = new QLabel("状态: 未连接", this);
        _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        layout->addWidget(_statusLabel, 4, 0, 1, 1);
        
        // 连接输出复选框的信号
        for (int i = 0; i < 8; ++i) {
            connect(_outputCheckBoxes[i], &QCheckBox::toggled, this, [this, i](bool checked) {
                emit outputChanged(i, checked);
            });
        }
        
        // 连接IP和端口编辑框的信号
        connect(_hostEdit, &QLineEdit::editingFinished, this, [this]() {
            emit hostChanged(_hostEdit->text());
        });
        
        connect(_portEdit, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
            emit portChanged(value);
        });
        
        // 设置大小策略
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setMinimumSize(250, 400);
        setMaximumSize(300, 500);
    }
    
    /**
     * @brief 设置输入状态
     * @param index 输入索引(0-7)
     * @param state 输入状态
     */
    void setInputState(int index, bool state) {
        if (index >= 0 && index < 8) {
            _inputLabels[index]->setText(QString("DI%1: %2").arg(index).arg(state ? "开" : "关"));
            _inputLabels[index]->setStyleSheet(state ? "color: green; font-weight: bold;" : "color: gray; font-weight: bold;");
        }
    }
    
    /**
     * @brief 设置输出状态
     * @param index 输出索引(0-7)
     * @param state 输出状态
     */
    void setOutputState(int index, bool state) {
        if (index >= 0 && index < 8) {
            _outputCheckBoxes[index]->blockSignals(true);
            _outputCheckBoxes[index]->setChecked(state);
            _outputCheckBoxes[index]->blockSignals(false);
        }
    }
    
    /**
     * @brief 获取输出状态
     * @param index 输出索引(0-7)
     * @return 输出状态
     */
    bool getOutputState(int index) const {
        if (index >= 0 && index < 8) {
            return _outputCheckBoxes[index]->isChecked();
        }
        return false;
    }
    
    /**
     * @brief 获取所有输出状态的位值
     * @return 8位输出状态值
     */
    int getAllOutputsValue() const {
        int value = 0;
        for (int i = 0; i < 8; ++i) {
            if (_outputCheckBoxes[i]->isChecked()) {
                value |= (1 << i);
            }
        }
        return value;
    }
    
    /**
     * @brief 设置连接状态
     * @param connected 连接状态
     */
    void setConnectionStatus(bool connected) {
        if (connected) {
            // 启用所有输出复选框和Read All按钮
            for (int i = 0; i < 8; ++i) {
                _outputCheckBoxes[i]->setEnabled(true);
            }
            _readAll->setEnabled(true);
            _statusLabel->setText("状态: 已连接");
            _statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            // 禁用所有输出复选框和Read All按钮
            for (int i = 0; i < 8; ++i) {
                _outputCheckBoxes[i]->setEnabled(false);
            }
            _readAll->setEnabled(false);
            _statusLabel->setText("状态: 未连接");
            _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }
    
    /**
     * @brief 获取主机地址
     * @return 主机地址
     */
    QString getHost() const {
        return _hostEdit->text();
    }
    
    /**
     * @brief 设置主机地址
     * @param host 主机地址
     */
    void setHost(const QString &host) {
        _hostEdit->setText(host);
    }
    
    /**
     * @brief 获取端口
     * @return 端口号
     */
    int getPort() const {
        return _portEdit->value();
    }
    
    /**
     * @brief 设置端口
     * @param port 端口号
     */
    void setPort(int port) {
        _portEdit->setValue(port);
    }
    
    /**
     * @brief 获取服务器ID
     * @return 服务器ID
     */
    int getServerId() const {
        return _serverId->value();
    }
    
    /**
     * @brief 设置服务器ID
     * @param id 服务器ID
     */
    void setServerId(int id) {
        _serverId->setValue(id);
    }

    // 公共成员，供外部访问
    QCheckBox *_outputCheckBoxes[8];
    QPushButton *_readAll;

signals:
    /**
     * @brief 主机地址改变信号
     * @param host 新的主机地址
     */
    void hostChanged(const QString &host);
    
    /**
     * @brief 端口改变信号
     * @param port 新的端口号
     */
    void portChanged(int port);
    
    /**
     * @brief 输出状态改变信号
     * @param index 输出索引
     * @param state 输出状态
     */
    void outputChanged(int index, bool state);
    
private:
    QLabel *_inputLabels[8];
    QLineEdit *_hostEdit;
    QSpinBox *_portEdit;
    QSpinBox *_serverId;
    QLabel *_statusLabel;
};