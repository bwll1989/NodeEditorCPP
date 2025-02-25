#ifndef VIDEOCLIPDELEGATE_H
#define VIDEOCLIPDELEGATE_H

#include "TimeLineWidget/AbstractClipDelegate.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QGroupBox>

class VideoClipDelegate : public AbstractClipDelegate {
    Q_OBJECT
public:
    explicit VideoClipDelegate(QObject *parent = nullptr) : AbstractClipDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {}
    
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QWidget* editor = new QWidget(parent);
        QVBoxLayout* mainLayout = new QVBoxLayout(editor);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(4);

        // 基本等级组
        auto* basicGroup = new QGroupBox("基本设置", editor);
        auto* basicLayout = new QGridLayout(basicGroup);
        
        // 时间相关控件
        basicLayout->addWidget(new QLabel("时间长度:"), 1, 0);
        auto* durationSpinBox = new QSpinBox(basicGroup);
        durationSpinBox->setValue(13566);
        basicLayout->addWidget(durationSpinBox, 1, 1);
        
        basicLayout->addWidget(new QLabel("开始时间:"), 3, 0);
        auto* startTimeSpinBox = new QSpinBox(basicGroup);
        startTimeSpinBox->setValue(0);
        basicLayout->addWidget(startTimeSpinBox, 3, 1);
        
        // 媒体文件选择
        basicLayout->addWidget(new QLabel("媒体:"), 4, 0);
        auto* mediaButton = new QPushButton("寻找媒体", basicGroup);
        basicLayout->addWidget(mediaButton, 4, 1, 1, 2);
        
        // 文件名显示
        auto* fileNameLabel = new QLabel("", basicGroup);
        basicLayout->addWidget(fileNameLabel, 5, 0, 1, 3);
        
        mainLayout->addWidget(basicGroup);

        // 原始初始位置组
        auto* posGroup = new QGroupBox("原始初始位置", editor);
        auto* posLayout = new QGridLayout(posGroup);
        
        posLayout->addWidget(new QLabel("X轴:"), 0, 0);
        auto* xSpinBox = new QSpinBox(posGroup);
        xSpinBox->setRange(-9999, 9999);
        xSpinBox->setValue(0);
        posLayout->addWidget(xSpinBox, 0, 1);
        
        posLayout->addWidget(new QLabel("Y轴:"), 1, 0);
        auto* ySpinBox = new QSpinBox(posGroup);
        ySpinBox->setRange(-9999, 9999);
        ySpinBox->setValue(0);
        posLayout->addWidget(ySpinBox, 1, 1);
        
        posLayout->addWidget(new QLabel("Z轴:"), 2, 0);
        auto* zSpinBox = new QSpinBox(posGroup);
        zSpinBox->setRange(-9999, 9999);
        zSpinBox->setValue(0);
        posLayout->addWidget(zSpinBox, 2, 1);
        
        mainLayout->addWidget(posGroup);

        return editor;
    }

private slots:
    void openFileDialog() {
        QString filePath = QFileDialog::getOpenFileName(nullptr, 
            "选择视频文件", 
            "", 
            "视频文件 (*.mp4 *.avi *.mkv *.mov);;所有文件 (*)");
        if (!filePath.isEmpty()) {
            // 处理文件选择
            // 可以发出信号来处理选中的文件
        }
    }
};

#endif // VIDEOCLIPDELEGATE_H 