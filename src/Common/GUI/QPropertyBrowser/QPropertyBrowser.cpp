//
// Created by WuBin on 24-11-4.
//

#include "QPropertyBrowser.h"
#include "QJsonValue"
#include "QJsonObject"
#include "QJsonArray"
QPropertyBrowser::QPropertyBrowser(QWidget* parent) : QWidget(parent) {

    // 创建属性浏览器并添加到布局中
    initializeBranches();
    // 设置布局
    layout = new QVBoxLayout(this);
    layout->addWidget(m_propertyBrowser);
    setLayout(layout);
}

void QPropertyBrowser::initializeBranches() {
    // 初始化属性管理器和编辑器工厂
    m_propertyManager = new QtVariantPropertyManager(this);
    m_editorFactory = new QtVariantEditorFactory(this);
    m_propertyBrowser = new QtTreePropertyBrowser(this);
    m_propertyBrowser->setFactoryForManager(m_propertyManager, m_editorFactory);
    // VaribaleItem = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(),
    //                      QLatin1String(" Variable"));
    // m_propertyBrowser->addProperty(VaribaleItem);
    connect(m_propertyManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
               this, SLOT(onNodeItemValueChanged(QtProperty *, const QVariant &)));

    // connect(m_propertyManager, &QtVariantPropertyManager::valueChanged,
    //         this, &QPropertyBrowser::onNodeItemValueChanged);
    // 此种方式会出现找不到信号，原因未知
}


// 根据 QVariantMap 构建属性
void QPropertyBrowser::buildPropertiesFromMap(const QVariantMap& map) {
    m_propertyBrowser->clear();  // 清空当前属性，确保重建而不是叠加
    updatePropertiesFromMap(map);  // 递归添加属性
}
/**
 * @brief 从QVariantMap递归添加属性到属性浏览器
 * @param map 包含属性的QVariantMap
 * @param parent 父属性，用于构建层次结构，默认为nullptr表示顶层属性
 * @param readOnly 属性是否为只读
 */
void QPropertyBrowser::updatePropertiesFromMap(const QVariantMap& map, QtProperty* parent, bool readOnly) {
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();
        QtVariantProperty* property = nullptr;
        
        // 根据值类型创建相应的属性
        switch (value.typeId()) {
            case QMetaType::QString:
                property = m_propertyManager->addProperty(QMetaType::QString, key);
                property->setValue(value.toString());
                break;
            case QMetaType::Int:
                property = m_propertyManager->addProperty(QMetaType::Int, key);
                property->setValue(value.toInt());
                break;
            case QMetaType::Bool:
                property = m_propertyManager->addProperty(QMetaType::Bool, key);
                property->setValue(value.toBool());
                break;
            case QMetaType::Double:
                property = m_propertyManager->addProperty(QMetaType::Double, key);
                property->setValue(value.toDouble());
                break;
            case QMetaType::QSize: {
                property = m_propertyManager->addProperty(QMetaType::QSize, key);
                property->setValue(value.toSize());
                break;
            }
            case QMetaType::QSizeF: {
                property = m_propertyManager->addProperty(QMetaType::QSizeF, key);
                property->setValue(value.toSizeF());
                break;
            }
            case QMetaType::QRect: {
                property = m_propertyManager->addProperty(QMetaType::QRect, key);
                property->setValue(value.toRect());
                break;
            }
            case QMetaType::QRectF: {
                property = m_propertyManager->addProperty(QMetaType::QRectF, key);
                property->setValue(value.toRectF());
                break;
            }
            case QMetaType::QPoint: {
                property = m_propertyManager->addProperty(QMetaType::QPoint, key);
                property->setValue(value.toPoint());
                break;
            }
            case QMetaType::QPointF: {
                property = m_propertyManager->addProperty(QMetaType::QPointF, key);
                property->setValue(value.toPointF());
                break;
            }
            case QMetaType::QVariantList: {
                // 将列表转换为带索引的map { "0": item1, "1": item2... }
                QVariantList list = value.toList();
                QVariantMap listMap;
                for (int i = 0; i < list.size(); ++i) {
                    listMap.insert(QString::number(i), list.at(i));
                }

                // 创建组属性作为父节点
                property = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), key);

                if (parent) {
                    parent->addSubProperty(property);
                } else {
                    m_propertyBrowser->addProperty(property);
                }

                // 递归处理转换后的map
                updatePropertiesFromMap(listMap, property, readOnly);
                continue;
            }
            case QMetaType::QVariantMap: {
                // 使用 QVariant::Invalid 表示组属性
                property = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), key);
                QVariantMap childMap = value.toMap();
                
                // 先添加父属性
                if (parent) {
                    parent->addSubProperty(property);
                } else {
                    m_propertyBrowser->addProperty(property);
                }
                
                // 递归添加子属性，传入当前属性作为父属性
                updatePropertiesFromMap(childMap, property, readOnly);
                
                // 已经处理了属性添加，跳过后面的代码
                continue;
            }
            case QMetaType::QJsonObject: {
                // 改为使用组类型创建父属性
                property = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), key);
                QJsonObject childMap = value.toJsonObject();

                // 先添加父属性
                if (parent) {
                    parent->addSubProperty(property);
                } else {
                    m_propertyBrowser->addProperty(property);
                }

                // 递归添加子属性，传入当前属性作为父属性
                updatePropertiesFromMap(childMap.toVariantMap(), property, readOnly);
                continue;
            }
            case QMetaType::QByteArray: {
                property = m_propertyManager->addProperty(QMetaType::QString, key);
                property->setValue(value.toByteArray().toHex());
                break;
            }
            default:
                // 对于不支持的类型，显示数据类型信息
                property = m_propertyManager->addProperty(QMetaType::QString, key);
                property->setValue(QString("Unsupported Type: %1").arg(value.typeName()));
                break;
        }

        if (property) {
            if (readOnly) {
                // 设置为只读
                m_propertyManager->setAttribute(property, "readOnly", true);
            }
            
            // 根据是否有父属性决定添加方式
            if (parent) {
                parent->addSubProperty(property);
            } else {
                m_propertyBrowser->addProperty(property);
            }
        }
    }
}

QVariantMap QPropertyBrowser::exportToMap() const {
    QVariantMap map;
    for (QtProperty* property : VaribaleItem->subProperties()) {
        map.insert(property->propertyName(), exportProperty(property));
    }
    return map;
}

QVariant QPropertyBrowser::exportProperty(QtProperty* property) const {
    QVariant value;
    if (QtVariantProperty* variantProperty = dynamic_cast<QtVariantProperty*>(property)) {
        if (variantProperty->valueType() == QMetaType::QVariantMap) {
            QVariantMap map;
            for (QtProperty* subProperty : property->subProperties()) {
                map.insert(subProperty->propertyName(), exportProperty(subProperty));
            }
            value = map;
        } else {
            value = variantProperty->value();
        }
    }

    return value;
}

void QPropertyBrowser::onNodeItemValueChanged(QtProperty* property, const QVariant& value) {
    // 检查是否为 m_nodeItem 的子属性


    emit nodeItemValueChanged(property->propertyName(), value);

}

// QVariant QPropertyBrowser::getProperties( const QString &name) {
//     QVariant result;
//     for (QtProperty *property : NodeItem->subProperties()) {
//         // 检查子属性的名称是否匹配
//         if (property->propertyName() == name) {
//             // 将属性的值设置为传入的 value
//             QtVariantProperty *variantProperty = dynamic_cast<QtVariantProperty*>(property);
//             result = variantProperty->value();
//
//         }
//
//     }
//     return result;
// }

