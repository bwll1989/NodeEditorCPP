#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QSpacerItem>

class showStoreGBxInterface : public QWidget
{
    Q_OBJECT

public:
    /**
     * 函数级注释：构造 showStoreGBx 界面，采用竖向 2*n 排列（两列多行）
     */
    showStoreGBxInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        Stop = new QPushButton(QStringLiteral("Stop"), this);
        status = new QPushButton(QStringLiteral("Disconnected"), this);

        auto layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);

        _hostEdit = new QLineEdit("127.0.0.1", this);
        layout->addWidget(new QLabel("Host:"), 0, 0, 1, 1);
        layout->addWidget(_hostEdit, 0, 1, 1, 1);

        layout->addWidget(status, 1, 0, 1, 2);
        status->setFlat(true);
        status->setCheckable(true);
        status->setEnabled(false);
        
        for (int i = 0; i < 4; ++i) {
            _playButtons[i] = new QPushButton(QString("Show%1").arg(i+1), this);
            _playButtons[i]->setEnabled(false); // 初始禁用输出复选框
            layout->addWidget(_playButtons[i], 2 + i, 0, 1, 2);
        }

        layout->addWidget(Stop, 6, 0, 1, 2);
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 7, 0, 1, 2);
        layout->setRowStretch(7, 1);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 1);
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

        } else {
            // 禁用所有输出复选框
            for (int i = 0; i < 4; ++i) {
                _playButtons[i]->setEnabled(false);
            }

        }
        Stop->setEnabled(connected);
        status->setText(connected?"Connected":"Disconnected");
        status->setStyleSheet(connected?"color: green; font-weight: bold;":"color: red; font-weight: bold;");
        status->setChecked(connected);
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

    QPushButton *_playButtons[4] = {};
    QPushButton *Stop = nullptr;
    QPushButton *status = nullptr;
    QLineEdit *_hostEdit = nullptr;
signals:

    void hostChanged(const QString &host);
    void outputChanged(int index, bool state);



};
