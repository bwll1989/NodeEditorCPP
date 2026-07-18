#include "VecData.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <cmath>

using namespace NodeDataTypes;

namespace {
float valueOrZero(const QVector<float> &values, int index)
{
    return (index >= 0 && index < values.size()) ? values[index] : 0.0f;
}
}

VecData::VecData() = default;

VecData::VecData(int size)
{
    if (size > 0) {
        m_values.fill(0.0f, size);
    }
}

VecData::VecData(QVector<float> values)
    : m_values(std::move(values))
{
}

VecData::VecData(const QVector2D &v)
    : m_values{v.x(), v.y()}
{
}

VecData::VecData(const QVector3D &v)
    : m_values{v.x(), v.y(), v.z()}
{
}

VecData::VecData(const QVector4D &v)
    : m_values{v.x(), v.y(), v.z(), v.w()}
{
}

VecData::VecData(int size, const QVector4D &v)
{
    if (size <= 0) {
        return;
    }
    m_values.resize(size);
    const float src[4] = {v.x(), v.y(), v.z(), v.w()};
    for (int i = 0; i < size; ++i) {
        m_values[i] = (i < 4) ? src[i] : 0.0f;
    }
}

VecData::VecData(const QVariant &payload, int preferredSize)
{
    *this = fromVariant(payload, preferredSize);
}

VecData::~VecData() = default;

QtNodes::NodeDataType VecData::type() const
{
    return QtNodes::NodeDataType{
        QStringLiteral("vec"),
        QStringLiteral("Vec")};
}

int VecData::size() const
{
    return m_values.size();
}

int VecData::components() const
{
    return m_values.size();
}

float VecData::at(int index) const
{
    return valueOrZero(m_values, index);
}

float VecData::x() const
{
    return at(0);
}

float VecData::y() const
{
    return at(1);
}

float VecData::z() const
{
    return at(2);
}

float VecData::w() const
{
    return at(3);
}

const QVector<float> &VecData::values() const
{
    return m_values;
}

QVector2D VecData::toVector2D() const
{
    return QVector2D(at(0), at(1));
}

QVector3D VecData::toVector3D() const
{
    return QVector3D(at(0), at(1), at(2));
}

QVector4D VecData::toVector4D() const
{
    return QVector4D(at(0), at(1), at(2), at(3));
}

QVariantList VecData::toVariantList() const
{
    QVariantList list;
    list.reserve(m_values.size());
    for (float v : m_values) {
        list.push_back(v);
    }
    return list;
}

QVariantMap VecData::toVariantMap() const
{
    // 仅命名分量，不在这里塞 default / values，避免和 VariableData 再包一层重复
    QVariantMap map;
    if (m_values.size() >= 1) {
        map.insert(QStringLiteral("x"), m_values[0]);
    }
    if (m_values.size() >= 2) {
        map.insert(QStringLiteral("y"), m_values[1]);
    }
    if (m_values.size() >= 3) {
        map.insert(QStringLiteral("z"), m_values[2]);
    }
    if (m_values.size() >= 4) {
        map.insert(QStringLiteral("w"), m_values[3]);
    }
    return map;
}

QJsonObject VecData::toJsonObject() const
{
    QJsonObject obj;
    if (m_values.size() >= 1) {
        obj.insert(QStringLiteral("x"), m_values[0]);
    }
    if (m_values.size() >= 2) {
        obj.insert(QStringLiteral("y"), m_values[1]);
    }
    if (m_values.size() >= 3) {
        obj.insert(QStringLiteral("z"), m_values[2]);
    }
    if (m_values.size() >= 4) {
        obj.insert(QStringLiteral("w"), m_values[3]);
    }
    return obj;
}

std::shared_ptr<VariableData> VecData::toVariableData() const
{
    // 约定形态：{ default: [..], x, y[, z][, w] }
    // default 给 value() / 通用处理；命名键给 Extract。各只出现一次，不再嵌套 map。
    auto var = std::make_shared<VariableData>(toVariantMap());
    var->insert(QStringLiteral("default"), toVariantList());
    return var;
}

void VecData::resize(int size)
{
    if (size < 0) {
        size = 0;
    }
    const int old = m_values.size();
    m_values.resize(size);
    for (int i = old; i < size; ++i) {
        m_values[i] = 0.0f;
    }
}

void VecData::setComponents(int components)
{
    resize(components);
}

void VecData::setValues(QVector<float> values)
{
    m_values = std::move(values);
}

void VecData::setValue(const QVector2D &v)
{
    m_values = {v.x(), v.y()};
}

void VecData::setValue(const QVector3D &v)
{
    m_values = {v.x(), v.y(), v.z()};
}

void VecData::setValue(const QVector4D &v)
{
    m_values = {v.x(), v.y(), v.z(), v.w()};
}

void VecData::setComponent(int index, float v)
{
    if (index < 0) {
        return;
    }
    if (index >= m_values.size()) {
        resize(index + 1);
    }
    m_values[index] = v;
}

