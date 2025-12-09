#pragma once
#include "QWidget"
#include "QLabel"
#include "QGridLayout"
#include "QVBoxLayout"
#include <QSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFileInfo>
#include <QCheckBox>  // 新增：包含QCheckBox头文件
#include <QTimeEdit>
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
namespace Nodes
{
    /**
     * @brief Artnet Universe回放节点界面
     * 提供Universe配置界面、视频文件选择和播放控制功能
     */
    class UniversePlaybackInterface: public QWidget{
        Q_OBJECT
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit UniversePlaybackInterface(QWidget *parent = nullptr){
            setupUI();
        }

    public:
        // Universe配置控件
        QSpinBox *universeSpinBox = new QSpinBox(this);
        QSpinBox *subnetSpinBox = new QSpinBox(this);
        QSpinBox *netSpinBox = new QSpinBox(this);
        
        // 控制按钮
        QPushButton *clearButton = new QPushButton("清空数据", this);
        // QPushButton *selectFileButton = new QPushButton("选择视频", this);
        QPushButton *playButton = new QPushButton("播放", this);
        QPushButton *stopButton = new QPushButton("停止", this);
        
        // 循环播放控制
        QCheckBox *loopCheckBox = new QCheckBox("循环播放", this);  // 新增：循环播放复选框
        SelectorComboBox* _fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::DMX);
        // // 信息显示
        // QLabel *fileNameLabel = new QLabel("未选择文件", this);
        QLabel *videoInfoLabel = new QLabel("", this);
        QLabel *statusLabel = new QLabel("就绪", this);
        /**
         * 函数级注释：当前播放时间显示，格式 hh:mm:ss.zzz，只读
         */
        QTimeEdit *currentTimeEdit = new QTimeEdit(this);

        /**
         * @brief 更新文件信息显示
         * @param fileName 文件名
         * @param duration 时长
         * @param fps 帧率
         */
        void updateFileInfo(const QString& fileName, double duration, double fps,int videoHeight) {
            QFileInfo fileInfo(fileName);
            videoInfoLabel->setText(QString("时长: %1s, 帧率: %2fps, 域数量: %3")
                                   .arg(duration, 0, 'f', 1).arg(fps, 0, 'f', 1).arg(videoHeight));
        }
        
        /**
         * @brief 更新播放状态
         * @param playing 是否正在播放
         */
        void updatePlaybackState(bool playing) {
            playButton->setEnabled(!playing);
            stopButton->setEnabled(playing);
            statusLabel->setText(playing ? "播放中" : "已停止");
        }
        
        /**
         * @brief 显示错误信息
         * @param error 错误信息
         */
        void showError(const QString& error) {
            statusLabel->setText(QString("错误: %1").arg(error));
            statusLabel->setStyleSheet("color: red;");
        }

    signals:
        /**
         * @brief 清空按钮点击信号
         */
        void clearDataClicked();
        
        /**
         * @brief 选择文件按钮点击信号
         */
        void selectFileClicked(QString fileName);
        
        /**
         * @brief 播放按钮点击信号
         */
        void playClicked();
        
        /**
         * @brief 停止按钮点击信号
         */
        void stopClicked();
        
        /**
         * @brief 循环播放状态改变信号
         * @param enabled 是否启用循环播放
         */
        void loopStateChanged(bool enabled);  // 新增：循环播放状态改变信号

    private:
        /**
         * @brief 设置用户界面
         */
        void setupUI() {
            QVBoxLayout *mainLayout = new QVBoxLayout(this);
            mainLayout->setSpacing(8);
            mainLayout->setContentsMargins(0, 0, 0, 0);
            
            // Universe配置组
            createUniverseConfigGroup(mainLayout);
            
            // 视频文件组
            createVideoFileGroup(mainLayout);
            
            // 播放控制组
            createPlaybackControlGroup(mainLayout);

            
            this->setLayout(mainLayout);
            this->setMinimumSize(250, 300);
        }

        /**
         * @brief 创建Universe配置组
         * @param parentLayout 父布局
         */
        void createUniverseConfigGroup(QVBoxLayout *parentLayout) {
            QGroupBox *configGroup = new QGroupBox("Universe 配置", this);
            QGridLayout *configLayout = new QGridLayout(configGroup);
            
            // Universe设置
            configLayout->addWidget(new QLabel("Universe:", this), 0, 0);
            universeSpinBox->setRange(0, 15);
            universeSpinBox->setValue(0);
            universeSpinBox->setToolTip("Universe编号 (0-15)");
            configLayout->addWidget(universeSpinBox, 0, 1);
            
            // Subnet设置
            configLayout->addWidget(new QLabel("Subnet:", this), 1, 0);
            subnetSpinBox->setRange(0, 15);
            subnetSpinBox->setValue(0);
            subnetSpinBox->setToolTip("Subnet编号 (0-15)");
            configLayout->addWidget(subnetSpinBox, 1, 1);
            
            // Net设置
            configLayout->addWidget(new QLabel("Net:", this), 2, 0);
            netSpinBox->setRange(0, 127);
            netSpinBox->setValue(0);
            netSpinBox->setToolTip("Net编号 (0-127)");
            configLayout->addWidget(netSpinBox, 2, 1);
            
            parentLayout->addWidget(configGroup);
        }

