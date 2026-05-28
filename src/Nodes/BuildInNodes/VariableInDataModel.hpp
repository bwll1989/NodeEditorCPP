#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QComboBox>
#include <iostream>
#include "ModelDataBridge/ModelDataBridge.hpp"
#include "Elements/DataBridgeSelectorBox/DataBridgeSelectorBox.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace QtNodes;

struct GlobalEvent;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
namespace Nodes {
    class VariableInDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString remarks READ remarks WRITE setRemarks NOTIFY remarksChanged)

    public:
        VariableInDataModel();

        virtual ~VariableInDataModel();

    public:
        QJsonObject save() const override;

        void load(QJsonObject const &p) override;

        /**
         * 函数级注释：获取当前变量输入节点的备注属性值
         */
        QString remarks() const;

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override;
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
         * 函数级注释：当变量输入节点备注属性发生变化时发出的通知信号
         */
        void remarksChanged(const QString &remarks);

    private:
        std::unordered_map<PortIndex, std::shared_ptr<NodeData>> _dataMap;
        DataBridgeSelectorBox *widget;

    };
}
