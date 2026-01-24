#pragma once

#include <QImage>
#include <QPainter>
#include <QFont>
#include <QDebug>
#include <QFileDialog>
#include <QJsonArray>
#include <QSignalBlocker>
#include "TimeLineDefines.h"
#include <QPushButton>
#include "AbstractClipModel.hpp"
#include "TimeCodeDefines.h"
#include <QSlider>
// #include "BaseTimeLineModel.h"
#include "ConstantDefines.h"
#include "Common/Devices/ClientController/SocketTransmitter.h"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"
#include "AbstractClipDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
namespace Clips
{
    class ImageClipModel : public AbstractClipDelegateModel {
        Q_OBJECT
    public:
        explicit ImageClipModel(int start,const QString& filePath = QString(), QObject* parent = nullptr)
            : AbstractClipDelegateModel(start, "Image", parent),
              m_filePath(filePath),
              m_editor(nullptr)
        {
            EMBEDWIDGET = false;
            SHOWBORDER = true;
            m_canNotify = false;

            m_server = getClientControlInstance();
            initPropertyWidget();
            // 构造阶段先禁止通知

            // SocketTransmitter::getInstance();
            // 构造完成，允许通知
            m_canNotify = true;

            // 异步触发一次初始通知，确保事件循环就绪
            QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
        }

        ~ImageClipModel() override
        {
            QJsonDocument doc;
            QJsonArray array;  // 创建一个JSON数组
            QJsonObject command=save();
            command["remove"] = true;
            array.append(command);
            doc.setObject(QJsonObject{{"fileList", array}}); // 正确设置JSON文档的根对象
            m_server->enqueueJson(doc);
            // AbstractClipModel::~AbstractClipModel();


        };

        // 设置文件路径并加载视频信息
        void setMedia(const QVariant& path) override {
            // if (m_filePath != path) {
            //     m_filePath = path;
            //     emit filePathChanged(m_filePath);
            //     emit onPropertyChanged();
            // }
            mediaSelector->setText(path.toString());
            emit mediaSelector->textChanged(path.toString());
            onPropertyChanged();
        }

        void setStart(int start) override  {
            AbstractClipModel::setStart(start);
            if (m_canNotify) {
                QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
            }
        }
        void setEnd(int end) override  {
            AbstractClipModel::setEnd(end);
            if (m_canNotify) {
                QMetaObject::invokeMethod(this, "onPropertyChanged", Qt::QueuedConnection);
            }
        }

        // 重写保存和加载函数
        QJsonObject save() const override {
            QJsonObject json = AbstractClipModel::save();
            json["file"] = m_filePath;
            json["type"] = "Image";
            // json["startTime"] = timecode_frame_to_time(frames_to_timecode_frame(start(), m_timecodeType), m_timecodeType);
            // json["endTime"] = timecode_frame_to_time(frames_to_timecode_frame(end(), m_timecodeType), m_timecodeType);
            json["zIndex"] = layer->value();

            QJsonObject position;
            position["x"] = postion_x->value();
            position["y"] = postion_y->value();
            json["position"] = position;

            QJsonObject size;
            size["width"] = width->value();
            size["height"] = height->value();
            json["size"] = size;
            json["rotation"] = rotation->value();

            json["startTime"]=timecode_frame_to_time(frames_to_timecode_frame(start(),getTimeCodeType()),getTimeCodeType());
            json["endTime"]=timecode_frame_to_time(frames_to_timecode_frame(end(),getTimeCodeType()),getTimeCodeType());
            return json;
        }

        void load(const QJsonObject& json) override {
            AbstractClipModel::load(json);
            m_filePath = json["file"].toString();
            mediaSelector->setText(m_filePath);

            if(json.contains("position")) {
                QJsonObject position = json["position"].toObject();
                postion_x->setValue(position["x"].toInt());
                postion_y->setValue(position["y"].toInt());
            }

            if(json.contains("size")) {
                QJsonObject size = json["size"].toObject();
                width->setValue(size["width"].toInt());
                height->setValue(size["height"].toInt());
            }

            m_id = json["Id"].toInt();
            rotation->setValue(json["rotation"].toInt());
            layer->setValue(json["zIndex"].toInt());
           
        }

