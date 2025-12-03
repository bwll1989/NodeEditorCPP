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

namespace Nodes
{
    /**
     * @brief Artnet Universe节点界面
     * 提供Universe配置界面和清空功能
     */
    class DMXUniverseInterface: public QWidget{
        Q_OBJECT
    public:
        /**
         * @brief 构造函数，初始化界面
         * @param parent 父控件
         */
        explicit DMXUniverseInterface(QWidget *parent = nullptr){
            setupUI();
        }

    public:
        // Universe配置控件
        QSpinBox *universeSpinBox = new QSpinBox(this);
        QSpinBox *subnetSpinBox = new QSpinBox(this);
        QSpinBox *netSpinBox = new QSpinBox(this);
        
        // 清空按钮
        QPushButton *clearButton = new QPushButton("清空数据", this);

    signals:
        /**
         * @brief 清空按钮点击信号
         */
        void clearDataClicked();

    private:
        /**
         * @brief 设置用户界面
         */
        void setupUI() {
            QVBoxLayout *mainLayout = new QVBoxLayout(this);
            mainLayout->setSpacing(8);
            mainLayout->setContentsMargins(8, 8, 8, 8);
            
            // Universe配置组
            createUniverseConfigGroup(mainLayout);
            
            // 控制按钮组
            createControlGroup(mainLayout);
            
            this->setLayout(mainLayout);
            this->setMinimumSize(300, 200);
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
         * @brief 创建控制按钮组
         * @param parentLayout 父布局
         */
        void createControlGroup(QVBoxLayout *parentLayout) {
            QGroupBox *controlGroup = new QGroupBox("数据控制", this);
            QVBoxLayout *controlLayout = new QVBoxLayout(controlGroup);
            
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
            connect(clearButton, &QPushButton::clicked, this, &DMXUniverseInterface::clearDataClicked);

            controlLayout->addWidget(clearButton);
            parentLayout->addWidget(controlGroup);
        }
    };
}
