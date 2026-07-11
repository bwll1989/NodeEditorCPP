#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QVBoxLayout>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * @brief USR-IO424 Modbus 主机界面
 *
 * - DI：4 路离散输入，寄存器 0x0020~0x0023
 * - DO：4 路线圈输出，寄存器 0x0000~0x0003
 */
class USR_IO424Interface : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kChannelCount = 4;

    USR_IO424Interface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(6);

        auto connectionGroup = new QGroupBox("连接设置", this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        connectionLayout->setContentsMargins(6, 8, 6, 6);
        connectionLayout->setSpacing(4);

        connectionLayout->addWidget(new QLabel("IP地址:", this), 0, 0);
        _hostEdit = new QLineEdit("127.0.0.1", this);
        connectionLayout->addWidget(_hostEdit, 0, 1);

        connectionLayout->addWidget(new QLabel("端口:", this), 1, 0);
        _portEdit = new IntDragValueWidget(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(8080);
        connectionLayout->addWidget(_portEdit, 1, 1);

        connectionLayout->addWidget(new QLabel("ServerID(Useless):", this), 2, 0);
        _serverId = new IntDragValueWidget(this);
        _serverId->setRange(0, 255);
        _serverId->setValue(1);
        connectionLayout->addWidget(_serverId, 2, 1);

        _statusLabel = new QPushButton("状态: 未连接", this);
        _statusLabel->setEnabled(false);
        _statusLabel->setCheckable(true);
        _statusLabel->setFlat(true);
        _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        connectionLayout->addWidget(_statusLabel, 3, 0, 1, 2);

        mainLayout->addWidget(connectionGroup);

        _readAll = new QPushButton("Read All", this);
        _readAll->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
        _readAll->setEnabled(false);
        mainLayout->addWidget(_readAll);

        auto inputGroup = new QGroupBox("DI 状态 (0x0020~0x0023)", this);
        auto inputLayout = new QGridLayout(inputGroup);
        inputLayout->setContentsMargins(6, 8, 6, 6);
        inputLayout->setSpacing(4);

        for (int i = 0; i < kChannelCount; ++i) {
            _inputLabels[i] = new QCheckBox(QString("DI%1: 关").arg(i), this);
            _inputLabels[i]->setEnabled(false);
            _inputLabels[i]->setStyleSheet("color: gray; font-weight: bold;");
            _inputLabels[i]->setMinimumWidth(80);
            inputLayout->addWidget(_inputLabels[i], i / 2, i % 2);
        }

        mainLayout->addWidget(inputGroup);

        auto outputGroup = new QGroupBox("DO 控制 (0x0000~0x0003)", this);
        auto outputLayout = new QGridLayout(outputGroup);
        outputLayout->setContentsMargins(6, 8, 6, 6);
        outputLayout->setSpacing(4);

        for (int i = 0; i < kChannelCount; ++i) {
            _outputCheckBoxes[i] = new QCheckBox(QString("DO%1").arg(i), this);
            _outputCheckBoxes[i]->setEnabled(false);
            outputLayout->addWidget(_outputCheckBoxes[i], i / 2, i % 2);
        }

        mainLayout->addWidget(outputGroup);
        mainLayout->addStretch();

        for (int i = 0; i < kChannelCount; ++i) {
            connect(_outputCheckBoxes[i], &QCheckBox::toggled, this, [this, i](bool checked) {
                emit outputChanged(i, checked);
            });
        }

        connect(_hostEdit, &QLineEdit::editingFinished, this, [this]() {
            emit hostChanged(_hostEdit->text());
        });

        connect(_portEdit, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit portChanged(value);
        });

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumSize(220, 320);
    }

    void setInputState(int index, bool state) {
        if (index >= 0 && index < kChannelCount) {
            _inputLabels[index]->setChecked(state);
            _inputLabels[index]->setText(QString("DI%1: %2").arg(index).arg(state ? "开" : "关"));
            _inputLabels[index]->setStyleSheet(state ? "color: green; font-weight: bold;" : "color: gray; font-weight: bold;");
        }
    }

    void setOutputState(int index, bool state) {
        if (index >= 0 && index < kChannelCount) {
            _outputCheckBoxes[index]->blockSignals(true);
            _outputCheckBoxes[index]->setChecked(state);
            _outputCheckBoxes[index]->blockSignals(false);
        }
    }

    bool getOutputState(int index) const {
        if (index >= 0 && index < kChannelCount) {
            return _outputCheckBoxes[index]->isChecked();
        }
        return false;
    }

    int getAllOutputsValue() const {
        int value = 0;
        for (int i = 0; i < kChannelCount; ++i) {
            if (_outputCheckBoxes[i]->isChecked()) {
                value |= (1 << i);
            }
        }
        return value;
    }

    void setConnectionStatus(bool connected) {
        for (int i = 0; i < kChannelCount; ++i) {
            _outputCheckBoxes[i]->setEnabled(connected);
        }
        _readAll->setEnabled(connected);
        _statusLabel->setChecked(connected);
        _statusLabel->setText(connected ? "状态: 已连接" : "状态: 未连接");
        _statusLabel->setStyleSheet(connected ? "color: green; font-weight: bold;" : "color: red; font-weight: bold;");
    }

    QString getHost() const { return _hostEdit->text(); }
    void setHost(const QString &host) { _hostEdit->setText(host); }
    int getPort() const { return _portEdit->value(); }
    void setPort(int port) { _portEdit->setValue(port); }
    int getServerId() const { return _serverId->value(); }
    void setServerId(int id) { _serverId->setValue(id); }

    QCheckBox *_outputCheckBoxes[kChannelCount];
    QPushButton *_readAll;

signals:
    void hostChanged(const QString &host);
    void portChanged(int port);
    void outputChanged(int index, bool state);

public:
    QCheckBox *_inputLabels[kChannelCount];
    QLineEdit *_hostEdit;
    IntDragValueWidget *_portEdit;
    IntDragValueWidget *_serverId;
    QPushButton *_statusLabel;
};
