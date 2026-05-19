#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtConcurrent/QtConcurrent>
#include <QColor>
#include "DataTypes/NodeDataList.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using namespace NodeDataTypes;


struct GlobalEvent;

namespace Nodes
{
    class ToHSVDataModel final : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

    public:
        ToHSVDataModel();

        ~ToHSVDataModel() override;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QColor color() const;
        void setColor(const QColor& c);

    signals:
        void colorChanged(const QColor& c);

    protected:
        void afterModelReady() override;

    private Q_SLOTS:
        void onColorChanged(const QColor& c);
        void onGlobalEvent(const GlobalEvent& ev);

    private:
        QColor m_color = QColor(0,0,0,255);
    };
}