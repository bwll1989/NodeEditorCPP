//
// Created by WuBin on 24-10-31.
//
#pragma once
#include "DataTypesExport.h"
#include "QtNodes/NodeData"
#include "QJsonObject"
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

using QtNodes::NodeData;
using QtNodes::NodeDataType;
namespace NodeDataTypes
{
    /**
     * 通用键值载体。线上 default 保持生产端自然形态（标量或列表）；
     * as* / fromDisplay 仅在消费端按期望解析，不改存储约定。
     */
    class DATATYPES_EXPORT VariableData : public NodeData {
    public:
        VariableData() ;

        explicit VariableData(QVariantMap* val) ;

        explicit VariableData(const QVariantMap &val) ;

        explicit VariableData(const QJsonObject* val);

        explicit VariableData(const QVariant &val) ;

        void insert(const QString &key, const QVariant &value) ;

        NodeDataType type() const override ;

        bool hasKey(const QString &key) const ;
        bool isEmpty() const ;

        QVariant value(const QString &key = QStringLiteral("default")) const ;

        /** 整份键值表（消费端；与 asNumber / asFloats 等命名统一） */
        QVariantMap asMap() const;

        std::unique_ptr<QJsonObject> json() const ;

        QString toJsonString() const;

        QStringList keys() const;

        /** default（或指定键）是否为列表载荷 */
        bool isList(const QString &key = QStringLiteral("default")) const;

        /** 期望单个数值：列表取首元，否则按 double 解析 */
        double asNumber(const QString &key = QStringLiteral("default"),
                        double fallback = 0.0) const;

        /** 期望整数：同 asNumber，再转为 int（截断向零，与 QVariant::toInt 一致） */
        int asInt(const QString &key = QStringLiteral("default"),
                  int fallback = 0) const;

        /** 期望布尔：列表取首元 */
        bool asBool(const QString &key = QStringLiteral("default"),
                    bool fallback = false) const;

        /**
         * 期望 float 向量。size>0 时短补 0、长截断；size<=0 保留自然长度。
         * 兼容列表 / 标量 / 旧几何类型（经 floatVectorFromVariant）。
         */
        QVector<float> asFloats(int size = 0,
                                const QString &key = QStringLiteral("default")) const;

        /** 列表原样；若是标量则包成单元素列表 */
        QVariantList asList(const QString &key = QStringLiteral("default")) const;

        /** 文本口：非列表用 toString；列表用 JSON 数组（同 asDisplay） */
        QString asString(const QString &key = QStringLiteral("default")) const;

        /** 仅 UI 展示：列表 → 紧凑 JSON 数组，否则 toString */
        QString asDisplay(const QString &key = QStringLiteral("default")) const;

        /**
         * UI / OSC 文本框 → QVariant。
         * 以 [ 开头且可解析为 JSON/JS 数组则返回列表，否则尝试 number/bool，再否则字符串。
         */
        static QVariant fromDisplay(const QString &text);

    private:
        QVariantMap NodeValues;
    };
}
