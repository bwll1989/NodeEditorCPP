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
#include "TimeLineWidget/AbstractClipModel.hpp"
#include "videoclipmodel.hpp"

class VideoClipDelegate : public AbstractClipDelegate {
    Q_OBJECT
public:
    explicit VideoClipDelegate(QObject *parent = nullptr) 
        : AbstractClipDelegate(parent)
        , m_model(nullptr)  // 在构造函数中初始化
        , m_xSpinBox(new QSpinBox())
        , m_ySpinBox(new QSpinBox())
        , m_sizeXSpinBox(new QSpinBox())
        , m_sizeYSpinBox(new QSpinBox())
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {}
    
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        auto* editor = new QWidget(parent);
        QVBoxLayout* mainLayout = new QVBoxLayout(editor);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(4);
        // 尝试直接从 internalPointer 获取
        auto* clip = static_cast<AbstractClipModel*>(index.internalPointer());
        if (!clip) {
            // 尝试通过 data 函数获取
            clip = index.data(TimelineRoles::ClipModelRole).value<AbstractClipModel*>();
            if (!clip) {
                qDebug() << "Failed to get clip from data()";
                return editor;
            }
        }

        // 使用 const_cast 因为 createEditor 是 const 成员函数
        auto* self = const_cast<VideoClipDelegate*>(this);
        self->m_model = qobject_cast<VideoClipModel*>(clip);
        if (!self->m_model) {
            qDebug() << "Failed to cast to VideoClipModel";
            return editor;
        }

        // 基本设置组
        auto* basicGroup = new QGroupBox("文件属性", editor);
        auto* basicLayout = new QGridLayout(basicGroup);
        // 时间相关控件
        basicLayout->addWidget(new QLabel("文件时长:"), 1, 0);
        auto* durationBox = new QLineEdit(basicGroup);
        durationBox->setReadOnly(true);
        // 将帧数转换为时间码格式显示
        durationBox->setText(FramesToTimeString(m_model->length(), m_model->getFrameRate()));
        basicLayout->addWidget(durationBox, 1, 1);
         // 文件名显示
        auto* fileNameLabel = new QLineEdit(m_model->filePath(), basicGroup);
        fileNameLabel->setReadOnly(true);
        basicLayout->addWidget(fileNameLabel, 2, 0, 1, 2);
        // 媒体文件选择
        basicLayout->addWidget(new QLabel("媒体文件:"), 4, 0);
        auto* mediaButton = new QPushButton("选择视频", basicGroup);
        basicLayout->addWidget(mediaButton, 4, 1);        
        // 连接信号槽
        connect(mediaButton, &QPushButton::clicked, [=]() {
            QString filePath = QFileDialog::getOpenFileName(editor,
                "选择视频文件",
                "",
                "视频文件 (*.mp4 *.avi *.mkv *.mov);;所有文件 (*)");
            
            if (!filePath.isEmpty()) {
                m_model->setFilePath(filePath);  // 这会触发视频信息加载和长度更新
                fileNameLabel->setText(filePath);
                // 更新时长显示，使用时间码格式
                durationBox->setText(FramesToTimeString(m_model->length(), m_model->getFrameRate()));
                m_sizeXSpinBox->setValue(m_model->getWidth());
                m_sizeYSpinBox->setValue(m_model->getHeight());
                m_xSpinBox->setValue(m_model->getPosX());
                m_ySpinBox->setValue(m_model->getPosY());
            }
        });

        mainLayout->addWidget(basicGroup);

        basicLayout->addWidget(new QLabel("水平位置:"), 5, 0,1,1);
        self->m_xSpinBox->setRange(-9999, 9999);
        self->m_xSpinBox->setValue(m_model->getPosX());
        basicLayout->addWidget(self->m_xSpinBox, 5, 1,1,1);
        
        basicLayout->addWidget(new QLabel("垂直位置:"), 6, 0,1,1);
        self->m_ySpinBox->setRange(-9999, 9999);
        self->m_ySpinBox->setValue(m_model->getPosY());
        basicLayout->addWidget(self->m_ySpinBox, 6, 1,1,1);        
        mainLayout->addWidget(basicGroup);
        // 添加一个按钮，用于设置大小
        auto* setSizeXLabel = new QLabel("宽度", basicGroup);
        basicLayout->addWidget(setSizeXLabel, 7, 0, 1, 1);
        self->m_sizeXSpinBox->setRange(0, 9999);
        self->m_sizeXSpinBox->setValue(m_model->getWidth());
        basicLayout->addWidget(self->m_sizeXSpinBox, 7, 1, 1, 1);
        auto* setSizeYLabel = new QLabel("高度", basicGroup);
        basicLayout->addWidget(setSizeYLabel, 8, 0, 1, 1);
        self->m_sizeYSpinBox->setRange(0, 9999);
        self->m_sizeYSpinBox->setValue(m_model->getHeight());
        basicLayout->addWidget(self->m_sizeYSpinBox, 8, 1, 1, 1);
        connect(self->m_sizeXSpinBox, &QSpinBox::valueChanged, [=]() {
            m_model->setWidth(self->m_sizeXSpinBox->value());
        });
        connect(self->m_sizeYSpinBox, &QSpinBox::valueChanged, [=]() {
            m_model->setHeight(self->m_sizeYSpinBox->value());
        });
        connect(self->m_xSpinBox, &QSpinBox::valueChanged, [=]() {
            if (m_model) {
                m_model->setPosX(self->m_xSpinBox->value());
            }
        });
        connect(self->m_ySpinBox, &QSpinBox::valueChanged, [=]() {
            if (m_model) {
                m_model->setPosY(self->m_ySpinBox->value());
            }
        });
        return editor;
    }
 
private:
    VideoClipModel* m_model;  // 使用 m_ 前缀表示成员变量
    QSpinBox* m_xSpinBox;
    QSpinBox* m_ySpinBox;
    QSpinBox* m_sizeXSpinBox;
    QSpinBox* m_sizeYSpinBox;
};

#endif // VIDEOCLIPDELEGATE_H 