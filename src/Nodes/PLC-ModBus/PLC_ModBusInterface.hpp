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
#include <QComboBox>
#include <QStringList>
#include <QVector>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * @brief Modbus主机界面类
 * 
 * 提供Modbus TCP主机的用户界面，包括：
 * - 连接设置（IP地址、端口、服务器ID）
 * - DI状态显示（8个离散输入）
 * - DO控制（8个数字输出）
 * - Read All按钮
 */
class PLC_ModBusInterface : public QWidget
{
    Q_OBJECT

public:
    PLC_ModBusInterface(QWidget *parent = nullptr)
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
        _portEdit = new IntDragValueWidget(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(8080);  // Modbus TCP默认端口
        connectionLayout->addWidget(_portEdit, 1, 1, 1, 1);
        
        // Server ID设置
        connectionLayout->addWidget(new QLabel("ServerID(Useless):"), 2, 0, 1, 1);
        _serverId = new IntDragValueWidget(this);
        _serverId->setRange(0, 255);
        _serverId->setValue(1);
        connectionLayout->addWidget(_serverId, 2, 1, 1, 1);

        // PLC 类型选择
        connectionLayout->addWidget(new QLabel("PLC类型:"), 3, 0, 1, 1);
        _plcTypeCombo = new QComboBox(this);
        _plcTypeCombo->addItem("Beckhoff");
        _plcTypeCombo->addItem("Siemens");
        connectionLayout->addWidget(_plcTypeCombo, 3, 1, 1, 1);

        // 起始地址显示（由 DataModel 根据 PLC 类型设置）
        // connectionLayout->addWidget(new QLabel("起始地址:"), 4, 0, 1, 1);
        // _baseAddressLabel = new QLabel("-", this);
        // connectionLayout->addWidget(_baseAddressLabel, 4, 1, 1, 1);

        // 连接状态指示器
        _statusLabel = new QPushButton("状态: 未连接", this);
        _statusLabel->setEnabled(false);
        _statusLabel->setCheckable(true);
        _statusLabel->setFlat(true);
        _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        connectionLayout->addWidget(_statusLabel, 5, 0, 1, 1);

        layout->addWidget(connectionGroup, 0, 0, 1, 1);

        // 创建保持寄存器控制组（Z：保持寄存器）
        _controlGroup = new QGroupBox("控制寄存器", this);
        auto controlLayout = new QGridLayout(_controlGroup);

        for (int i = 0; i < 32; ++i) {
            _controlButtons[i] = new QPushButton(QString("%1").arg(i), this);
            _controlButtons[i]->setCheckable(true); // 使按钮可切换状态
            _controlButtons[i]->setEnabled(false);
            controlLayout->addWidget(_controlButtons[i], i / 4, i % 4);
            connect(_controlButtons[i], &QPushButton::toggled, this, [this, i](bool checked) {
               emit controlOutputChanged(i, checked);
           });
        }

        layout->addWidget(_controlGroup, 1, 0, 1, 1);

        // 创建状态显示组
        _statusGroup = new QGroupBox("状态寄存器", this);
        auto statusLayout = new QGridLayout(_statusGroup);

        for (int i = 0; i < 32; ++i) {
            _statusCheckBoxes[i] = new QCheckBox(QString("%1").arg(i), this);
            _statusCheckBoxes[i]->setEnabled(false);
            // 保持可勾选状态但只读，确保正常显示
            // _statusCheckBoxes[i]->setAttribute(Qt::WA_TransparentForMouseEvents);
            _statusCheckBoxes[i]->setFocusPolicy(Qt::NoFocus);
            statusLayout->addWidget(_statusCheckBoxes[i], i / 4, i % 4);
        }
        layout->addWidget(_statusGroup, 2, 0, 1, 1);

        // // 连接输出按钮的信号
        // for (int i = 0; i < 32; ++i) {
        //     connect(_controlButtons[i], &QPushButton::toggled, this, [this, i](bool checked) {
        //         emit controlOutputChanged(i, checked);
        //     });
        // }

        connect(_plcTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
            emit plcTypeChanged(idx);
        });
        
