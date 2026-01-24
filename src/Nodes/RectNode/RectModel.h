//
// Created by pablo on 3/9/24.
//

#ifndef RECTMODEL_H
#define RECTMODEL_H


#include <opencv2/core/types.hpp>
#include <QtNodes/NodeDelegateModel>
#include "DataTypes/NodeDataList.hpp"
#include "ui_RectForm.h"
#include <QPointF>
#include <QRectF>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

namespace Ui {
    class RectForm;
}
using namespace NodeDataTypes;
namespace Nodes
{
    class RectModel final : public AbstractDelegateModel {
        Q_OBJECT
        Q_PROPERTY(qreal x READ getX WRITE setX NOTIFY xChanged)
        Q_PROPERTY(qreal y READ getY WRITE setY NOTIFY yChanged)
        Q_PROPERTY(qreal width READ getWidth WRITE setWidth NOTIFY widthChanged)
        Q_PROPERTY(qreal height READ getHeight WRITE setHeight NOTIFY heightChanged)

    public:
        RectModel(){
            m_outRect = QRectF(0, 0, 100, 100); // Default size? Or 0,0,0,0 as before. Before was 0,0,0,0.
            InPortCount =4;
            OutPortCount=4;
            Caption="Rect";
            CaptionVisible=true;
            WidgetEmbeddable= false;
            Resizable=false;
        }

        ~RectModel() override{}

        void afterModelReady() override {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/x"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/y"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/width"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/height"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            Q_UNUSED(portIndex);
            Q_UNUSED(portType);
            return VariableData().type();
        }

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, const QtNodes::PortIndex portIndex) override{
            if (!nodeData) return;
            auto varData = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (!varData) return;

            bool ok = false;
            float val = varData->value().toFloat(&ok);
            if (!ok) return;

            switch (portIndex)
            {
            case 0: setX(val); break;
            case 1: setY(val); break;
            case 2: setWidth(val); break;
            case 3: setHeight(val); break;
            }
        }

        std::shared_ptr<QtNodes::NodeData> outData(const QtNodes::PortIndex port) override{
            switch (port) {
            case 0:
                return std::make_shared<VariableData>(m_outRect);
            case 1:
                return std::make_shared<VariableData>(m_outRect.size());
            case 2:
                return std::make_shared<VariableData>(m_outRect.center());
            case 3:
                return std::make_shared<VariableData>(m_outRect.bottomRight());
            default:
                return std::make_shared<VariableData>();
            }
        }

        QWidget* embeddedWidget() override
        {
            if (!m_widget) {
                m_ui = std::make_unique<Ui::RectForm>();
                m_widget = new QWidget();
                m_ui->setupUi(m_widget);
                AbstractDelegateModel::registerExternalControl("/x",m_ui->sb_x);
                AbstractDelegateModel::registerExternalControl("/y",m_ui->sb_y);
                AbstractDelegateModel::registerExternalControl("/width",m_ui->sb_width);
                AbstractDelegateModel::registerExternalControl("/height",m_ui->sb_height);
                // UI -> Property connections
                connect(m_ui->sb_x, &QLineEdit::editingFinished, this, [this](){
                    setX(m_ui->sb_x->text().toFloat());
                });
                connect(m_ui->sb_y, &QLineEdit::editingFinished, this, [this](){
                    setY(m_ui->sb_y->text().toFloat());
                });
                connect(m_ui->sb_width, &QLineEdit::editingFinished, this, [this](){
                    setWidth(m_ui->sb_width->text().toFloat());
                });
                connect(m_ui->sb_height, &QLineEdit::editingFinished, this, [this](){
                    setHeight(m_ui->sb_height->text().toFloat());
                });

                // Initial sync
                m_ui->sb_x->setText(QString::number(m_outRect.x()));
                m_ui->sb_y->setText(QString::number(m_outRect.y()));
                m_ui->sb_width->setText(QString::number(m_outRect.width()));
                m_ui->sb_height->setText(QString::number(m_outRect.height()));
            }
            return m_widget;
        }


