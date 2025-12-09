#pragma once

#include <iostream>
#include <QComboBox>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "DataTypes/NodeDataList.hpp"
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>

#include "ConstantDefines.h"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"
#include "OSCMessage.h"
#include "OSCSender/OSCSender.h"
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using namespace NodeDataTypes;
namespace Nodes
{
    class ImageLoaderModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        ImageLoaderModel() {
            InPortCount =0;
            OutPortCount=1;


            CaptionVisible=true;
            Caption="Image File";
            WidgetEmbeddable= true;
            Resizable=false;
            PortEditable= false;
            m_outImageData=std::make_shared<ImageData>();
            NodeDelegateModel::registerOSCControl("/file",_fileSelectComboBox);
            connect(_fileSelectComboBox, &SelectorComboBox::textChanged, this, &ImageLoaderModel::loadImage);
        }
        ~ImageLoaderModel() override = default;


    public:
        NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            return ImageData().type();
        }

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override{
            return m_outImageData;
        }

        void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex const portIndex) override {}

        QWidget *embeddedWidget() override {

            return _fileSelectComboBox;
            // return _selectorComboBox;
        }

        QJsonObject save() const override{
            QJsonObject modelJson = NodeDelegateModel::save();
            if (m_outImageData && !_fileSelectComboBox->text().isEmpty()) {
                modelJson["path"] = _fileSelectComboBox->text();
            }
            return modelJson;
        }

        void load(QJsonObject const& jsonObj) override{

            const QJsonValue path = jsonObj["path"];
            if (!path.isUndefined()) {
               _fileSelectComboBox->setText(path.toString());
                loadImage(path.toString());
            }
        }

        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    private:
        void loadImage(QString fileName){
    
            if (!fileName.isEmpty()) {
                m_path= AppConstants::MEDIA_LIBRARY_STORAGE_DIR+"/"+fileName;
                m_outImageData = std::make_shared<ImageData>(m_path);
                if (m_outImageData && !m_outImageData->image().isNull()) {
                    updateNodeState(QtNodes::NodeValidationState::State::Valid);
    
                }
            } else {
                updateNodeState(QtNodes::NodeValidationState::State::Error,"cannot open file");
                m_outImageData.reset();
              
            }
            emit dataUpdated(0);
        }

    private:
        // QLabel *_label=new QLabel("Ctrl+left click to load image");
        // QComboBox* _fileSelectComboBox = new QComboBox();
        SelectorComboBox* _fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Image);
        QString m_path;
        std::shared_ptr<ImageData> m_outImageData;
        // MediaLibrary* mediaLibrary = MediaLibrary::instance();
    };
}