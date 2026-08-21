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
    /** RGBA：把输入解析为 [r,g,b,a]，输出 HSV 向量、RGBA 向量与 R/G/B。 */
    class ColorDataModel final : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QVariant rgba READ rgbaValue WRITE setRgbaValue NOTIFY rgbaChanged)
        Q_PROPERTY(double red READ red WRITE setRed NOTIFY redChanged)
        Q_PROPERTY(double green READ green WRITE setGreen NOTIFY greenChanged)
        Q_PROPERTY(double blue READ blue WRITE setBlue NOTIFY blueChanged)
        Q_PROPERTY(double alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)

    public:
        enum InPort : int { RgbaIn = 0, RedIn, GreenIn, BlueIn, AlphaIn, InCount };
        enum OutPort : int { HsvOut = 0, RgbaOut, RedOut, GreenOut, BlueOut, OutCount };

        ColorDataModel();
        ~ColorDataModel() override;

        QtNodes::NodeDataType dataType(QtNodes::PortType, QtNodes::PortIndex) const override;
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;
        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;
        QWidget *embeddedWidget() override { return colorEditorWidget; }
        QJsonObject save() const override;
        void load(const QJsonObject &p) override;

        QVariant rgbaValue() const { return floatVectorToList(m_rgba); }
        void setRgbaValue(const QVariant &v);

        double red() const { return m_rgba[0]; }
        void setRed(double r);
        double green() const { return m_rgba[1]; }
        void setGreen(double g);
        double blue() const { return m_rgba[2]; }
        void setBlue(double b);
        double alpha() const { return m_rgba[3]; }
        void setAlpha(double a);

    signals:
        void rgbaChanged(const QVariant &rgba);
        void redChanged(double r);
        void greenChanged(double g);
        void blueChanged(double b);
        void alphaChanged(double a);

    protected:
        void afterModelReady() override;

    private:
        QVector<float> hsvVec() const;
        void setComponent(int index, double value);
        void commitRgba(QVector<float> next, bool syncWidget);
        void syncWidgetFromRgba();
        void notifyAll();

        ColorEditorWidget *colorEditorWidget = new ColorEditorWidget();
        QVector<float> m_rgba{0.0f, 0.0f, 0.0f, 1.0f};
        float m_hue = 0.0f;

    private Q_SLOTS:
        void onWidgetColorChanged();
        void onGlobalEvent(const GlobalEvent &ev);
    };
}