        QString portCaption(QtNodes::PortType port, QtNodes::PortIndex port_index) const override{
            switch (port) {
            case QtNodes::PortType::In:
                switch (port_index) {
                    case 0: return "POS_X";
                    case 1: return "POS_Y";
                    case 2: return "WIDTH";
                    case 3: return "HEIGHT";
                    default: return QString();
                }
            case QtNodes::PortType::Out:
                switch (port_index) {
                    case 0: return "Rect";
                    case 1: return "SIZE";
                    case 2: return "CENTER";
                    case 3: return "BOTTOM_RIGHT";
                    default: return QString();
                }
            default:
                return NodeDelegateModel::portCaption(port, port_index);
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["x"] = m_outRect.x();
            modelJson1["y"] = m_outRect.y();
            modelJson1["width"] = m_outRect.width();
            modelJson1["height"] = m_outRect.height();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["rect"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["rect"];
            if (!v.isUndefined()&&v.isObject()) {
                if (v["x"].isDouble()) setX(v["x"].toDouble());
                else if (v["x"].isString()) setX(v["x"].toString().toDouble());

                if (v["y"].isDouble()) setY(v["y"].toDouble());
                else if (v["y"].isString()) setY(v["y"].toString().toDouble());

                if (v["width"].isDouble()) setWidth(v["width"].toDouble());
                else if (v["width"].isString()) setWidth(v["width"].toString().toDouble());

                if (v["height"].isDouble()) setHeight(v["height"].toDouble());
                else if (v["height"].isString()) setHeight(v["height"].toString().toDouble());
            }
        }

    public: // Getters and Setters
        qreal getX() const { return m_outRect.x(); }
        void setX(qreal value) {
            if (qFuzzyCompare(m_outRect.x(), value)) return;
            m_outRect.moveLeft(value); // Moves the rect, preserving size.
            // Wait, moveLeft changes left and KEEPS size.
            // If I just want to set X and keep everything else... 
            // setRect(x, y, w, h) is explicit.
            m_outRect.setRect(value, m_outRect.y(), m_outRect.width(), m_outRect.height());
            
            if (m_widget && m_ui && m_ui->sb_x->text().toDouble() != value) {
                QSignalBlocker blocker(m_ui->sb_x);
                m_ui->sb_x->setText(QString::number(value));
            }
            emit xChanged(value);
            AbstractDelegateModel::stateFeedBack("/x", value);
            emitDataUpdates();
        }

        qreal getY() const { return m_outRect.y(); }
        void setY(qreal value) {
            if (qFuzzyCompare(m_outRect.y(), value)) return;
            m_outRect.setRect(m_outRect.x(), value, m_outRect.width(), m_outRect.height());
            
            if (m_widget && m_ui && m_ui->sb_y->text().toDouble() != value) {
                QSignalBlocker blocker(m_ui->sb_y);
                m_ui->sb_y->setText(QString::number(value));
            }
            emit yChanged(value);
            AbstractDelegateModel::stateFeedBack("/y", value);
            emitDataUpdates();
        }

        qreal getWidth() const { return m_outRect.width(); }
        void setWidth(qreal value) {
            if (qFuzzyCompare(m_outRect.width(), value)) return;
            m_outRect.setWidth(value);
            
            if (m_widget && m_ui && m_ui->sb_width->text().toDouble() != value) {
                QSignalBlocker blocker(m_ui->sb_width);
                m_ui->sb_width->setText(QString::number(value));
            }
            emit widthChanged(value);
            AbstractDelegateModel::stateFeedBack("/width", value);
            emitDataUpdates();
        }

        qreal getHeight() const { return m_outRect.height(); }
        void setHeight(qreal value) {
            if (qFuzzyCompare(m_outRect.height(), value)) return;
            m_outRect.setHeight(value);
            
            if (m_widget && m_ui && m_ui->sb_height->text().toDouble() != value) {
                QSignalBlocker blocker(m_ui->sb_height);
                m_ui->sb_height->setText(QString::number(value));
            }
            emit heightChanged(value);
            AbstractDelegateModel::stateFeedBack("/height", value);
            emitDataUpdates();
        }

    private:
        void emitDataUpdates() {
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            Q_EMIT dataUpdated(2);
            Q_EMIT dataUpdated(3);
        }

    signals:
        void xChanged(qreal value);
        void yChanged(qreal value);
        void widthChanged(qreal value);
        void heightChanged(qreal value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            
            if (localPath == "x") {
                setX(ev.payload.toDouble());
            } else if (localPath == "y") {
                setY(ev.payload.toDouble());
            } else if (localPath == "width") {
                setWidth(ev.payload.toDouble());
            } else if (localPath == "height") {
                setHeight(ev.payload.toDouble());
            }
        }

    private:
        QWidget* m_widget = nullptr;
        std::unique_ptr<Ui::RectForm> m_ui;
        // in
        // 0
        std::weak_ptr<VariableData> m_inData;
        //out
        // 0
        QRectF m_outRect;
    };
}

#endif //RECTMODEL_H
