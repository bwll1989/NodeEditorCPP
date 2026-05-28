#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "ColorInterface.hpp"
#include "Common/DataTypes/NodeDataList.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
using namespace NodeDataTypes;
using namespace Nodes;

struct GlobalEvent;

namespace Nodes
{
    class ColorFDataModel final : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int red READ red WRITE setRed NOTIFY redChanged)
        Q_PROPERTY(int green READ green WRITE setGreen NOTIFY greenChanged)
        Q_PROPERTY(int blue READ blue WRITE setBlue NOTIFY blueChanged)
        Q_PROPERTY(int alpha READ alpha WRITE setAlpha NOTIFY alphaChanged)
        Q_PROPERTY(int hue READ hue WRITE setHue NOTIFY hueChanged)
        Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
        Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

    public:
        ColorFDataModel();

        ~ColorFDataModel() override;

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override;

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override;

        QWidget *embeddedWidget() override { return colorEditorWidget; }

        QJsonObject save() const override;

        void load(const QJsonObject &p) override;

        int red() const { return m_color.red(); }
        void setRed(int r);

        int green() const { return m_color.green(); }
        void setGreen(int g);

        int blue() const { return m_color.blue(); }
        void setBlue(int b);

        int alpha() const { return m_color.alpha(); }
        void setAlpha(int a);

        int hue() const { return m_color.hue(); }
        void setHue(int h);

        int saturation() const { return m_color.saturation(); }
        void setSaturation(int s);

        int value() const { return m_color.value(); }
        void setValue(int v);

    public Q_SLOTS:
        void toggleEditorMode();

    signals:
        void redChanged(int r);
        void greenChanged(int g);
        void blueChanged(int b);
        void alphaChanged(int a);
        void hueChanged(int h);
        void saturationChanged(int s);
        void valueChanged(int v);

    protected:
        void afterModelReady() override;

    private:
        // ImageConstInterface *widget = new ImageConstInterface();
        ColorEditorWidget *colorEditorWidget=new ColorEditorWidget();
        QColor m_color = QColor(0,0,0,255);

        void updateDisplay();
        void notifyAllPropertiesChanged();

    private Q_SLOTS:
        void onColorChanged(const QColor& c);
        void onGlobalEvent(const GlobalEvent& ev);
    };
}