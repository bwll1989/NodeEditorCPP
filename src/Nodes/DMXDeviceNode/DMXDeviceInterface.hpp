#pragma once

#include <QLineEdit>
#include "QWidget"
#include "QLabel"
#include "QGridLayout"
#include "QVBoxLayout"
#include "QHBoxLayout"
#include <QSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QGroupBox>
#include <QScrollArea>
#include <QVector>
#include <QCheckBox>

namespace Nodes
{
    /**
     * @brief 通道设备节点界面
     * 提供多通道数据的配置和控制界面，支持动态通道数量
     */
    class DMXDeviceInterface: public QWidget{
        Q_OBJECT
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit DMXDeviceInterface(QWidget *parent = nullptr){
            setupUI();
            updateChannelControls(5); // 默认5个通道
        }

        /**
         * @brief 更新地址范围显示
         * @param startAddr 起始地址
         * @param channelCount 通道数量
         */
        void updateAddressRangeLabel(int startAddr, int channelCount) {
            int endAddr = startAddr + channelCount - 1;
            addressRangeLabel->setText(QString("地址范围: %1-%2").arg(startAddr).arg(endAddr));
        }

        /**
         * @brief 更新通道控件数量
         * @param channelCount 新的通道数量
         */
        void updateChannelControls(int channelCount) {
            // 清除现有控件
            clearChannelControls();
            
            // 创建新的控件
            createChannelControls(channelCount);
            
            // 更新组标题
            channelGroup->setTitle(QString("通道控制 (%1个通道)").arg(channelCount));
        }

        /**
         * @brief 获取指定通道的滑块控件
         * @param index 通道索引
         * @return 滑块控件指针，如果索引无效返回nullptr
         */
        QSlider* getChannelSlider(int index) {
            if (index >= 0 && index < channelSliders.size()) {
                return channelSliders[index];
            }
            return nullptr;
        }

        /**
         * @brief 获取指定通道的标签控件
         * @param index 通道索引
         * @return 标签控件指针，如果索引无效返回nullptr
         */
        QLabel* getChannelLabel(int index) {
            if (index >= 0 && index < channelLabels.size()) {
                return channelLabels[index];
            }
            return nullptr;
        }

        /**
         * @brief 获取当前通道数量
         * @return 当前通道数量
         */
        int getChannelCount() const {
            return channelSliders.size();
        }

    public:
        // 通道配置控件
        QSpinBox *channelCountSpinBox = new QSpinBox(this);
        QSpinBox *startAddressSpinBox = new QSpinBox(this);  // 起始DMX地址
        QLabel *addressRangeLabel;  // 地址范围显示标签
        QCheckBox *enableCheckBox = new QCheckBox(this);  // 启用/禁用控件

    signals:
        /**
         * @brief 启用状态改变信号
         * @param enabled 是否启用
         */
        void enableStateChanged(bool enabled);

    private:
        /**
         * @brief 设置用户界面
         */
        void setupUI() {
            QVBoxLayout *mainLayout = new QVBoxLayout(this);
            mainLayout->setSpacing(5);
            mainLayout->setContentsMargins(5, 5, 5, 5);
            
            // 通道配置组
            createChannelConfigGroup(mainLayout);
            
            // 通道控制组
            createChannelControlGroup(mainLayout);
            
            this->setLayout(mainLayout);
            this->setMinimumSize(200, 300);
        }

