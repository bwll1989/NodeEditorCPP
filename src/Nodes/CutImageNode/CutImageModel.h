//
// Created by pablo on 3/9/24.
//

#ifndef CUTIMAGEMODEL_H
#define CUTIMAGEMODEL_H

#include <QtNodes/NodeDelegateModel>
#include <QFutureWatcher>

#include "DataTypes/NodeDataList.hpp"
#include "CutImageInterface.hpp"
using namespace NodeDataTypes;
namespace Nodes
{
    class CutImageModel final : public QtNodes::NodeDelegateModel {
        Q_OBJECT

    public:
        CutImageModel();

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override;

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override;

        QWidget* embeddedWidget() override;

        QJsonObject save() const override;

        void load(const QJsonObject &p) override;

    private slots:
        void processImage();
    private:
        std::weak_ptr<ImageData> m_inImageData;

        QRect m_outRect;

        std::shared_ptr<ImageData> m_outImageData;

        QVariant m_inData;

        CutImageInterface* widget=new CutImageInterface();
    };
}

#endif //CUTIMAGEMODEL_H
