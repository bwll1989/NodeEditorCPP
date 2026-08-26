#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <unordered_map>
#include <memory>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;

namespace Nodes {

/// Container 子图 Audio 出口：内部写入后由外壳中继到外层。
class AudioOutDataModel : public AbstractDelegateModel
{
    Q_OBJECT

public:
    AudioOutDataModel();
    ~AudioOutDataModel() override = default;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;
    QWidget *embeddedWidget() override;

private:
    std::unordered_map<PortIndex, std::shared_ptr<NodeData>> _dataMap;
};

} // namespace Nodes
