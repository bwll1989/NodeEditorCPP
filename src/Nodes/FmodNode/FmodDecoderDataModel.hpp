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

namespace Nodes
{
    class FmodDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
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

    public slots:
        void select_audio_file();
        void onEventSelected(const QString& eventPath);
        void updateEventListUI(const QStringList& events);

    private:
        FmodDecoderInterface* widget = nullptr;
        
        // Output Buffers
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputBuffers_;
        // 缓存可用事件列表，便于索引触发
        QStringList availableEvents_;
        
        // Worker Thread
        QThread* workerThread_ = nullptr;
        FmodDecoderWorker* worker_ = nullptr;
    };
}
