//
// Created by Administrator on 2023/12/13.
//

#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QGridLayout"
#include <QSpinBox>
#include <QPushButton>

namespace Nodes
{
    class NDVControlInterface: public QWidget{
        Q_OBJECT
    public:
        explicit NDVControlInterface(QWidget *parent = nullptr){
            // IP 和端口设置在第一行
            main_layout->addWidget(IP, 0, 0, 1, 2);        // IP 地址占两列
            main_layout->addWidget(Port, 0, 2, 1, 2);      // 端口号占一列

            // 文件索引和播放控制在第二行
            main_layout->addWidget(FileIndexLabel, 1, 0, 1, 1); // 文件索引标签
            main_layout->addWidget(FileIndex, 1, 1, 1, 1);      // 文件索引输入框
            FileIndex->setRange(1, 10);
            main_layout->addWidget(Play, 1, 2, 1, 1);           // 播放按钮
            main_layout->addWidget(LoopPlay, 1, 3, 1, 1);      // 循环播放按钮

            main_layout->addWidget(Stop, 2, 0, 1, 4);
            // 关闭所有客户端在第五行
            main_layout->addWidget(ClosePC, 3, 0, 1, 4);   // 关闭所有客户端，占三列

            main_layout->setColumnStretch(0, 1);           // 第一列占比为 1
            main_layout->setColumnStretch(1, 1);           // 第二列占比为 1
            main_layout->setColumnStretch(2, 1);           // 第三列占比为 1
            main_layout->setColumnStretch(3, 1);           // 第四列占比为 1
            main_layout->setSpacing(5);                    // 设置网格间距
            Port->setRange(1000, 65535);
            Port->setValue(8008);
            FileIndex->setRange(0, 10);
            main_layout->setContentsMargins(5, 5, 5, 5);   // 设置边距

            this->setLayout(main_layout);
        }
        signals:
            void AddressChanged(const int &port);
    public slots:
        void valueChanged(const QString& propertyName, const QVariant& value) {
    }
    public:
        QGridLayout *main_layout=new QGridLayout(this);
        QLineEdit *IP=new QLineEdit("127.0.0.1",this);
        QSpinBox *Port=new QSpinBox(this);
        QLabel *FileIndexLabel = new QLabel("File Index:", this);
        QSpinBox *FileIndex = new QSpinBox(this);
        QPushButton *Play=new QPushButton("play",this);
        QPushButton *LoopPlay=new QPushButton("loop play",this);
        QPushButton *Stop=new QPushButton("stop",this);
        QPushButton *ClosePC=new QPushButton("Close all client",this);
    };
}
