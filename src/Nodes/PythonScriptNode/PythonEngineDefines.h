

#pragma once

#include <QtCore/QObject>
#include <QVariantMap>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QString>
#include <QStringList>
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/cast.h"

namespace py = pybind11;

namespace PythonEngineDefines {
// 前置声明
static py::object variantToPyObject(const QVariant& variant);
static py::dict variantMapToPyDict(const QVariantMap& map);
static QVariantMap pyObjectToVariantMap(const py::object& pyObj);
static QVariant pyObjectToVariant(const py::object& pyObj);
/**
 * @brief 将QVariantMap转换为Python字典对象
 * @param map Qt变体映射
 * @return Python字典对象
 */
static py::dict variantMapToPyDict(const QVariantMap& map) {
    py::dict result;
    
    for (auto it = map.begin(); it != map.end(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();
        
        // 根据QVariant类型转换为对应的Python类型
        switch (value.typeId()) {
            case QMetaType::Type::Bool:
                result[key.toStdString().c_str()] = value.toBool();
                break;
            case QMetaType::Type::Int:
                result[key.toStdString().c_str()] = value.toInt();
                break;
            case QMetaType::Type::UInt:
                result[key.toStdString().c_str()] = value.toUInt();
                break;
            case QMetaType::Type::LongLong:
                result[key.toStdString().c_str()] = value.toLongLong();
                break;
            case QMetaType::Type::ULongLong:
                result[key.toStdString().c_str()] = value.toULongLong();
                break;
            case QMetaType::Type::Double:
                result[key.toStdString().c_str()] = value.toDouble();
                break;
            case QMetaType::Type::QString:
                result[key.toStdString().c_str()] = value.toString().toStdString();
                break;
            case QMetaType::Type::QStringList: {
                QStringList strList = value.toStringList();
                py::list pyList;
                for (const QString& str : strList) {
                    pyList.append(str.toStdString());
                }
                result[key.toStdString().c_str()] = pyList;
                break;
            }
            case QMetaType::Type::QVariantList: {
                QVariantList varList = value.toList();
                py::list pyList;
                for (const QVariant& var : varList) {
                    // 递归处理列表中的复杂类型
                    if (var.typeId() == QMetaType::Type::QVariantMap) {
                        pyList.append(variantMapToPyDict(var.toMap()));
                    } else {
                        pyList.append(variantToPyObject(var));
                    }
                }
                result[key.toStdString().c_str()] = pyList;
                break;
            }
            case QMetaType::Type::QVariantMap: {
                QVariantMap varMap = value.toMap();
                result[key.toStdString().c_str()] = variantMapToPyDict(varMap);
                break;
            }
            default:
                // 其他类型转换为字符串
                result[key.toStdString().c_str()] = value.toString().toStdString();
                break;
        }
    }
    
    return result;
}

/**
 * @brief 将单个QVariant转换为Python对象
 * @param variant Qt变体
 * @return Python对象
 */
static py::object variantToPyObject(const QVariant& variant) {
    switch (variant.typeId()) {
        case QMetaType::Type::Bool:
            return py::bool_(variant.toBool());
        case QMetaType::Type::Int:
            return py::int_(variant.toInt());
        case QMetaType::Type::UInt:
            return py::int_(variant.toUInt());
        case QMetaType::Type::LongLong:
            return py::int_(variant.toLongLong());
        case QMetaType::Type::ULongLong:
            return py::int_(variant.toULongLong());
        case QMetaType::Type::Double:
            return py::float_(variant.toDouble());
        case QMetaType::Type::QString:
            return py::str(variant.toString().toStdString());
        case QMetaType::Type::QStringList: {
            QStringList strList = variant.toStringList();
            py::list pyList;
            for (const QString& str : strList) {
                pyList.append(py::str(str.toStdString()));
            }
            return pyList;
        }
        case QMetaType::Type::QVariantList: {
            QVariantList varList = variant.toList();
            py::list pyList;
            for (const QVariant& var : varList) {
                pyList.append(variantToPyObject(var));
            }
            return pyList;
        }
        case QMetaType::Type::QVariantMap:
            return variantMapToPyDict(variant.toMap());
        default:
            return py::cast(variant.toString().toStdString());
    }
}

/**
 * @brief 将Python对象转换为QVariantMap
 * @param pyObj Python对象
 * @return Qt变体映射
 */
static QVariantMap pyObjectToVariantMap(const py::object& pyObj) {
    QVariantMap result;
    
    try {
        if (py::isinstance<py::dict>(pyObj)) {
            // 处理Python字典
            py::dict pyDict = pyObj.cast<py::dict>();
            for (auto item : pyDict) {
                QString key = QString::fromStdString(py::str(item.first).cast<std::string>());
                QVariant value = pyObjectToVariant(item.second.cast<py::object>());
                result[key] = value;
            }
        } else {
            // 如果不是字典，使用默认键存储值
            result["default"] = pyObjectToVariant(pyObj);
        }
    } catch (const std::exception& e) {
        qWarning() << "Python对象转换为QVariantMap失败:" << e.what();
        result["error"] = QString("转换失败: %1").arg(e.what());
    }
    
    return result;
}

/**
 * @brief 将Python对象转换为QVariant
 * @param pyObj Python对象
 * @return Qt变体
 */
static QVariant pyObjectToVariant(const py::object& pyObj) {
    try {
        // 检查Python对象类型并转换为对应的QVariant
        if (pyObj.is_none()) {
            return QVariant();
        } else if (py::isinstance<py::bool_>(pyObj)) {
            return QVariant(pyObj.cast<bool>());
        } else if (py::isinstance<py::int_>(pyObj)) {
            return QVariant(pyObj.cast<long long>());
        } else if (py::isinstance<py::float_>(pyObj)) {
            return QVariant(pyObj.cast<double>());
        } else if (py::isinstance<py::str>(pyObj)) {
            return QVariant(QString::fromStdString(pyObj.cast<std::string>()));
        } else if (py::isinstance<py::list>(pyObj) || py::isinstance<py::tuple>(pyObj)) {
            // 处理Python列表或元组
            py::list pyList = pyObj.cast<py::list>();
            QVariantList varList;
            for (auto item : pyList) {
                varList.append(pyObjectToVariant(item.cast<py::object>()));
            }
            return QVariant(varList);
        } else if (py::isinstance<py::dict>(pyObj)) {
            // 处理Python字典
            return QVariant(pyObjectToVariantMap(pyObj));
        } else {
            // 其他类型尝试转换为字符串
            return QVariant(QString::fromStdString(py::str(pyObj).cast<std::string>()));
        }
    } catch (const std::exception& e) {
        qWarning() << "Python对象转换为QVariant失败:" << e.what();
        return QVariant(QString("转换失败: %1").arg(e.what()));
    }
}

/**
 * @brief 将QVariant转换为Python对象（便捷函数）
 * @param variant Qt变体
 * @return Python对象
 */
static py::object qvariantToPyObject(const QVariant& variant) {
    return variantToPyObject(variant);
}

/**
 * @brief 将QVariantMap转换为Python字典（便捷函数）
 * @param map Qt变体映射
 * @return Python字典
 */
static py::dict qvariantMapToPyDict(const QVariantMap& map) {
    return variantMapToPyDict(map);
}

} // namespace PythonEngineDefines

