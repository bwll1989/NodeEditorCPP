#include "ColorDataModel.hpp"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>
#include <QtGlobal>

using namespace Nodes;
using namespace NodeDataTypes;

ColorDataModel::ColorDataModel()
{
    InPortCount = InCount;
    OutPortCount = OutCount;
    CaptionVisible = true;
    Caption = QStringLiteral("RGBA");
    WidgetEmbeddable = false;
    Resizable = false;
    connect(colorEditorWidget, &ColorEditorWidget::colorChanged,
            this, &ColorDataModel::onWidgetColorChanged);
    syncWidgetFromRgba();

    const char *members[] = {"rgba", "red", "green", "blue", "alpha"};
    const char *addrs[] = {"/rgba", "/red", "/green", "/blue", "/alpha"};
    for (int i = 0; i < 5; ++i) {
        NodeDelegateModel::ExternalBinding b;
        b.member = QString::fromLatin1(members[i]);
        AbstractDelegateModel::registerExternalBinding(QString::fromLatin1(addrs[i]), this, b);
    }
}

ColorDataModel::~ColorDataModel()
{
    if (colorEditorWidget) {
        colorEditorWidget->setParent(nullptr);
        colorEditorWidget->deleteLater();
    }
}

QtNodes::NodeDataType ColorDataModel::dataType(QtNodes::PortType, QtNodes::PortIndex) const
{
    return VariableData().type();
}

QString ColorDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        static const char *inNames[] = {"RGBA", "R", "G", "B", "A"};
        if (portIndex >= 0 && portIndex < InCount) {
            return QString::fromLatin1(inNames[portIndex]);
        }
    } else if (portType == QtNodes::PortType::Out) {
        static const char *outNames[] = {"HSV", "RGBA", "R", "G", "B"};
        if (portIndex >= 0 && portIndex < OutCount) {
            return QString::fromLatin1(outNames[portIndex]);
        }
    }
    return {};
}

QVector<float> ColorDataModel::hsvVec() const
{
    QVector<float> hsv = hsvFromRgb(m_rgba[0], m_rgba[1], m_rgba[2]);
    if (hsv[1] < 1e-6f || hsv[2] < 1e-6f) {
        hsv[0] = m_hue;
    }
    return hsv;
}

std::shared_ptr<QtNodes::NodeData> ColorDataModel::outData(QtNodes::PortIndex port)
{
    switch (port) {
    case HsvOut:
        return std::make_shared<VariableData>(floatVectorToList(hsvVec()));
    case RgbaOut:
        return std::make_shared<VariableData>(rgbaValue());
    case RedOut:
    case GreenOut:
    case BlueOut:
        return std::make_shared<VariableData>(double(m_rgba[port - RedOut]));
    default:
        return std::make_shared<VariableData>(rgbaValue());
    }
}

void ColorDataModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port)
{
    if (!nodeData) {
        return;
    }
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) {
        return;
    }
    if (port == RgbaIn) {
        commitRgba(v->asFloats(4), true);
        return;
    }
    if (port >= RedIn && port <= AlphaIn) {
        setComponent(port - RedIn, v->asNumber());
    }
}

QJsonObject ColorDataModel::save() const
{
    QJsonObject values;
    values[QStringLiteral("rgba")] = QJsonArray::fromVariantList(floatVectorToList(m_rgba));
    values[QStringLiteral("hue")] = double(m_hue);
    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson[QStringLiteral("values")] = values;
    return modelJson;
}

void ColorDataModel::load(const QJsonObject &p)
{
    const QJsonValue v = p[QStringLiteral("values")];
    if (!v.isObject()) {
        return;
    }
    const QJsonObject values = v.toObject();
    const QJsonValue rgba = values.value(QStringLiteral("rgba"));
    if (rgba.isArray()) {
        QVector<float> next = floatVectorFromVariant(rgba.toArray().toVariantList(), 4);
        if (next.size() < 4) {
            next.resize(4);
        }
        m_rgba = std::move(next);
    }
    if (values.contains(QStringLiteral("hue"))) {
        m_hue = float(wrapHueF(values.value(QStringLiteral("hue")).toDouble()));
    } else {
        const QVector<float> hsv = hsvFromRgb(m_rgba[0], m_rgba[1], m_rgba[2]);
        if (hsv[1] >= 1e-6f && hsv[2] >= 1e-6f) {
            m_hue = hsv[0];
        }
    }
    syncWidgetFromRgba();
    notifyAll();
}

