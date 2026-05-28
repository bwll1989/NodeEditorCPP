#pragma once

#include <iostream>
#include <QComboBox>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "NodeDataList.hpp"
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include "Common/AppConfig/ConfigManager.h"
#include <QtWidgets/QFileDialog>
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using namespace NodeDataTypes;

struct GlobalEvent;
namespace Nodes
{
    class ImageLoaderModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)

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
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "file";
            binding.control = _fileSelectComboBox;
            AbstractDelegateModel::registerExternalBinding("/file", this, binding);

            connect(_fileSelectComboBox, &SelectorComboBox::textChanged, this, &ImageLoaderModel::setFile);
            connect(this, &ImageLoaderModel::fileChanged, this, [this](const QString&){
                {
                    QSignalBlocker blocker(_fileSelectComboBox);
                    _fileSelectComboBox->setText(m_file);
                }
                loadImage(m_file);
            });
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
            if (m_outImageData && !file().isEmpty()) {
                modelJson["path"] = file();
            }
            return modelJson;
        }

        void load(QJsonObject const& jsonObj) override{
            const QJsonValue path = jsonObj["path"];
            if (!path.isUndefined()) {
                setFile(path.toString());
            }
        }

        /**
         * 函数级注释：获取当前图像文件名属性（媒体库相对路径）
         */
        QString file() const {
            return m_file;
        }

        /**
         * 函数级注释：设置当前图像文件名属性，触发加载与状态反馈
         */
        void setFile(const QString& fileName){
            const QString trimmed = fileName.trimmed();
            if (trimmed == m_file) {
                return;
            }
            m_file = trimmed;
            Q_EMIT fileChanged(trimmed);
        }

    signals:
        /**
         * 函数级注释：当图像文件名属性发生变化时发出的通知信号
         */
        void fileChanged(const QString& file);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，实现外部文件路径控制
         */
        void afterModelReady() override {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/file"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:
        /**
         * 函数级注释：处理来自全局事件总线的文件路径命令，更新 file 属性
         */
        void onGlobalEvent(const GlobalEvent& ev){
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/file")) {
                return;
            }
            setFile(ev.payload.toString());
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
        QString m_file;
        // MediaLibrary* mediaLibrary = MediaLibrary::instance();
    };
}
