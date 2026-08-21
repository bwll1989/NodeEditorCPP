#pragma once

#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtNodes/NodeDelegateModel>
#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes
{
    /** HSV：把输入解析为 [h,s,v]（0–1），输出 RGBA 向量、HSV 向量与 H/S/V。 */
    class HsvColorDataModel final : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QVariant hsv READ hsvValue WRITE setHsvValue NOTIFY hsvChanged)
        Q_PROPERTY(double hue READ hue WRITE setHue NOTIFY hueChanged)
        Q_PROPERTY(double saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)

    public:
        enum InPort : int { HsvIn = 0, HueIn, SatIn, ValIn, InCount };
        enum OutPort : int { RgbaOut = 0, HsvOut, HueOut, SatOut, ValOut, OutCount };

        HsvColorDataModel();
        ~HsvColorDataModel() override;

        QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override;
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;
        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;
        QWidget *embeddedWidget() override { return colorEditorWidget; }
        QJsonObject save() const override;
        void load(const QJsonObject &p) override;

        QVariant hsvValue() const { return floatVectorToList(m_hsv); }
        void setHsvValue(const QVariant &v);

        double hue() const { return m_hsv[0]; }
        void setHue(double h);
        double saturation() const { return m_hsv[1]; }
        void setSaturation(double s);
        double value() const { return m_hsv[2]; }
        void setValue(double v);

    signals:
        void hsvChanged(const QVariant &hsv);
        void hueChanged(double h);
        void saturationChanged(double s);
        void valueChanged(double v);

    protected:
        void afterModelReady() override;

    private:
        QVector<float> rgbaVec() const;
        void setComponent(int index, double value);
        void commitHsv(QVector<float> next, bool syncWidget);
        void syncWidgetFromHsv();
        void notifyAll();

        ColorEditorWidget *colorEditorWidget = new ColorEditorWidget();
        QVector<float> m_hsv{0.0f, 0.0f, 0.0f};

    private Q_SLOTS:
        void onWidgetColorChanged();
        void onGlobalEvent(const GlobalEvent &ev);
    };
}