void ColorDataModel::setRgbaValue(const QVariant &v)
{
    commitRgba(floatVectorFromVariant(v, 4), true);
}

void ColorDataModel::setRed(double r) { setComponent(0, r); }
void ColorDataModel::setGreen(double g) { setComponent(1, g); }
void ColorDataModel::setBlue(double b) { setComponent(2, b); }
void ColorDataModel::setAlpha(double a) { setComponent(3, a); }

void ColorDataModel::setComponent(int index, double value)
{
    if (index < 0 || index > 3) {
        return;
    }
    if (qAbs(double(m_rgba[index]) - value) < 1e-6) {
        return;
    }
    QVector<float> next = m_rgba;
    next[index] = float(value);
    commitRgba(std::move(next), true);
}

void ColorDataModel::commitRgba(QVector<float> next, bool syncWidget)
{
    if (next.size() < 4) {
        next.resize(4);
    } else if (next.size() > 4) {
        next.resize(4);
    }
    if (floatVectorFuzzyEqual(next, m_rgba)) {
        return;
    }
    m_rgba = std::move(next);
    const QVector<float> hsv = hsvFromRgb(m_rgba[0], m_rgba[1], m_rgba[2]);
    if (hsv[1] >= 1e-6f && hsv[2] >= 1e-6f) {
        m_hue = hsv[0];
    }
    if (syncWidget) {
        syncWidgetFromRgba();
    }
    notifyAll();
}

void ColorDataModel::syncWidgetFromRgba()
{
    const QVector<float> hsv = hsvVec();
    const QSignalBlocker b(colorEditorWidget);
    colorEditorWidget->setHsvF(
        hsv[0], hsv[1], hsv[2],
        qBound(0.0, double(m_rgba[3]), 1.0));
}

void ColorDataModel::notifyAll()
{
    Q_EMIT rgbaChanged(rgbaValue());
    Q_EMIT redChanged(m_rgba[0]);
    Q_EMIT greenChanged(m_rgba[1]);
    Q_EMIT blueChanged(m_rgba[2]);
    Q_EMIT alphaChanged(m_rgba[3]);
    Q_EMIT dataUpdated(HsvOut);
    Q_EMIT dataUpdated(RgbaOut);
    Q_EMIT dataUpdated(RedOut);
    Q_EMIT dataUpdated(GreenOut);
    Q_EMIT dataUpdated(BlueOut);
}

void ColorDataModel::onWidgetColorChanged()
{
    const auto c = colorEditorWidget->getColor();
    const float hue = float(colorEditorWidget->hueF());
    QVector<float> next = {
        float(c.redF()), float(c.greenF()), float(c.blueF()), float(c.alphaF())
    };
    const bool rgbaEqual = floatVectorFuzzyEqual(next, m_rgba);
    const bool hueEqual = qAbs(double(hue) - double(m_hue)) < 1e-6;
    if (rgbaEqual && hueEqual) {
        return;
    }
    m_rgba = std::move(next);
    m_hue = hue;
    notifyAll();
}

void ColorDataModel::afterModelReady()
{
    const char *addrs[] = {"/rgba", "/red", "/green", "/blue", "/alpha"};
    for (const char *addr : addrs) {
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress(QString::fromLatin1(addr)), this, SLOT(onGlobalEvent(GlobalEvent)));
    }
}

void ColorDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }
    const QString addr = ev.address;
    if (addr == makeFullOscAddress(QStringLiteral("/rgba"))) {
        setRgbaValue(ev.payload);
    } else if (addr == makeFullOscAddress(QStringLiteral("/red"))) {
        setRed(ev.payload.toDouble());
    } else if (addr == makeFullOscAddress(QStringLiteral("/green"))) {
        setGreen(ev.payload.toDouble());
    } else if (addr == makeFullOscAddress(QStringLiteral("/blue"))) {
        setBlue(ev.payload.toDouble());
    } else if (addr == makeFullOscAddress(QStringLiteral("/alpha"))) {
        setAlpha(ev.payload.toDouble());
    }
}
