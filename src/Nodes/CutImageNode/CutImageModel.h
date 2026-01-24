//
// Created by pablo on 3/9/24.
//

#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QFutureWatcher>

#include "DataTypes/NodeDataList.hpp"
#include "CutImageInterface.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
using namespace NodeDataTypes;
struct GlobalEvent;
namespace Nodes
{
    class CutImageModel final : public AbstractDelegateModel {
        Q_OBJECT
        Q_PROPERTY(int topLeftX READ topLeftX WRITE setTopLeftX NOTIFY topLeftXChanged)
        Q_PROPERTY(int topLeftY READ topLeftY WRITE setTopLeftY NOTIFY topLeftYChanged)
        Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)
        Q_PROPERTY(int height READ height WRITE setHeight NOTIFY heightChanged)

    public:
        CutImageModel();

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override;

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override;

        QWidget* embeddedWidget() override;

        QJsonObject save() const override;

        void load(const QJsonObject &p) override;

        /**
         * 函数级注释：获取当前裁剪矩形左上角 X 坐标属性
         */
        int topLeftX() const { return m_outRect.left(); }

        /**
         * 函数级注释：获取当前裁剪矩形左上角 Y 坐标属性
         */
        int topLeftY() const { return m_outRect.top(); }

        /**
         * 函数级注释：获取当前裁剪矩形宽度属性
         */
        int width() const { return m_outRect.width(); }

        /**
         * 函数级注释：获取当前裁剪矩形高度属性
         */
        int height() const { return m_outRect.height(); }

    public slots:
        /**
         * 函数级注释：设置裁剪矩形左上角 X 坐标属性，并触发图像处理与状态反馈
         */
        void setTopLeftX(int x);

        /**
         * 函数级注释：设置裁剪矩形左上角 Y 坐标属性，并触发图像处理与状态反馈
         */
        void setTopLeftY(int y);

        /**
         * 函数级注释：设置裁剪矩形宽度属性，并触发图像处理与状态反馈
         */
        void setWidth(int w);

        /**
         * 函数级注释：设置裁剪矩形高度属性，并触发图像处理与状态反馈
         */
        void setHeight(int h);

    Q_SIGNALS:
        /**
         * 函数级注释：当裁剪矩形左上角 X 坐标属性变化时发出的通知信号
         */
        void topLeftXChanged(int x);

        /**
         * 函数级注释：当裁剪矩形左上角 Y 坐标属性变化时发出的通知信号
         */
        void topLeftYChanged(int y);

        /**
         * 函数级注释：当裁剪矩形宽度属性变化时发出的通知信号
         */
        void widthChanged(int w);

        /**
         * 函数级注释：当裁剪矩形高度属性变化时发出的通知信号
         */
        void heightChanged(int h);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，实现外部命令控制裁剪参数
         */
        void afterModelReady() override;

    private slots:
        void processImage();
        /**
         * 函数级注释：处理来自全局事件总线的裁剪命令，映射到对应属性
         */
        void onGlobalEvent(const GlobalEvent& ev);
    private:
        std::weak_ptr<ImageData> m_inImageData;

        QRect m_outRect;

        std::shared_ptr<ImageData> m_outImageData;

        QVariant m_inData;

        CutImageInterface* widget=new CutImageInterface();


    };
}