        /**
         * @brief 创建视频文件组
         * @param parentLayout 父布局
         */
        void createVideoFileGroup(QVBoxLayout *parentLayout) {
            QGroupBox *fileGroup = new QGroupBox("DMX文件", this);
            QVBoxLayout *fileLayout = new QVBoxLayout(fileGroup);
            
            // 文件选择按钮
            fileLayout->addWidget(_fileSelectComboBox);
            connect(_fileSelectComboBox, &SelectorComboBox::textChanged, this, &UniversePlaybackInterface::selectFileClicked);
            // 视频信息显示
            videoInfoLabel->setStyleSheet("color: gray;");
            fileLayout->addWidget(videoInfoLabel);
            
            parentLayout->addWidget(fileGroup);
        }
        
        /**
         * @brief 创建播放控制组
         * @param parentLayout 父布局
         */
        void createPlaybackControlGroup(QVBoxLayout *parentLayout) {
            QGroupBox *playbackGroup = new QGroupBox("播放控制", this);
            QGridLayout *playbackLayout = new QGridLayout(playbackGroup);
            currentTimeEdit->setDisplayFormat("hh:mm:ss.zzz");
            currentTimeEdit->setReadOnly(true);
            currentTimeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
            currentTimeEdit->setTime(QTime(0,0,0,0));
            playbackLayout->addWidget(currentTimeEdit, 0, 0,1,2);
            // 循环播放复选框
            loopCheckBox->setToolTip("启用后播放结束时会自动重新开始");
           
            connect(loopCheckBox, &QCheckBox::toggled, this, &UniversePlaybackInterface::loopStateChanged);
            playbackLayout->addWidget(loopCheckBox,1,0,1,2);

            playButton->setToolTip("开始播放视频（25fps）");
            playButton->setStyleSheet(
                "QPushButton {"
                "    background-color: #4CAF50;"
                "    color: white;"
                "    border: none;"
                "    padding: 8px 16px;"
                "    border-radius: 4px;"
                "    font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "    background-color: #45a049;"
                "}"
                "QPushButton:disabled {"
                "    background-color: #cccccc;"
                "}"
            );
            connect(playButton, &QPushButton::clicked, this, &UniversePlaybackInterface::playClicked);
            playbackLayout->addWidget(playButton,2,0,1,1);
            
            stopButton->setToolTip("停止播放并重置到开头");
            stopButton->setEnabled(false);
            stopButton->setStyleSheet(
                "QPushButton {"
                "    background-color: #f44336;"
                "    color: white;"
                "    border: none;"
                "    padding: 8px 16px;"
                "    border-radius: 4px;"
                "    font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "    background-color: #da190b;"
                "}"
                "QPushButton:disabled {"
                "    background-color: #cccccc;"
                "}"
            );
            connect(stopButton, &QPushButton::clicked, this, &UniversePlaybackInterface::stopClicked);
            playbackLayout->addWidget(stopButton,2,1,1,1);

            // 清空按钮
            clearButton->setToolTip("清空当前Universe的所有DMX数据（将所有512个通道设置为0）");
            clearButton->setStyleSheet(
                "QPushButton {"
                "    background-color: #ff6b6b;"
                "    color: white;"
                "    border: none;"
                "    padding: 8px 16px;"
                "    border-radius: 4px;"
                "    font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "    background-color: #ff5252;"
                "}"
                "QPushButton:pressed {"
                "    background-color: #e53935;"
                "}"
            );

            // 连接清空按钮信号
            connect(clearButton, &QPushButton::clicked, this, &UniversePlaybackInterface::clearDataClicked);

            playbackLayout->addWidget(clearButton,3,0,1,2);
            // 状态显示
            statusLabel->setAlignment(Qt::AlignCenter);
            statusLabel->setStyleSheet("font-weight: bold; padding: 4px;");
            playbackLayout->addWidget(statusLabel,4,0,1,2);
            parentLayout->addWidget(playbackGroup);
        }
    };
}
