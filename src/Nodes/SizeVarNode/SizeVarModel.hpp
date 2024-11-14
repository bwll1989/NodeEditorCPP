//
// Created by pablo on 2/29/24.
//

#ifndef SIZEVARMODEL_H
#define SIZEVARMODEL_H

#include <QtNodes/NodeDelegateModel>
#include "DataTypes/NodeDataList.hpp"
#include "ui_SizeVarForm.h"
namespace Ui {
    class SizeVarForm;
}

class SizeVarModel final : public QtNodes::NodeDelegateModel {
    Q_OBJECT
public:
    SizeVarModel() {
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Size Source";
        WidgetEmbeddable= true;
        PortEditable= false;
        Resizable= true;
    };

    ~SizeVarModel() override{}



    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        switch (portType) {
            case QtNodes::PortType::In:
                return VariableData().type();
            case QtNodes::PortType::Out:
                return VariableData().type();
            default:
                return VariableData().type();
        }
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{
        m_inSizeData = std::dynamic_pointer_cast<VariableData>(nodeData);
        if (auto sizeData = m_inSizeData.lock()) {
            // 检查是否成功锁定
            const auto lockSize = sizeData->value();
            if (lockSize.isValid() &&  lockSize.canConvert<QSize>()) {
                // setOutSize();
                m_outSize=lockSize.toSize();
                m_ui->sb_width->setValue(lockSize.toSize().width());
                m_ui->sb_height->setValue(lockSize.toSize().height());
                m_ui->sb_width->setEnabled(false);
                m_ui->sb_height->setEnabled(false);
            } else {

                m_ui->sb_width->setValue(sizeData->value("Width").toInt());
                m_ui->sb_height->setValue(sizeData->value("Height").toInt());
                // enable the spinboxes
                m_ui->sb_width->setEnabled(true);
                m_ui->sb_height->setEnabled(true);
            }
        }
    }

    std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override {
        switch (port) {
            case 0:
                m_outData=std::make_shared<VariableData>(QVariant(m_outSize));
                m_outData->insert("Width", m_outSize.width());
                m_outData->insert("Height", m_outSize.height());
                return m_outData;
            default:
                return nullptr;
        }
    }

    QWidget* embeddedWidget() override{
        if (!m_widget) {
            m_ui = std::make_unique<Ui::SizeVarForm>();
            m_widget = new QWidget();
            m_ui->setupUi(m_widget);
            // sb_width and sb_height are QSpinBox
            connect(m_ui->sb_width, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
                setOutSize(QSize(value, m_outSize.height()));
            });
            connect(m_ui->sb_height, QOverload<int>::of(&QSpinBox::valueChanged), [this](int value) {
                setOutSize(QSize(m_outSize.width(), value));
            });
            // setOutSize(QSize(0, 0));
        }
        return m_widget;}

    QJsonObject save() const override {
        QJsonObject modelJson = NodeDelegateModel::save();
        if (m_inSizeData.expired() && m_ui) {
            modelJson["width"] = m_ui->sb_width->value();
            modelJson["height"] = m_ui->sb_height->value();
        }
        return modelJson;
    }

    void load(QJsonObject const& jsonObj) override{
        if (m_inSizeData.expired() && m_ui) {
            setOutSize(QSize(jsonObj["width"].toInt(), jsonObj["height"].toInt()));
        }}

private:
    void setOutSize(const QSize& size) {
        // updates the size from incoming data
        m_outSize=size;
        emit dataUpdated(0);
    }

private:
    std::unique_ptr<Ui::SizeVarForm> m_ui;
    QWidget* m_widget = nullptr;
    // in
    std::weak_ptr<VariableData> m_inSizeData;
    // out
    // 0
    QSize m_outSize;
    // std::shared_ptr<VariantData> m_outSizeData;
    // 1
    std::shared_ptr<VariableData> m_outData;
    // 2
    // std::shared_ptr<VariantData> m_outHeightData;
};



#endif //SIZEVARMODEL_H