        QVariant data(int role) const override {
            switch (role) {
            case Qt::DisplayRole:
                return m_filePath;
            case TimelineRoles::ClipModelRole:
                return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast<ImageClipModel*>(this)));
            default:
                return AbstractClipModel::data(role);
            }
        }

        QWidget* clipPropertyWidget() override{
            m_editor = new QWidget();
            QVBoxLayout* mainLayout = new QVBoxLayout(m_editor);
            mainLayout->setContentsMargins(5, 5, 5, 5);
            mainLayout->setSpacing(4);
            // 基本设置组
            auto* basicGroup = new QGroupBox("文件属性", m_editor);
            auto* basicLayout = new QGridLayout(basicGroup);
            // 媒体文件选择
            basicLayout->addWidget(new QLabel("媒体文件:"), 0, 0,1,1);
            mediaSelector = new SelectorComboBox(MediaLibrary::Category::Image,basicGroup);
            basicLayout->addWidget(mediaSelector, 0, 1,1,2);
            connect(mediaSelector,&SelectorComboBox::textChanged,this,&ImageClipModel::onFileChange);
            registerExternalControl("/file", mediaSelector);
            mainLayout->addWidget(basicGroup);
            // 添加尺寸位置参数设置
            auto* positionGroup = new QGroupBox("位置参数", m_editor);
            auto* positionLayout = new QGridLayout(positionGroup);
            // 位置
            positionLayout->addWidget(new QLabel("X:"), 1, 0);
            postion_x = new QSpinBox(positionGroup);
            postion_x->setMinimum(-10000);
            postion_x->setMaximum(10000);
            postion_x->setValue(0);
            registerExternalControl("/posX",postion_x);
            positionLayout->addWidget(postion_x, 1, 1);
            positionLayout->addWidget(new QLabel("Y:"), 2, 0);
            postion_y = new QSpinBox(positionGroup);
            postion_y->setMinimum(-10000);
            postion_y->setMaximum(10000);
            postion_y->setValue(0);
            registerExternalControl("/posY",postion_y);
            positionLayout->addWidget(postion_y, 2, 1);
            positionLayout->addWidget(new QLabel("Width:"), 3, 0);
            width = new QSpinBox(positionGroup);
            width->setMinimum(0);
            width->setMaximum(10000);
            width->setValue(100);
            registerExternalControl("/width",width);
            positionLayout->addWidget(width, 3, 1);
            positionLayout->addWidget(new QLabel("Height:"), 4, 0);
            height = new QSpinBox(positionGroup);
            height->setMinimum(0);
            height->setMaximum(10000);
            height->setValue(100);
            registerExternalControl("/height",height);
            positionLayout->addWidget(height, 4, 1);
            positionLayout->addWidget(new QLabel("Layer:"), 5, 0);
            layer = new QSpinBox(positionGroup);
            layer->setSingleStep(1);
            layer->setMinimum(0);
            layer->setMaximum(5);
            layer->setValue(0);
            registerExternalControl("/layer",layer);
            positionLayout->addWidget(layer, 5, 1);
            rotation=new QSpinBox(positionGroup);
            rotation->setMinimum(-180);
            rotation->setMaximum(180);
            rotation->setValue(0);
            registerExternalControl("/rotation",rotation);
            positionLayout->addWidget(new QLabel("Rotate:"), 6, 0);
            positionLayout->addWidget(rotation, 6, 1);
            mainLayout->addWidget(positionGroup);
            // 连接信号槽
            connect(postion_x, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &ImageClipModel::onPositionXChange);
            connect(postion_y, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &ImageClipModel::onPositionYChange);
            connect(width, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &ImageClipModel::onWidthChange);
            connect(height, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &ImageClipModel::onHeightChange);
            connect(layer, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &ImageClipModel::onLayerChange);
            connect(rotation, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &ImageClipModel::onRotationChange);
            return m_editor;
        }

        QVariantMap currentData(int currentFrame) const override {
            QVariantMap data;
            // if (!m_oscSender) return data;

            if(currentFrame == start()) {
                data["/file"] = m_filePath;

                return data;
            }
            if(currentFrame+1 == end()) {
                data["/stop"]=1;
                return data;
            }

            return data;
        }


    public Q_SLOTS:
        void onPropertyChanged(){
            if (!m_server) {
                qWarning() << "[VideoClipModel] SocketTransmitter not ready, skip onPropertyChanged";
                return;
            }
            QJsonDocument doc;
            QJsonArray array;  // 创建一个JSON数组
            array.append(save()); // 将对象添加到数组中
            doc.setObject(QJsonObject{{"fileList", array}}); // 正确设置JSON文档的根对象
            m_server->enqueueJson(doc);
        }

        /**
         * 函数级注释：统一处理图片文件路径变更并发送状态反馈
         */
        void onFileChange(const QString& text){
                if (m_filePath != text) {
                    m_filePath = text;
                    emit filePathChanged(m_filePath);
                    emit onPropertyChanged();
                }
                stateFeedBack("/file", m_filePath);
        }
        /**
         * 函数级注释：统一处理图片宽度变更并发送状态反馈
         */
        void onWidthChange(int value){
            stateFeedBack("/width", value);
            onPropertyChanged();
        }

        /**
         * 函数级注释：统一处理图片高度变更并发送状态反馈
         */
        void onHeightChange(int value){
            stateFeedBack("/height", value);
            onPropertyChanged();
        }

        /**
         * 函数级注释：统一处理图层顺序变更并发送状态反馈
         */
        void onLayerChange(int value){
            stateFeedBack("/layer", value);
            onPropertyChanged();
        }

        /**
         * 函数级注释：统一处理图片旋转角度变更并发送状态反馈
         */
        void onRotationChange(int value){
            stateFeedBack("/rotation", value);
            onPropertyChanged();
        }

        /**
         * 函数级注释：统一处理图片 X 坐标变更并发送状态反馈
         */
        void onPositionXChange(int value){
            stateFeedBack("/posX", value);
            onPropertyChanged();
        }

        /**
         * 函数级注释：统一处理图片 Y 坐标变更并发送状态反馈
         */
        void onPositionYChange(int value){
            stateFeedBack("/posY", value);
            onPropertyChanged();
        }

        /**
         * 函数级注释：处理来自全局事件总线的外部命令，调用业务槽并阻断控件信号
         */
        void onGlobalEvent(const GlobalEvent& ev){
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addrPosX = makeFullOscAddress("/posX");
            const QString addrPosY = makeFullOscAddress("/posY");
            const QString addrWidth = makeFullOscAddress("/width");
            const QString addrHeight = makeFullOscAddress("/height");
            const QString addrLayer = makeFullOscAddress("/layer");
            const QString addrRotation = makeFullOscAddress("/rotation");

            if (ev.address == addrPosX && postion_x) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                onPositionXChange(v);
                QSignalBlocker blocker(postion_x);
                postion_x->setValue(v);
            } else if (ev.address == addrPosY && postion_y) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                onPositionYChange(v);
                QSignalBlocker blocker(postion_y);
                postion_y->setValue(v);
            } else if (ev.address == addrWidth && width) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                onWidthChange(v);
                QSignalBlocker blocker(width);
                width->setValue(v);
            } else if (ev.address == addrHeight && height) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                onHeightChange(v);
                QSignalBlocker blocker(height);
                height->setValue(v);
            } else if (ev.address == addrLayer && layer) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                onLayerChange(v);
                QSignalBlocker blocker(layer);
                layer->setValue(v);
            } else if (ev.address == addrRotation && rotation) {
                bool ok = false;
                int v = ev.payload.toInt(&ok);
                if (!ok) return;
                onRotationChange(v);
                QSignalBlocker blocker(rotation);
                rotation->setValue(v);
            }
        }
    protected:
        /**
         * 函数级注释：剪辑模型初始化完成后订阅位置与尺寸相关的外部命令
         */
        void afterModelReady() override{
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/posX"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/posY"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/width"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/height"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/layer"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/rotation"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }
    private:
        QString m_filePath;
        QWidget* m_editor;
     
        QSpinBox* postion_x;
        QSpinBox* postion_y;
        QSpinBox* width;
        QSpinBox* height;
        QSpinBox* layer;
        QSpinBox* rotation;
        SelectorComboBox* mediaSelector;
        SocketTransmitter *m_server;
        bool m_canNotify = false;
    };
}
