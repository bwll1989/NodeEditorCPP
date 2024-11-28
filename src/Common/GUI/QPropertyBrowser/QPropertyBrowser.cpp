//
// Created by WuBin on 24-11-4.
//

#include "QPropertyBrowser.h"

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
    VaribaleItem = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                         QLatin1String(" Variable"));
    m_propertyBrowser->addProperty(VaribaleItem);

    NodeItem = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                     QLatin1String("Property"));
    m_propertyBrowser->addProperty(NodeItem);
    connect(m_propertyManager, SIGNAL(valueChanged(QtProperty *, const QVariant &)),
               this, SLOT(onNodeItemValueChanged(QtProperty *, const QVariant &)));

    // connect(m_propertyManager, &QtVariantPropertyManager::valueChanged,
    //         this, &QPropertyBrowser::onNodeItemValueChanged);
    // 此种方式会出现找不到信号，原因未知
}

void QPropertyBrowser::addFixedProperties(int propertyType, const QString &name,const QVariant &value) {

    QtVariantProperty* nameProperty = m_propertyManager->addProperty(propertyType, name);
    nameProperty->setValue(value);
    NodeItem->addSubProperty(nameProperty);
    m_propertyManager->setAttribute(nameProperty, "readOnly", false);

}
// 根据 QVariantMap 构建属性
void QPropertyBrowser::buildPropertiesFromMap(const QVariantMap& map) {
    // m_propertyBrowser->clear();  // 清空当前属性
    addPropertiesFromMap(map, VaribaleItem);  // 递归添加属性
}

void QPropertyBrowser::addPropertiesFromMap(const QVariantMap& map, QtVariantProperty* parentProperty,bool readOnly) {
    if(parentProperty) {
        qDeleteAll(parentProperty->subProperties());
    }

    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            const QString& key = it.key();
            const QVariant& value = it.value();
            QtVariantProperty* property = nullptr;
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
                    QSize size = value.toSize();
                    property->setValue(size);
                    break;
                }
                case QMetaType::QRect: {
                    property = m_propertyManager->addProperty(QMetaType::QRect, key);
                    QRect rect = value.toRect();
                    property->setValue(rect);
                    break;
                }
                case QMetaType::QRectF: {
                    property = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), key);
                    QRectF rectF = value.toRectF();
                    property->setValue(rectF);
                    break;
                }
                case QMetaType::QPoint: {
                    property = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), key);
                    QPoint point = value.toPoint();
                    property->setValue(point);
                    break;
                }
                case QMetaType::QVariantMap: {
                    // 使用 QVariant::Invalid 表示组属性
                    property = m_propertyManager->addProperty(QtVariantPropertyManager::groupTypeId(), key);
                    QVariantMap childMap = value.toMap();
                    addPropertiesFromMap(childMap, property, readOnly);  // 递归添加子属性
                    break;
                }
                case QMetaType::QByteArray: {
                    property = m_propertyManager->addProperty(QMetaType::QString, key);
                    property->setValue(value.toByteArray());
                    break;
                }
                default:
                    // 对于不支持的类型，显示数据类型信息
                    property = m_propertyManager->addProperty(QMetaType::QString, key);
                    property->setValue(QString("Unsupported Type: %1").arg(value.typeName()));
                    break;
            }

            if (property && readOnly) {
                // 设置为只读
                m_propertyManager->setAttribute(property, "readOnly", true);
            }

            if (parentProperty) {
                parentProperty->addSubProperty(property);
            } else {
                m_propertyBrowser->addProperty(property);
            }
        }
    }

QVariantMap QPropertyBrowser::exportToMap() const {
    QVariantMap map;
    for (QtProperty* property : NodeItem->subProperties()) {
        map.insert(property->propertyName(), exportProperty(property));
    }
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
    if (NodeItem->subProperties().contains(property)) {
        // qDebug() << "NodeItem property changed:" << property->propertyName() << "New value:" << value.toString();

        // 这里可以执行具体的逻辑处理，比如发出自定义信号或更新界面等
        emit nodeItemValueChanged(property->propertyName(), value);
    }
}

QVariant QPropertyBrowser::getProperties( const QString &name) {
    QVariant result;
    for (QtProperty *property : NodeItem->subProperties()) {
        // 检查子属性的名称是否匹配
        if (property->propertyName() == name) {
            // 将属性的值设置为传入的 value
            QtVariantProperty *variantProperty = dynamic_cast<QtVariantProperty*>(property);
            result = variantProperty->value();

        }

    }
    return result;
}

void QPropertyBrowser::setProperty(const QString& name, const QVariant& val) {
    QtProperty* existingProperty = nullptr;
    for (QtProperty* child : NodeItem->subProperties()) {
        if (child->propertyName() == name) {
            existingProperty = child;
            break;
        }
    }

    if (existingProperty) {
        if (QtVariantProperty* variantProperty = dynamic_cast<QtVariantProperty*>(existingProperty)) {
            variantProperty->setValue(val);
            emit nodeItemValueChanged(name,val);
        } else {
            qWarning() << "Existing property is not a QtVariantProperty";
        }
    } else {
        QtVariantProperty* newProperty = m_propertyManager->addProperty(val.typeId(), name);
        newProperty->setValue(val);
        NodeItem->addSubProperty(newProperty);
        emit nodeItemValueChanged(name,val);
    }
}
