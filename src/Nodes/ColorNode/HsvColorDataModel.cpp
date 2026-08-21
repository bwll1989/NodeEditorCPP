#include "HsvColorDataModel.hpp"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>
#include <QtGlobal>

using namespace Nodes;
using namespace NodeDataTypes;

HsvColorDataModel::HsvColorDataModel()
{
    InPortCount = InCount;
    OutPortCount = OutCount;
    CaptionVisible = true;
    Caption = QStringLiteral("HSV");
    WidgetEmbeddable = false;
    Resizable = false;
    connect(colorEditorWidget, &ColorEditorWidget::colorChanged,
            this, &HsvColorDataModel::onWidgetColorChanged);
    syncWidgetFromHsv();

    const char *members[] = {"hsv", "hue", "saturation", "value"};
    const char *addrs[] = {"/hsv", "/hue", "/saturation", "/value"};
    for (int i = 0; i < 4; ++i) {
        NodeDelegateModel::ExternalBinding b;
        b.member = QString::fromLatin1(members[i]);
        AbstractDelegateModel::registerExternalBinding(QString::fromLatin1(addrs[i]), this, b);
    }
}

HsvColorDataModel::~HsvColorDataModel()
{
    if (colorEditorWidget) {
        colorEditorWidget->setParent(nullptr);
        colorEditorWidget->deleteLater();
    }
}

QtNodes::NodeDataType HsvColorDataModel::dataType(QtNodes::PortType, QtNodes::PortIndex) const
{
    return VariableData().type();
}

QString HsvColorDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        static const char *inNames[] = {"HSV", "H", "S", "V"};
        if (portIndex >= 0 && portIndex < InCount) {
            return QString::fromLatin1(inNames[portIndex]);
        }
    } else if (portType == QtNodes::PortType::Out) {
        static const char *outNames[] = {"RGBA", "HSV", "H", "S", "V"};
        if (portIndex >= 0 && portIndex < OutCount) {
            return QString::fromLatin1(outNames[portIndex]);
        }
    }
    return {};
}

QVector<float> HsvColorDataModel::rgbaVec() const
{
    const QVector<float> rgb = rgbFromHsv(m_hsv[0], m_hsv[1], m_hsv[2]);
    return {rgb[0], rgb[1], rgb[2], 1.0f};
}

std::shared_ptr<QtNodes::NodeData> HsvColorDataModel::outData(QtNodes::PortIndex port)
{
    switch (port) {
    case RgbaOut:
        return std::make_shared<VariableData>(floatVectorToList(rgbaVec()));
    case HsvOut:
        return std::make_shared<VariableData>(hsvValue());
    case HueOut:
    case SatOut:
    case ValOut:
        return std::make_shared<VariableData>(double(m_hsv[port - HueOut]));
    default:
        return std::make_shared<VariableData>(hsvValue());
    }
}

void HsvColorDataModel::setInData(const std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port)
{
    if (!nodeData) {
        return;
    }
    auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!v) {
        return;
    }
    if (port == HsvIn) {
        commitHsv(v->asFloats(3), true);
        return;
    }
    if (port >= HueIn && port <= ValIn) {
        setComponent(port - HueIn, v->asNumber());
    }
}

QJsonObject HsvColorDataModel::save() const
{
    QJsonObject values;
    values[QStringLiteral("hsv")] = QJsonArray::fromVariantList(floatVectorToList(m_hsv));
    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson[QStringLiteral("values")] = values;
    return modelJson;
}

void HsvColorDataModel::load(const QJsonObject &p)
{
    const QJsonValue v = p[QStringLiteral("values")];
    if (v.isObject()) {
        const QJsonValue hsv = v.toObject().value(QStringLiteral("hsv"));
        if (hsv.isArray()) {
            QVector<float> next = floatVectorFromVariant(hsv.toArray().toVariantList(), 3);
            if (next.size() < 3) {
                next.resize(3);
            }
            m_hsv = std::move(next);
            m_hsv[0] = float(wrapHueF(m_hsv[0]));
        }
    }
    syncWidgetFromHsv();
    notifyAll();
}

void HsvColorDataModel::setHsvValue(const QVariant &v)
{
    commitHsv(floatVectorFromVariant(v, 3), true);
}

void HsvColorDataModel::setHue(double h) { setComponent(0, h); }
void HsvColorDataModel::setSaturation(double s) { setComponent(1, s); }
void HsvColorDataModel::setValue(double v) { setComponent(2, v); }

void HsvColorDataModel::setComponent(int index, double value)
{
    if (index < 0 || index > 2) {
        return;
    }
    if (qAbs(double(m_hsv[index]) - value) < 1e-6) {
        return;
    }
    QVector<float> next = m_hsv;
    next[index] = float(value);
    commitHsv(std::move(next), true);
}

void HsvColorDataModel::commitHsv(QVector<float> next, bool syncWidget)
{
    if (next.size() < 3) {
        next.resize(3);
    } else if (next.size() > 3) {
        next.resize(3);
    }
    next[0] = float(wrapHueF(next[0]));
    next[1] = qBound(0.0f, next[1], 1.0f);
    next[2] = qBound(0.0f, next[2], 1.0f);
    if (floatVectorFuzzyEqual(next, m_hsv)) {
        return;
    }
    m_hsv = std::move(next);
    if (syncWidget) {
        syncWidgetFromHsv();
    }
    notifyAll();
}

void HsvColorDataModel::syncWidgetFromHsv()
{
    const QSignalBlocker b(colorEditorWidget);
    colorEditorWidget->setHsvF(
        m_hsv[0], m_hsv[1], m_hsv[2], 1.0);
}

void HsvColorDataModel::notifyAll()
{
    Q_EMIT hsvChanged(hsvValue());
    Q_EMIT hueChanged(m_hsv[0]);
    Q_EMIT saturationChanged(m_hsv[1]);
    Q_EMIT valueChanged(m_hsv[2]);
    Q_EMIT dataUpdated(RgbaOut);
    Q_EMIT dataUpdated(HsvOut);
    Q_EMIT dataUpdated(HueOut);
    Q_EMIT dataUpdated(SatOut);
    Q_EMIT dataUpdated(ValOut);
}

void HsvColorDataModel::onWidgetColorChanged()
{
    commitHsv({
        float(colorEditorWidget->hueF()),
        float(colorEditorWidget->saturationF()),
        float(colorEditorWidget->valueF())
    }, false);
}

void HsvColorDataModel::afterModelReady()
{
    const char *addrs[] = {"/hsv", "/hue", "/saturation", "/value"};
    for (const char *addr : addrs) {
        GlobalEventBus::instance()->subscribe(
            makeFullOscAddress(QString::fromLatin1(addr)), this, SLOT(onGlobalEvent(GlobalEvent)));
    }
}

void HsvColorDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }
    const QString addr = ev.address;
    if (addr == makeFullOscAddress(QStringLiteral("/hsv"))) {
        setHsvValue(ev.payload);
    } else if (addr == makeFullOscAddress(QStringLiteral("/hue"))) {
        setHue(ev.payload.toDouble());
    } else if (addr == makeFullOscAddress(QStringLiteral("/saturation"))) {
        setSaturation(ev.payload.toDouble());
    } else if (addr == makeFullOscAddress(QStringLiteral("/value"))) {
        setValue(ev.payload.toDouble());
    }
}
