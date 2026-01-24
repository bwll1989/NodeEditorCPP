#pragma once
#include "QtNodes/NodeDelegateModel"
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include "DataTypes/NodeDataList.hpp"
#include "FmodDecoderInterface.hpp"
#include "QFileDialog"
#include <memory>
#include "QtNodes/Definitions"
#include "QTimer"
#include "QThread"
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "DataTypes/AudioData.h"
#include "FmodDecoderWorker.h"

using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::NodeId;
using QtNodes::NodeRole;

using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
    class FmodDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString bankPath READ bankPath WRITE setBankPath NOTIFY bankPathChanged)
        Q_PROPERTY(QString currentEvent READ currentEvent WRITE setCurrentEvent NOTIFY currentEventChanged)

    public:
        FmodDecoderDataModel();
        ~FmodDecoderDataModel() override;

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
        /**
         * @brief 设置端口输入
         * @param data 输入数据
         * @param portIndex 端口索引（0:事件文本，1:事件索引）
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override;

        std::shared_ptr<NodeData> outData(PortIndex port) override;

        QJsonObject save() const override;

        void load(QJsonObject const &p) override;

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
        
        QWidget* embeddedWidget() override { return widget; }

        /**
         * 函数级注释：获取当前 FMOD Bank 目录路径属性
         */
        QString bankPath() const;

        /**
         * 函数级注释：设置 FMOD Bank 目录路径属性，触发加载与状态反馈
         */
        void setBankPath(const QString& path);

        /**
         * 函数级注释：获取当前播放事件路径属性
         */
        QString currentEvent() const;

        /**
         * 函数级注释：设置当前播放事件路径属性，触发播放与状态反馈
         */
        void setCurrentEvent(const QString& eventPath);

    public slots:
        void select_audio_file();
        void onEventSelected(const QString& eventPath);
        void updateEventListUI(const QStringList& events);

    signals:
        /**
         * 函数级注释：当 FMOD Bank 路径属性发生变化时发出的通知信号
         */
        void bankPathChanged(const QString& path);

        /**
         * 函数级注释：当当前播放事件属性发生变化时发出的通知信号
         */
        void currentEventChanged(const QString& eventPath);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
         */
        void afterModelReady() override;

    private slots:
        /**
         * 函数级注释：处理来自全局事件总线的外部命令，更新 Bank 路径或当前事件
         */
        void onGlobalEvent(const GlobalEvent& ev);

    private:
        FmodDecoderInterface* widget = nullptr;
        
        // Output Buffers
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputBuffers_;
        // 缓存可用事件列表，便于索引触发
        QStringList availableEvents_;
        
        // Worker Thread
        QThread* workerThread_ = nullptr;
        FmodDecoderWorker* worker_ = nullptr;

        QString m_bankPath;
        QString m_currentEvent;
    };
}
