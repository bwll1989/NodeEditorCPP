#pragma once
#include <QtNodes/NodeDelegateModelRegistry>
#include "DataTypes/NodeDataList.hpp"
using namespace NodeDataTypes;
namespace Nodes
{
    class ImageShowModel final : public QtNodes::NodeDelegateModel
    {
        Q_OBJECT

    public:
        ImageShowModel();

        ~ImageShowModel() override = default;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return _label; }

        bool resizable() const override { return true; }

    protected:
        bool eventFilter(QObject *object, QEvent *event) override;

    private:
        QLabel *_label;

        std::weak_ptr<ImageData> m_outData;
    };
}