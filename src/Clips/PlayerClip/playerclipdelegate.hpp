#ifndef PLAYERCLIPDELEGATE_H
#define PLAYERCLIPDELEGATE_H

#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipDelegate.hpp"
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
#include "Widget/TimeLineWidget/TimelineAbstract/AbstractClipModel.hpp"
#include "playerclipmodel.hpp"

class PlayerClipDelegate : public AbstractClipDelegate {
    Q_OBJECT
public:
    explicit PlayerClipDelegate(QObject *parent = nullptr) 
        : AbstractClipDelegate(parent)
        , m_model(nullptr)  // 在构造函数中初始化
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
        auto* self = const_cast<PlayerClipDelegate*>(this);
        self->m_model = qobject_cast<PlayerClipModel*>(clip);
        if (!self->m_model) {
            qDebug() << "Failed to cast to PlayerClipModel";
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
        durationBox->setText(FramesToTimeString(m_model->length(), Timecode::getFrameRate(m_model->getTimecodeType())));
        basicLayout->addWidget(durationBox, 1, 1);
         // 文件名显示
        auto* fileNameLabel = new QLineEdit(m_model->filePath(), basicGroup);
        fileNameLabel->setReadOnly(true);
        basicLayout->addWidget(fileNameLabel, 2, 0, 1, 2);
        // 媒体文件选择
        basicLayout->addWidget(new QLabel("媒体文件:"), 4, 0);
        auto* mediaButton = new QPushButton("选择媒体文件", basicGroup);
        basicLayout->addWidget(mediaButton, 4, 1);        
        // 连接信号槽
        connect(mediaButton, &QPushButton::clicked, [=]() {
            QString filePath = QFileDialog::getOpenFileName(editor,
                "选择媒体文件",
                "",
                "视频文件 (*.mp4 *.avi *.mkv *.mov);;"
                "音频文件 (*.mp3 *.wav *.aac *.flac);;"
                "图片文件 (*.jpg *.jpeg *.png *.bmp);;"
                "所有文件 (*)");
            
            if (!filePath.isEmpty()) {
                m_model->setFilePath(filePath);  // 这会触发视频信息加载和长度更新
                fileNameLabel->setText(filePath);
                // 更新时长显示，使用时间码格式
                durationBox->setText(FramesToTimeString(m_model->length(), Timecode::getFrameRate(m_model->getTimecodeType())));
     
            }
        });

        mainLayout->addWidget(basicGroup);
        auto* playGroup = new QGroupBox("播放器设置", editor);
        auto* playLayout = new QGridLayout(playGroup);
        QLineEdit* playerName = new QLineEdit(playGroup);
        playerName->setText(m_model->oscHost());
        connect(playerName, &QLineEdit::editingFinished, [=]() {
            m_model->setOscHost(playerName->text());
        });
        playLayout->addWidget(playerName, 0, 0,1,2);
        auto* playerID = new QLineEdit(playGroup);
        playerID->setText(m_model->playerID());
        connect(playerID, &QLineEdit::editingFinished, [=]() {
            m_model->setPlayerID(playerID->text());
        });
        playLayout->addWidget(playerID, 0, 2,1,2);
        auto* playButton = new QPushButton("播放", playGroup);
        connect(playButton, &QPushButton::clicked, [=]() {
            m_model->play();
        });
        auto* stopButton = new QPushButton("停止", playGroup);
        connect(stopButton, &QPushButton::clicked, [=]() {
            m_model->stop();
        });
        auto* pauseButton = new QPushButton("暂停", playGroup);
        connect(pauseButton, &QPushButton::clicked, [=]() {
            m_model->pause();
        });
        auto* fullScreenButton = new QPushButton("全屏", playGroup);
        fullScreenButton->setCheckable(true);
        connect(fullScreenButton, &QPushButton::clicked, [=]() {
            m_model->fullScreen(fullScreenButton->isChecked());
        });
        playLayout->addWidget(playButton, 1, 0);
        playLayout->addWidget(stopButton, 1, 1);
        playLayout->addWidget(pauseButton, 1, 2);
        playLayout->addWidget(fullScreenButton, 1, 3);
        mainLayout->addWidget(playGroup);
        return editor;
    }
 
private:
    PlayerClipModel* m_model;  // 使用 m_ 前缀表示成员变量
   
};

#endif // PLAYERCLIPDELEGATE_H 