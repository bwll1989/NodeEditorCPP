#pragma once

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <unordered_map>
#include <memory>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"

struct GlobalEvent;

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::PortIndex;
using QtNodes::PortType;

namespace Nodes {

/// Container 子图 Image 入口：外壳注入后从 Out 口送入内部。
class ImageInDataModel : public AbstractDelegateModel
{
    Q_OBJECT
    Q_PROPERTY(QString remarks READ remarks WRITE setRemarks NOTIFY remarksChanged)

public:
    ImageInDataModel();
    ~ImageInDataModel() override = default;

    QString remarks() const;
    void setRemarks(const QString &remarks) override;

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override;
    std::shared_ptr<NodeData> outData(PortIndex port) override;
    void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override;
    QWidget *embeddedWidget() override;

protected:
    void afterModelReady() override;

private Q_SLOTS:
    void onGlobalEvent(const GlobalEvent &ev);

signals:
    void remarksChanged(const QString &remarks);

private:
    std::unordered_map<PortIndex, std::shared_ptr<NodeData>> _dataMap;
};

} // namespace Nodes