        // 连接IP和端口编辑框的信号
        connect(_hostEdit, &QLineEdit::editingFinished, this, [this]() {
            emit hostChanged(_hostEdit->text());
        });

        connect(_portEdit, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit portChanged(value);
        });

        // 设置大小策略
        // setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        // setMinimumSize(250, 450); // 调整最小尺寸以适应新的UI元素
    }


    void setZLabels(const QStringList& labels) {
        const int n = qMin(labels.size(), 32);
        for (int i = 0; i < n; ++i) {
            if (_controlButtons[i]) {
                _controlButtons[i]->setText(labels.at(i));
            }
        }
    }

    /**
     * @brief 设置状态标签
     * @param labels 标签列表（长度最多 32）
     */
    void setSLabels(const QStringList& labels) {
        const int n = qMin(labels.size(), 32);
        for (int i = 0; i < n; ++i) {
            if (_statusCheckBoxes[i]) {
                _statusCheckBoxes[i]->setText(labels.at(i));
            }
        }
    }
    
    /**
     * @brief 设置控制输出状态
     * @param index 输出索引(0-31)
     * @param state 输出状态
     */
    void setControlOutputState(int index, bool state) {
        if (index >= 0 && index < 32) {
            _controlButtons[index]->blockSignals(true);
            _controlButtons[index]->setChecked(state);
            _controlButtons[index]->blockSignals(false);
        }
    }

    /**
     * @brief 设置状态显示
     * @param index 状态索引(0-31)
     * @param state 状态值
     */
    void setStatusDisplay(int index, bool state) {
        if (index >= 0 && index < 32) {
            _statusCheckBoxes[index]->blockSignals(true);
            _statusCheckBoxes[index]->setChecked(state);
            _statusCheckBoxes[index]->blockSignals(false);
        }
    }
    
    /**
     * @brief 获取控制输出状态
     * @param index 输出索引(0-31)
     * @return 输出状态
     */
    bool getControlOutputState(int index) const {
        if (index >= 0 && index < 32) {
            return _controlButtons[index]->isChecked();
        }
        return false;
    }
    
    /**
     * @brief 获取所有控制输出状态的位值
     * @return 32位控制输出状态值
     */
    quint32 getAllControlOutputsValue() const {
        quint32 value = 0;
        for (int i = 0; i < 32; ++i) {
            if (_controlButtons[i]->isChecked()) {
                value |= (quint32(1) << i);
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
            // 启用所有控制按钮
            for (int i = 0; i < 32; ++i) {
                _controlButtons[i]->setEnabled(true);
                // 状态复选框保持启用但只读
                // _statusCheckBoxes[i]->setEnabled(true);
            }
            _statusLabel->setChecked(true);
            _statusLabel->setText("状态: 已连接");
            _statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            // 禁用所有控制按钮
            for (int i = 0; i < 32; ++i) {
                _controlButtons[i]->setEnabled(false);
            }
            _statusLabel->setChecked(false);
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

signals:
    void hostChanged(const QString &host);
    void portChanged(int port);
    void controlOutputChanged(int index, bool state); // 更改信号名称
    void plcTypeChanged(int plcType);

public:
    QComboBox *_plcTypeCombo = nullptr;
    QGroupBox *_controlGroup = nullptr; // 更改为控制组
    QGroupBox *_statusGroup = nullptr;  // 新增状态组
    QLineEdit *_hostEdit;
    IntDragValueWidget *_portEdit;
    IntDragValueWidget *_serverId;
    QPushButton *_statusLabel;
    QPushButton *_controlButtons[32]; // 更改为QPushButton
    QCheckBox *_statusCheckBoxes[32]; // 新增状态复选框
};