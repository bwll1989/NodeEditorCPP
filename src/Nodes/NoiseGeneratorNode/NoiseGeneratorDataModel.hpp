#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "DataTypes/NodeDataList.hpp"
#include "NoiseGeneratorInterface.hpp"
#include <memory>
#include <TimestampGenerator/TimestampGenerator.hpp>

#include "ConstantDefines.h"
#include "QtNodes/Definitions"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "QTimer"
#include "NoiseGenerator.hpp"
#include "QThread"


using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::NodeId;
using QtNodes::NodeRole;

using namespace NodeDataTypes;

namespace Nodes
{
    class NoiseGeneratorDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
    public:
        /**
         * @brief 构造函数，初始化噪音生成器Node
         */
        NoiseGeneratorDataModel(){
            InPortCount = 3;
            OutPortCount = 1;  // 单声道输出
            CaptionVisible = true;
            Caption = "Noise Generator";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;
        
            // 初始化噪音生成器 - 单声道配置
            generator->initializeGenerator(48000, 1, 16);  // 1声道，16位整数
        
            // 设置初始音量为界面滑块的默认值 (0 dB)
            generator->setVolume(0.0);  // 0 dB = 线性值 1.0
        
            // 连接信号
            connect(widget->startButton, &QPushButton::clicked,
                    this, &NoiseGeneratorDataModel::startGeneration, Qt::QueuedConnection);
            connect(widget->stopButton, &QPushButton::clicked,
                    this, &NoiseGeneratorDataModel::stopGeneration, Qt::QueuedConnection);
            connect(widget->volumeSlider, &QDoubleSpinBox::valueChanged,
                    this, &NoiseGeneratorDataModel::onVolumeChanged, Qt::QueuedConnection);
            connect(widget->noiseTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &NoiseGeneratorDataModel::onNoiseTypeChanged, Qt::QueuedConnection);

            // 注册OSC控制
            AbstractDelegateModel::registerOSCControl("/volume", widget->volumeSlider);
            AbstractDelegateModel::registerOSCControl("/noise_type", widget->noiseTypeCombo);
            AbstractDelegateModel::registerOSCControl("/start", widget->startButton);
            AbstractDelegateModel::registerOSCControl("/stop", widget->stopButton);
        }

        /**
         * @brief 析构函数，释放资源
         */
        ~NoiseGeneratorDataModel(){
            if (generator->isGenerating()){
                generator->stopGeneration();
            }
        }

        /**
         * @brief 获取端口数据类型
         * @param portType 端口类型（输入/输出）
         * @param portIndex 端口索引
         * @return 端口数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return AudioData().type();
            case PortType::None:
                break;
            }

            return VariableData().type();
        }

        /**
         * @brief 获取指定端口的输出数据
         * @param port 端口索引 (0-单声道输出)
         * @return 包含共享环形缓冲区的音频数据
         */
        std::shared_ptr<NodeData> outData(PortIndex port) override
        {

            // 创建新的AudioData并设置共享环形缓冲区
            auto audioData = std::make_shared<AudioData>();
            audioData->setSharedAudioBuffer(generator->getAudioBuffer());
            return audioData;
        }

        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题字符串
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                if (portIndex == 0) return "GAIN";
                if (portIndex == 1) return "TYPE";
                if (portIndex == 2) return "START";
                break;
            case PortType::Out:
                if (portIndex == 0) return "Out";
                break;
            }
            return "";
        }

        /**
         * @brief 设置输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data) {
                if (auto variableData = std::dynamic_pointer_cast<VariableData>(data)) {
                    switch (portIndex) {
                    case 0: // 音量控制
                        {
                            double volume = variableData->value().toDouble();
                            generator->setVolume(volume);
                            widget->volumeSlider->setValue(volume);
                        }

                        break;
                    case 1: // 噪音类型控制
                        {
                            int type = variableData->value().toInt();
                            if (type >= 0 && type < 2)
                            {
                                generator->setNoiseType(static_cast<NoiseType>(type));
                                widget->noiseTypeCombo->setCurrentIndex(type);
                            }
                        }
                        break;
                    case 2: // 启动停止
                        {
                            bool type = variableData->value().toBool();
                            if (type)
                            {
                                widget->startButton->click();
                            }else
                                widget->stopButton->click();
                        }
                        break;
                    }
                }
            }
        }

        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        /**
         * @brief 保存节点状态
         * @return JSON对象
         */
        QJsonObject save() const override
        {
            QJsonObject obj;
            obj["volume"] = static_cast<double>(generator->getVolume());
            obj["noiseType"] = static_cast<int>(generator->getNoiseType());
            obj["generating"] = generator->isGenerating();
            return obj;
        }

        /**
         * @brief 加载节点状态
         * @param p JSON对象
         */
        void load(const QJsonObject &p) override
        {
            if (p.contains("volume")) {
                double volume = p["volume"].toDouble();
                generator->setVolume(volume);
                widget->volumeSlider->setValue(volume);
            }
            
            if (p.contains("noiseType")) {
                int type = p["noiseType"].toInt();
                generator->setNoiseType(static_cast<NoiseType>(type));
                widget->noiseTypeCombo->setCurrentIndex(type);
            }
            
            if (p.contains("generating") && p["generating"].toBool()) {
                generator->startGeneration();
            }
        }

    public slots:
        /**
         * @brief 开始生成噪音
         */
        void startGeneration() {
            // qDebug() << "Starting Noise Generator"<<TimestampGenerator::getInstance()->getCurrentFrameCount()<<TimestampGenerator::getInstance()->calculateFrameCountByTimeDelta(101);
            generator->startGeneration();
            widget->startButton->setEnabled(false);
            widget->stopButton->setEnabled(true);
        }

        /**
         * @brief 停止生成噪音
         */
        void stopGeneration() {
            generator->stopGeneration();
            widget->startButton->setEnabled(true);
            widget->stopButton->setEnabled(false);
        }

        /**
         * @brief 音量改变槽函数
         * @param value 音量值
         */
        void onVolumeChanged(double value) {
            generator->setVolume(value);
        }

        /**
         * @brief 噪音类型改变槽函数
         * @param index 类型索引
         */
        void onNoiseTypeChanged(int index) {
            generator->setNoiseType(static_cast<NoiseType>(index));
        }
    
    private:
        NoiseGeneratorInterface *widget = new NoiseGeneratorInterface();
        NoiseGenerator *generator = new NoiseGenerator();

    };
}

