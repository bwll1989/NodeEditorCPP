#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "Nodes/NodeDataList.hpp"
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
class UNTITLED_EXPORT ImageShowModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    ImageShowModel(): _label(new QLabel("Image will appear here"))
    {
        _label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

        QFont f = _label->font();
        f.setBold(true);
        f.setItalic(true);
        WidgetEmbeddable= false;
        _label->setFont(f);

        _label->setMinimumSize(200, 200);

        _label->installEventFilter(this);
        Caption="Image Display";
    }

    ~ImageShowModel() override= default;

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
//    bool widgetEmbeddable() const override { return false; }
    unsigned int nPorts(PortType const portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result = 1;
                break;

            case PortType::Out:
                result = 1;

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
        _nodeData = nodeData;

        if (_nodeData) {
            auto d = std::dynamic_pointer_cast<PixmapData>(_nodeData);

            int w = _label->width();
            int h = _label->height();

            _label->setPixmap(d->pixmap().scaled(w, h, Qt::KeepAspectRatio));
        } else {
            _label->setPixmap(QPixmap());
        }

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

            if (event->type() == QEvent::Resize) {
                auto d = std::dynamic_pointer_cast<PixmapData>(_nodeData);
                if (d) {
                    _label->setPixmap(d->pixmap().scaled(w, h, Qt::KeepAspectRatio));
                }
            }
        }

        return false;
    }


private:
    QLabel *_label;

    std::shared_ptr<NodeData> _nodeData;
};