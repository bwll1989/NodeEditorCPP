#pragma once
#include <QtNodes/NodeDelegateModelRegistry>
#include <QWidget>
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
using namespace NodeDataTypes;
namespace Nodes
{
    class ImageShowModel final : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         */
        ImageShowModel();

        ~ImageShowModel() override = default;

        /**
         * @brief 获取节点端口数据类型
         */
        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        /**
         * @brief 输出当前图像数据
         */
        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        /**
         * @brief 设置输入图像并更新界面
         */
        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return _view; }

        bool resizable() const override { return true; }


    private:
        QWidget *_view;

        std::shared_ptr<ImageData> m_outData;
    };
}
