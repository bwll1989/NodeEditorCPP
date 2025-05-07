//
// Created by Administrator on 2023/12/13.
//

#include <QTreeWidget>
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include <QFileDialog>  // 新增文件对话框支持

class AudioDecoderInterface: public QWidget{
public:
    explicit AudioDecoderInterface(QWidget *parent = nullptr) {
        main_layout = new QGridLayout(this);  // 正确初始化网格布局

        // 初始化文件显示框（设为只读）
        fileDisplay->setReadOnly(true);
        main_layout->addWidget(fileDisplay, 0, 0, 1, 3);

        // 按钮布局
        main_layout->addWidget(button, 1, 0);
        main_layout->addWidget(button1, 1, 1);
        main_layout->addWidget(button2, 1, 2);

        // 树控件设置
        treeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        main_layout->addWidget(treeWidget, 2, 0, 1, 3);
        main_layout->setContentsMargins(4,2,4,4);
        this->setLayout(main_layout);


    }
public:
    // 修正后的成员变量
    QGridLayout *main_layout;  // 统一使用网格布局
public:
    void selectFile() {
        // 完善文件对话框参数
        QString path = QFileDialog::getOpenFileName(this, "选择音频文件", "",
            "音频文件 (*.mp3 *.wav *.flac *.aac)");

        if (!path.isEmpty()) {
            fileDisplay->setText(path);
            // ... 保持原有解码初始化逻辑 ...
        }
    }
    QLineEdit *fileDisplay = new QLineEdit(this);
    QPushButton *button=new QPushButton("select");
    QPushButton *button1=new QPushButton("play");
    QPushButton *button2=new QPushButton("stop");
    QTreeView *treeWidget=new QTreeView(this);
//    QHBoxLayout *sub_layout=new QHBoxLayout;

};

