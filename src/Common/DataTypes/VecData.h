#pragma once

#include <QtNodes/NodeData>
#include <QJsonObject>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <memory>

#include "DataTypesExport.h"
#include "VariableData.h"

namespace NodeDataTypes
{
/**
 * @brief 通用向量端口数据类型
 *
 * 端口匹配统一 id "vec"，与具体维数无关；分量存放在 QVector<float>，维数不限。
 * 使用方按需截取：toVector2D / toVector3D / toVector4D / at(i)，越界按 0。
 * VariableData 桥接（toVariableData）：{ "default": [...], "x","y"[, "z"][, "w"] }
 */
class DATATYPES_EXPORT VecData final : public QtNodes::NodeData
{
public:
    VecData();
    /** 长度为 size 的零向量；size <= 0 则为空 */
    explicit VecData(int size);
    explicit VecData(QVector<float> values);
    explicit VecData(const QVector2D &v);
    explicit VecData(const QVector3D &v);
    explicit VecData(const QVector4D &v);
    /** 取 v 的前 size 个分量，不足补 0 */
    VecData(int size, const QVector4D &v);
    /**
     * 从 map / list / QVector*D / JSON 解析。
     * preferredSize > 0 时结果 resize 到该长度（截断或补 0）；否则保留载荷自然长度。
     */
    explicit VecData(const QVariant &payload, int preferredSize = 0);
    ~VecData() override;

    QtNodes::NodeDataType type() const override;

    int size() const;
    /** 同 size()，兼容旧调用 */
    int components() const;

    float at(int index) const;
    float x() const;
    float y() const;
    float z() const;
    float w() const;

    const QVector<float> &values() const;

    /** 按需截取 / 补 0 */
    QVector2D toVector2D() const;
    QVector3D toVector3D() const;
    QVector4D toVector4D() const;
    QVariantList toVariantList() const;
    /** 仅 {x,y[,z][,w]}，不含 default */
    QVariantMap toVariantMap() const;
    QJsonObject toJsonObject() const;

    /**
     * 桥接到 VariableData：{ default: 分量数组, x, y[, z][, w] }，不嵌套重复。
     * 图模型投递到 VariableData 口时会自动调用。
     */
    std::shared_ptr<VariableData> toVariableData() const;

    void resize(int size);
    void setComponents(int components);
    void setValues(QVector<float> values);
    void setValue(const QVector2D &v);
    void setValue(const QVector3D &v);
    void setValue(const QVector4D &v);
    void setComponent(int index, float v);

    bool fuzzyEquals(const VecData &other) const;

    static VecData fromVariant(const QVariant &payload, int preferredSize = 0);
    static VecData fromVariableData(const std::shared_ptr<VariableData> &varData,
                                    int preferredSize = 0);

private:
    static VecData finalize(QVector<float> values, int preferredSize);

    QVector<float> m_values;
};
}
