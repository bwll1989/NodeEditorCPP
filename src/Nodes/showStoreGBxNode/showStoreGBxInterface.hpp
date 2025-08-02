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

class showStoreGBxInterface : public QWidget
{
    Q_OBJECT

public:
    showStoreGBxInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        // 创建主布局
        auto layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        // 创建连接设置组
        auto connectionGroup = new QGroupBox("连接设置", this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        // IP地址设置
        connectionLayout->addWidget(new QLabel("IP地址:"), 0, 0,1,1);
        _hostEdit = new QLineEdit("127.0.0.1", this);
        connectionLayout->addWidget(_hostEdit, 0, 1,1,1);
        // 连接按钮和状态显示
        layout->addWidget(connectionGroup, 0, 0, 1, 1);
        layout->setRowStretch(0,1);
        // 创建输出控制组
        auto outputGroup = new QGroupBox("播放控制", this);
        auto outputLayout = new QGridLayout(outputGroup);
        
        for (int i = 0; i < 4; ++i) {
            _playButtons[i] = new QPushButton(QString("Show%1").arg(i+1), this);
            _playButtons[i]->setEnabled(false); // 初始禁用输出复选框
            outputLayout->addWidget(_playButtons[i], i/4, i%4);
        }
         outputLayout->addWidget(Stop,2,0,1,4);
        layout->addWidget(outputGroup, 1, 0, 1, 1);
        layout->setRowStretch(1,3);
        // 连接按钮的信号
        for (int i = 0; i < 4; ++i) {
            connect(_playButtons[i], &QPushButton::clicked, this, [this, i](bool checked) {
                emit outputChanged(i, checked);
            });
        }
        
        // 连接IP和端口编辑框的信号
        connect(_hostEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
            emit hostChanged(text);
        });
        
        // 设置大小策略
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setMinimumSize(100, 200);
    }
    


    
    // 设置输出状态
    void setOutputState(int index, bool state) {
        if (index >= 0 && index < 4) {
            _playButtons[index]->setChecked(state);
        }
    }
    
    // 获取输出状态
    bool getOutputState(int index) const {
        if (index >= 0 && index < 4) {
            return _playButtons[index]->isChecked();
        }
        return false;
    }
    

    
    // 设置连接状态
    void setConnectionStatus(bool connected) {
        if (connected) {
            // 启用所有输出复选框
            for (int i = 0; i < 4; ++i) {
                _playButtons[i]->setEnabled(true);
            }
            Stop->setEnabled(true);
        } else {
            // 禁用所有输出复选框
            for (int i = 0; i < 4; ++i) {
                _playButtons[i]->setEnabled(false);
            }
            Stop->setEnabled(false);
        }
    }
    
    // 获取主机地址
    QString getHost() const {
        return _hostEdit->text();
    }
    
    // 设置主机地址
    void setHost(const QString &host) {
        _hostEdit->setText(host);
    }
    
    // 获取端口
    int getPort() const {
        return 23;
    }

    QPushButton *_playButtons[4];
    QPushButton *Stop=new QPushButton("Stop");
    QLabel *State[4];
signals:

    void hostChanged(const QString &host);
    void outputChanged(int index, bool state);
    
private:
    QLineEdit *_hostEdit;

};