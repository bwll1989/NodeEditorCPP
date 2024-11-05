//
// Created by pablo on 3/9/24.
//

#ifndef RECTMODEL_H
#define RECTMODEL_H


#include <opencv2/core/types.hpp>
#include <QtNodes/NodeDelegateModel>
#include "DataTypes/NodeDataList.hpp"
#include "ui_RectForm.h"
namespace Ui {
    class RectForm;
}

class RectModel final : public QtNodes::NodeDelegateModel {
    Q_OBJECT

public:
    RectModel(){
        m_outRect = QRect(0, 0, 0, 0);
        m_outRectsData = std::make_shared<RectData>(m_outRect);
        InPortCount =1;
        OutPortCount=2;
        Caption="Rect";
        CaptionVisible=true;
        WidgetEmbeddable= true;
        Resizable=false;
    }

    ~RectModel() override{}


    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
        switch (portType) {
            case QtNodes::PortType::In:
                return RectData().type();
            case QtNodes::PortType::Out:
                switch (portIndex) {
                    case 0:
                        return RectData().type();
                    case 1:
                        return PointData().type();
                    default:
                        return VariantData().type();
                }
            default:
                return VariantData().type();
        }
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{
        // if the input data is a rect, update the m_outRectValue and the ui
        if (auto rectData = std::dynamic_pointer_cast<RectData>(nodeData)) {
            m_inRectData = rectData;
            m_outRect = rectData->rect();
            m_outRectsData = rectData;
            if (m_ui) {
                // disable the widgets
                m_ui->sb_x->setEnabled(false);
                m_ui->sb_y->setEnabled(false);
                m_ui->sb_width->setEnabled(false);
                m_ui->sb_height->setEnabled(false);
                // block signals to avoid infinite loop
                QSignalBlocker blocker(m_ui->sb_x);
                QSignalBlocker blocker2(m_ui->sb_y);
                QSignalBlocker blocker3(m_ui->sb_width);
                QSignalBlocker blocker4(m_ui->sb_height);
                m_ui->sb_x->setValue(m_outRect.x());
                m_ui->sb_y->setValue(m_outRect.y());
                m_ui->sb_width->setValue(m_outRect.width());
                m_ui->sb_height->setValue(m_outRect.height());
            }
        }
        else {
            m_inRectData.reset();
            m_outRect = QRect(0, 0, 0, 0);
            m_outRectsData = std::make_shared<RectData>(m_outRect);
            if (m_ui) {
                // enable the widgets
                m_ui->sb_x->setEnabled(true);
                m_ui->sb_y->setEnabled(true);
                m_ui->sb_width->setEnabled(true);
                m_ui->sb_height->setEnabled(true);
                // block signals to avoid infinite loop
                QSignalBlocker blocker(m_ui->sb_x);
                QSignalBlocker blocker2(m_ui->sb_y);
                QSignalBlocker blocker3(m_ui->sb_width);
                QSignalBlocker blocker4(m_ui->sb_height);
                m_ui->sb_x->setValue(m_outRect.x());
                m_ui->sb_y->setValue(m_outRect.y());
                m_ui->sb_width->setValue(m_outRect.width());
                m_ui->sb_height->setValue(m_outRect.height());
            }
        }
        emit dataUpdated(0);
        updateCenter();
    }

    std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override{
        switch (port) {
            case 0:
                return m_outRectsData;
            case 1:
                return m_outCenterData;
            default:
                return std::make_shared<VariantData>();
        }
    }

    QWidget* embeddedWidget() override
    { if (!m_widget) {
        m_ui = std::make_unique<Ui::RectForm>();
        m_widget = new QWidget();
        m_ui->setupUi(m_widget);
        connect(m_ui->sb_x, QOverload<int>::of(&QSpinBox::valueChanged), this, &RectModel::updateRectFromUI);
        connect(m_ui->sb_y, QOverload<int>::of(&QSpinBox::valueChanged), this, &RectModel::updateRectFromUI);
        connect(m_ui->sb_width, QOverload<int>::of(&QSpinBox::valueChanged), this, &RectModel::updateRectFromUI);
        connect(m_ui->sb_height, QOverload<int>::of(&QSpinBox::valueChanged), this, &RectModel::updateRectFromUI);
    }
        return m_widget;
    }


    QString portCaption(QtNodes::PortType port, QtNodes::PortIndex port_index) const override{
        switch (port) {
            case QtNodes::PortType::In:
                switch (port_index) {
                    case 0:
                        return "Rect";
                    default:
                        return QString();
                }
            case QtNodes::PortType::Out:
                switch (port_index) {
                    case 0:
                        return "Rect";
                    case 1:
                        return "Center";
                    default:
                        return QString();
                }
            default:
                return NodeDelegateModel::portCaption(port, port_index);
        }
    }

private slots:
    void updateRectFromUI(){
        // take the values from the ui and update the out data
        const int x = m_ui->sb_x->value();
        const int y = m_ui->sb_y->value();
        const int width = m_ui->sb_width->value();
        const int height = m_ui->sb_height->value();
        m_outRect = QRect(x, y, width, height);
        m_outRectsData = std::make_shared<RectData>(m_outRect);
        emit dataUpdated(0);
        updateCenter();
    }

    void updateCenter(){
        m_outCenterData = std::make_shared<PointData>(m_outRect.center());
        if (m_ui) {
            // block signals to avoid infinite loop
            QSignalBlocker blocker(m_ui->sb_center_x);
            QSignalBlocker blocker2(m_ui->sb_center_y);
            m_ui->sb_center_x->setValue(m_outRect.center().x());
            m_ui->sb_center_y->setValue(m_outRect.center().y());
        }
        emit dataUpdated(1);
    }

private:
    QWidget* m_widget = nullptr;
    std::unique_ptr<Ui::RectForm> m_ui;
    // in
    // 0
    std::weak_ptr<RectData> m_inRectData;
    //out
    // 0
    QRect m_outRect;
    std::shared_ptr<RectData> m_outRectsData;
    // 1
    std::shared_ptr<PointData> m_outCenterData;

};


#endif //RECTMODEL_H