        /**
         * @brief 创建通道配置组
         * @param parentLayout 父布局
         */
        void createChannelConfigGroup(QVBoxLayout *parentLayout) {
            QGroupBox *configGroup = new QGroupBox("通道配置", this);
            QGridLayout *configLayout = new QGridLayout(configGroup);
            
            // 启用/禁用控件
            enableCheckBox->setText("启用输出");
            enableCheckBox->setChecked(true);  // 默认启用
            enableCheckBox->setToolTip("启用时发送当前值，禁用时发送0值");
            enableCheckBox->setStyleSheet("QCheckBox { font-weight: bold; color: #2196F3; }");
            configLayout->addWidget(enableCheckBox, 0, 0, 1, 2);
            
            // 连接信号
            connect(enableCheckBox, &QCheckBox::toggled, this, &DMXDeviceInterface::enableStateChanged);
            
            // 起始DMX地址设置
            configLayout->addWidget(new QLabel("起始地址:", this), 1, 0);
            startAddressSpinBox->setRange(1, 512);
            startAddressSpinBox->setValue(1);
            startAddressSpinBox->setToolTip("DMX起始地址 (1-512)");
            configLayout->addWidget(startAddressSpinBox, 1, 1);
            
            // 通道数量设置
            configLayout->addWidget(new QLabel("通道数:", this), 2, 0);
            channelCountSpinBox->setRange(1, 512);
            channelCountSpinBox->setValue(5);
            channelCountSpinBox->setToolTip("通道数量 (1-512)");
            configLayout->addWidget(channelCountSpinBox, 2, 1);
            
            // 添加地址范围显示标签
            addressRangeLabel = new QLabel("地址范围: 1-5", this);
            addressRangeLabel->setStyleSheet("color: #888; font-style: italic;");
            configLayout->addWidget(addressRangeLabel, 3, 0, 1, 2);
            
            parentLayout->addWidget(configGroup);
        }

        /**
         * @brief 创建通道控制组
         * @param parentLayout 父布局
         */
        void createChannelControlGroup(QVBoxLayout *parentLayout) {
            channelGroup = new QGroupBox("通道控制", this);
            
            // 创建滚动区域
            scrollArea = new QScrollArea(this);
            scrollWidget = new QWidget();
            channelLayout = new QVBoxLayout(scrollWidget);  // 改为垂直布局
            
            scrollWidget->setLayout(channelLayout);
            scrollArea->setWidget(scrollWidget);
            scrollArea->setWidgetResizable(true);
            scrollArea->setMaximumHeight(200);
            
            QVBoxLayout *groupLayout = new QVBoxLayout(channelGroup);
            groupLayout->setContentsMargins(0,2,0,2);
            groupLayout->addWidget(scrollArea);
            
            parentLayout->addWidget(channelGroup);
        }

        /**
         * @brief 清除所有通道控件
         */
        void clearChannelControls() {
            // 清除布局中的所有控件
            QLayoutItem *item;
            while ((item = channelLayout->takeAt(0)) != nullptr) {
                if (item->widget()) {
                    item->widget()->deleteLater();
                }
                delete item;
            }
            
            // 清空容器
            channelSliders.clear();
            channelLabels.clear();
        }

        /**
         * @brief 创建指定数量的通道控件
         * @param channelCount 通道数量
         */
        void createChannelControls(int channelCount) {
            channelSliders.resize(channelCount);
            channelLabels.resize(channelCount);
            
            for (int i = 0; i < channelCount; ++i) {
                // 创建每个通道的容器
                QWidget *channelWidget = new QWidget(scrollWidget);
                QHBoxLayout *channelWidgetLayout = new QHBoxLayout(channelWidget);
                channelWidgetLayout->setContentsMargins(0, 2, 0, 2);
                channelWidgetLayout->setSpacing(5);
                
                // 通道标签 - 从CH0开始，对应设备通道
                channelLabels[i] = new QLabel(QString("CH: %1").arg(i), channelWidget);
                channelLabels[i]->setStyleSheet("font-weight: bold; color: orange;");
                
                // 通道滑块
                channelSliders[i] = new QSlider(Qt::Horizontal, channelWidget);
                channelSliders[i]->setRange(0, 255);
                channelSliders[i]->setValue(0);

                
                // 将标签和滑块添加到水平布局中
                channelWidgetLayout->addWidget(channelLabels[i]);
                channelWidgetLayout->addWidget(channelSliders[i]);
                channelWidgetLayout->setStretch(0,1);
                channelWidgetLayout->setStretch(1,4);
                // 将通道容器添加到垂直布局中（竖向排列）
                channelLayout->addWidget(channelWidget);
            }
            
            // 添加弹性空间，使控件向上对齐
            channelLayout->addStretch();
            
            // 更新滚动区域
            scrollWidget->updateGeometry();
            scrollArea->updateGeometry();
        }

    private:
        // 动态通道控件容器
        QVector<QSlider*> channelSliders;
        QVector<QLabel*> channelLabels;
        
        // 界面组件
        QGroupBox *channelGroup;
        QScrollArea *scrollArea;
        QWidget *scrollWidget;
        QVBoxLayout *channelLayout;  // 改为垂直布局指针
    };
}
