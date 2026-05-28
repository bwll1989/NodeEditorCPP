#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "NodeDataList.hpp"
#include "NoiseGeneratorInterface.hpp"
#include <memory>
#include <TimestampGenerator/TimestampGenerator.hpp>
#include "QtNodes/Definitions"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "QTimer"
#include "NoiseGenerator.hpp"
#include "QThread"
#include "StatusContainer/GlobalEventBus.hpp"


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
        Q_PROPERTY(double volume READ getVolume WRITE setVolume NOTIFY volumeChanged)
        Q_PROPERTY(int noiseType READ getNoiseType WRITE setNoiseType NOTIFY noiseTypeChanged)
        Q_PROPERTY(bool generating READ isGenerating WRITE setGenerating NOTIFY generatingChanged)

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
            generator->initializeGenerator(48000, 1, 32);  // 1声道，32位浮点
        
            // 设置初始音量为界面滑块的默认值 (0 dB)
            generator->setVolume(0.0);  // 0 dB = 线性值 1.0
        
            // 连接信号
            // 同步更新按钮文字：启动时显示“停止”，停止时显示“启动”
            connect(widget->startButton, &QPushButton::clicked,
                    this, [this](bool checked){
                        setGenerating(checked);
                        widget->startButton->setText(checked ? "停止" : "启动");
                    });

            connect(widget->volumeSlider, &FloatDragValueWidget::valueChanged,
                    this, &NoiseGeneratorDataModel::setVolume);
            connect(widget->noiseTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &NoiseGeneratorDataModel::setNoiseType);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "volume";
                b.control = widget->volumeSlider;
                AbstractDelegateModel::registerExternalBinding("/volume", this,b);
            }
            // AbstractDelegateModel::registerExternalControl("/volume", widget->volumeSlider);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "noiseType";
                b.control = widget->noiseTypeCombo;
                AbstractDelegateModel::registerExternalBinding("/noiseType", this,b);
            }
            // AbstractDelegateModel::registerExternalControl("/noise_type", widget->noiseTypeCombo);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "generating";
                b.control = widget->startButton;
                AbstractDelegateModel::registerExternalBinding("/generating", this,b);
            }
            // AbstractDelegateModel::registerExternalControl("/start", widget->startButton);
            // AbstractDelegateModel::registerExternalControl("/stop", widget->stopButton);
        }

        /**
         * @brief 析构函数，释放资源
         */
        ~NoiseGeneratorDataModel(){
            if (generator->isGenerating()){
                generator->stopGeneration();
            }
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            
            // Subscribe to properties
            bus->subscribe(makeFullOscAddress("/volume"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/noiseType"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/generating"), this, SLOT(onGlobalEvent(GlobalEvent)));
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
                if (portIndex == 2) return "START/STOP";
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
                        setVolume(variableData->value().toDouble());
                        break;
                    case 1: // 噪音类型控制
                        setNoiseType(variableData->value().toInt());
                        break;
                    case 2: // 启动停止
                        setGenerating(variableData->value().toBool());
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
            QJsonObject obj = NodeDelegateModel::save();
            QJsonObject values;
            values["volume"] = m_volume;
            values["noiseType"] = m_noiseType;
            values["generating"] = m_generating;
            obj["values"] = values;
            return obj;
        }

        /**
         * @brief 加载节点状态
         * @param p JSON对象
         */
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                QJsonObject values = v.toObject();
                if (values.contains("volume")) {
                    setVolume(values["volume"].toDouble());
                }
                if (values.contains("noiseType")) {
                    setNoiseType(values["noiseType"].toInt());
                }
                if (values.contains("generating")) {
                    setGenerating(values["generating"].toBool());
                }
                NodeDelegateModel::load(p);
            }
        }

    public:
        double getVolume() const { return m_volume; }
        void setVolume(double value) {
            if (qFuzzyCompare(m_volume, value)) return;
            m_volume = value;
            generator->setVolume(value);
            
            if (widget && !qFuzzyCompare(widget->volumeSlider->value(), value)) {
                QSignalBlocker blocker(widget->volumeSlider);
                widget->volumeSlider->setValue(value);
            }
            
            emit volumeChanged(value);
        }

        int getNoiseType() const { return m_noiseType; }
        void setNoiseType(int value) {
            if (m_noiseType == value) return;
            if (value < 0 || value >= 2) return;
            
            m_noiseType = value;
            generator->setNoiseType(static_cast<NoiseType>(value));
            
            if (widget && widget->noiseTypeCombo->currentIndex() != value) {
                QSignalBlocker blocker(widget->noiseTypeCombo);
                widget->noiseTypeCombo->setCurrentIndex(value);
            }
            
            emit noiseTypeChanged(value);
        }

        bool isGenerating() const { return m_generating; }
        void setGenerating(bool value) {
            if (m_generating == value) return;
            m_generating = value;
            
            if (value) {
                generator->startGeneration();
               
            } else {
                generator->stopGeneration();
               
            }
            widget->blockSignals(true);
            widget->startButton->setChecked(value);
            widget->startButton->setText(value ? "停止" : "启动");
            widget->blockSignals(false);
            emit generatingChanged(value);
        }

    Q_SIGNALS:
        void volumeChanged(double value);
        void noiseTypeChanged(int value);
        void generatingChanged(bool value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            
            if (localPath == "volume") {
                setVolume(ev.payload.toDouble());
            } else if (localPath == "noiseType") {
                setNoiseType(ev.payload.toInt());
            } else if (localPath == "generating") {
                setGenerating(ev.payload.toBool());
            }
        }
    
    private:
        NoiseGeneratorInterface *widget = new NoiseGeneratorInterface();
        NoiseGenerator *generator = new NoiseGenerator();
        
        double m_volume = 0.0;
        int m_noiseType = 0;
        bool m_generating = false;

    };
}

