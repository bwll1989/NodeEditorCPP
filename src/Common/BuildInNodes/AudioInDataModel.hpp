#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QComboBox>
#include <iostream>
#include "ModelDataBridge/ModelDataBridge.hpp"
#include "Elements/DataBridgeSelectorBox/DataBridgeSelectorBox.hpp"
#include "ConstantDefines.h"
#include "AbstractDelegateModel.h"

struct GlobalEvent;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
namespace Nodes {
    class AudioInDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString remarks READ remarks WRITE setRemarks NOTIFY remarksChanged)

    public:
        AudioInDataModel();

        virtual ~AudioInDataModel();

    public:
        QJsonObject save() const override;

        void load(QJsonObject const &p) override;

        /**
         * 函数级注释：获取当前音频输入节点的备注属性值
         */
        QString remarks() const;

    public:

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override;

        std::shared_ptr<NodeData> outData(PortIndex port) override;

        void setInData(std::shared_ptr<NodeData>, PortIndex) override;

        QWidget *embeddedWidget() override;

        void setRemarks(const QString& remarks) override;

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
         */
        void afterModelReady() override;

    private Q_SLOTS:
        /**
         * 函数级注释：处理来自全局事件总线的外部命令，更新备注属性
         */
        void onGlobalEvent(const GlobalEvent& ev);

    signals:
        /**
         * 函数级注释：当音频输入节点备注属性发生变化时发出的通知信号
         */
        void remarksChanged(const QString &remarks);

    private:
        std::unordered_map<PortIndex, std::shared_ptr<NodeData>> _dataMap;
        DataBridgeSelectorBox *widget;

    };
}
