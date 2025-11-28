//
// Created by WuBin on 24-10-31.
//
#pragma once
#include "DataTypesExport.h"
#include "QtNodes/NodeData"
#include "QJsonObject"
using QtNodes::NodeData;
using QtNodes::NodeDataType;
namespace NodeDataTypes
{
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

        QVariant value(const QString &key = "default") const ;

        QVariantMap getMap() const ;

        std::unique_ptr<QJsonObject> json() const ;

    private:
        QVariantMap NodeValues;
    };
}
