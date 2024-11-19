#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "DataTypes/NodeDataList.hpp"

#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>
#include <QtCore/qglobal.h>
#if defined(UNTITLED_LIBRARY)
#  define UNTITLED_EXPORT Q_DECL_EXPORT
#else
#  define UNTITLED_EXPORT Q_DECL_IMPORT
#endif
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class UNTITLED_EXPORT GroupNode : public NodeDelegateModel
{
    Q_OBJECT

public:
    GroupNode(): _label(new QWidget())
    {
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        WidgetEmbeddable= true;
        Resizable=true;
        PortEditable= false;
        _label->setWindowOpacity(0.1);

        QFont f = _label->font();
        f.setBold(true);
        f.setItalic(true);
        _label->setFont(f);

        _label->setMinimumSize(300, 300);

        _label->installEventFilter(this);
        Caption="TimeCode Source";
    }

    ~GroupNode() override= default;

public:

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "In "+QString::number(portIndex);
        QString out = "Out "+QString::number(portIndex);
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
    }


public:

    unsigned int nPorts(PortType const portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result =0;
                break;

            case PortType::Out:
                result = 0;

            default:
                break;
        }

        return result;
    }

    NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
    {
        Q_UNUSED(portType);
        Q_UNUSED(portIndex);
        return PixmapData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return _nodeData;
    }

    void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
    {
        Q_UNUSED(port);
        // _nodeData = nodeData;
        //
        // if (_nodeData) {
        //     auto d = std::dynamic_pointer_cast<PixmapData>(_nodeData);
        //
        //     int w = _label->width();
        //     int h = _label->height();
        //
        //     _label->setPixmap(d->pixmap().scaled(w, h, Qt::KeepAspectRatio));
        // } else {
        //     _label->setPixmap(QPixmap());
        // }

        Q_EMIT dataUpdated(0);
    }

    QWidget *embeddedWidget() override { return _label; }

    bool resizable() const override { return true; }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == _label) {
            int w = _label->width();
            int h = _label->height();
        qDebug() << event->type();
            if (event->type() == QEvent::Resize) {
                qDebug()<<"resize";
            }
        }

        return false;
    }


private:
    QWidget *_label;

    std::shared_ptr<NodeData> _nodeData;
};