bool VecData::fuzzyEquals(const VecData &other) const
{
    if (m_values.size() != other.m_values.size()) {
        return false;
    }
    for (int i = 0; i < m_values.size(); ++i) {
        if (std::fabs(m_values[i] - other.m_values[i]) > 1e-6f) {
            return false;
        }
    }
    return true;
}

VecData VecData::finalize(QVector<float> values, int preferredSize)
{
    VecData out(std::move(values));
    if (preferredSize > 0) {
        out.resize(preferredSize);
    }
    return out;
}

VecData VecData::fromVariant(const QVariant &payload, int preferredSize)
{
    if (!payload.isValid()) {
        return VecData(preferredSize > 0 ? preferredSize : 0);
    }

    const int typeId = payload.typeId();
    if (typeId == QMetaType::QVector2D) {
        return finalize({payload.value<QVector2D>().x(),
                         payload.value<QVector2D>().y()},
                        preferredSize);
    }
    if (typeId == QMetaType::QVector3D) {
        const QVector3D v = payload.value<QVector3D>();
        return finalize({v.x(), v.y(), v.z()}, preferredSize);
    }
    if (typeId == QMetaType::QVector4D) {
        const QVector4D v = payload.value<QVector4D>();
        return finalize({v.x(), v.y(), v.z(), v.w()}, preferredSize);
    }

    if (typeId == QMetaType::QVariantMap || payload.canConvert<QVariantMap>()) {
        const QVariantMap map = payload.toMap();
        if (map.contains(QStringLiteral("values"))) {
            return fromVariant(map.value(QStringLiteral("values")), preferredSize);
        }
        // VariableData 桥接：default 为分量数组时直接解析
        if (map.contains(QStringLiteral("default"))) {
            const QVariant def = map.value(QStringLiteral("default"));
            if (def.typeId() == QMetaType::QVariantList || def.canConvert<QVariantList>()) {
                return fromVariant(def, preferredSize);
            }
        }

        QVector<float> values;
        const bool hasX = map.contains(QStringLiteral("x")) || map.contains(QStringLiteral("X"));
        const bool hasY = map.contains(QStringLiteral("y")) || map.contains(QStringLiteral("Y"));
        const bool hasZ = map.contains(QStringLiteral("z")) || map.contains(QStringLiteral("Z"));
        const bool hasW = map.contains(QStringLiteral("w")) || map.contains(QStringLiteral("W"));

        if (hasX || hasY || hasZ || hasW) {
            const int natural = hasW ? 4 : (hasZ ? 3 : (hasY || hasX ? 2 : 1));
            values.resize(natural);
            if (natural >= 1) {
                values[0] = float(map.value(QStringLiteral("x"), map.value(QStringLiteral("X"))).toDouble());
            }
            if (natural >= 2) {
                values[1] = float(map.value(QStringLiteral("y"), map.value(QStringLiteral("Y"))).toDouble());
            }
            if (natural >= 3) {
                values[2] = float(map.value(QStringLiteral("z"), map.value(QStringLiteral("Z"))).toDouble());
            }
            if (natural >= 4) {
                values[3] = float(map.value(QStringLiteral("w"), map.value(QStringLiteral("W"))).toDouble());
            }
        }
        return finalize(std::move(values), preferredSize);
    }

    if (typeId == QMetaType::QVariantList || payload.canConvert<QVariantList>()) {
        const QVariantList list = payload.toList();
        QVector<float> values;
        values.reserve(list.size());
        for (const QVariant &item : list) {
            values.push_back(float(item.toDouble()));
        }
        return finalize(std::move(values), preferredSize);
    }

    if (typeId == QMetaType::QString || typeId == QMetaType::QByteArray) {
        const QJsonDocument doc = QJsonDocument::fromJson(payload.toString().trimmed().toUtf8());
        if (doc.isObject()) {
            return fromVariant(doc.object().toVariantMap(), preferredSize);
        }
        if (doc.isArray()) {
            QVariantList list;
            for (const QJsonValue &jv : doc.array()) {
                list.push_back(jv.toVariant());
            }
            return fromVariant(list, preferredSize);
        }
    }

    if (payload.canConvert<double>()) {
        const float f = float(payload.toDouble());
        if (preferredSize > 0) {
            QVector<float> values(preferredSize, f);
            return VecData(std::move(values));
        }
        return VecData(QVector<float>{f});
    }

    return VecData(preferredSize > 0 ? preferredSize : 0);
}

VecData VecData::fromVariableData(const std::shared_ptr<VariableData> &varData, int preferredSize)
{
    if (!varData) {
        return VecData(preferredSize > 0 ? preferredSize : 0);
    }
    QVariant payload = varData->value(QStringLiteral("default"));
    if (!payload.isValid()) {
        const QVariantMap map = varData->getMap();
        if (map.contains(QStringLiteral("values"))
            || map.contains(QStringLiteral("x")) || map.contains(QStringLiteral("y"))
            || map.contains(QStringLiteral("z")) || map.contains(QStringLiteral("w"))) {
            payload = QVariant::fromValue(map);
        }
    }
    return fromVariant(payload, preferredSize);
}
