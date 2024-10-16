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
//#include "opencv2/core/core.hpp"
//#include "opencv2/imgproc/imgproc.hpp"
//#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/highgui/highgui_c.h"
//#include "opencv2/opencv.hpp"
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
//using namespace cv;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class UNTITLED_EXPORT ImageLoaderModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    ImageLoaderModel() : _label(new QLabel("Double click to load image"))
    {
        _label->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

        QFont f = _label->font();
        f.setBold(true);
        f.setItalic(true);
        _label->setFont(f);

        _label->setMinimumSize(200, 200);
        _label->setMaximumSize(500, 300);

        _label->installEventFilter(this);
        Caption="Image Input";
    }

    ~ImageLoaderModel() override= default;

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
    virtual QString modelName() const { return QString("Source Image"); }

    unsigned int nPorts(PortType const portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result = 0;
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
        Q_UNUSED(portIndex);
        Q_UNUSED(portType);
        return PixmapData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return std::make_shared<PixmapData>(_pixmap);
    }

    void setInData(std::shared_ptr<NodeData>, PortIndex const portIndex) override {}

    QWidget *embeddedWidget() override { return _label; }

    bool resizable() const override { return true; }

protected:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (object == _label) {
            int w = _label->width();
            int h = _label->height();

            if (event->type() == QEvent::MouseButtonPress ) {
                QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                                tr("Open Image"),
                                                                QDir::homePath(),
                                                                tr("Image Files (*.png *.jpg *.bmp)"));
//                cv::Mat img;
//                img=cv::imread(fileName.toStdString());
//                namedWindow("opencv test",CV_WINDOW_NORMAL);
//                imshow("opencv_test",img);
//                waitKey(0);
                _pixmap = QPixmap(fileName);

                _label->setPixmap(_pixmap.scaled(w, h, Qt::KeepAspectRatio));

                Q_EMIT dataUpdated(0);

                return true;
            } else if (event->type() == QEvent::Resize) {
                if (!_pixmap.isNull())
                    _label->setPixmap(_pixmap.scaled(w, h, Qt::KeepAspectRatio));
            } else if (event->type() == QEvent::ContextMenu){

                _label->setContextMenuPolicy(Qt::ActionsContextMenu);

            }

        }

        return false;
    }

private:
    QLabel *_label;

    QPixmap _pixmap;
};