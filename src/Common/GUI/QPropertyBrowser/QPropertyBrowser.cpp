//
// Created by WuBin on 24-11-4.
//

#include "QPropertyBrowser.h"

QPropertyBrowser::QPropertyBrowser(QWidget* parent) : QWidget(parent) {
    // 初始化属性管理器和编辑器工厂
    m_propertyManager = new QtVariantPropertyManager(this);
    m_editorFactory = new QtVariantEditorFactory(this);

    // 创建属性浏览器并添加到布局中
    m_propertyBrowser = new QtTreePropertyBrowser(this);
    m_propertyBrowser->setFactoryForManager(m_propertyManager, m_editorFactory);

    // 设置布局
    layout = new QVBoxLayout(this);
    layout->addWidget(m_propertyBrowser);
    setLayout(layout);


}

void QPropertyBrowser::addProperties() {
    // 添加一个字符串属性
    QtVariantProperty* nameProperty = m_propertyManager->addProperty(QVariant::String, "Name");
    nameProperty->setValue("Default Name");
    m_propertyBrowser->addProperty(nameProperty);

    // 添加一个整数属性
    QtVariantProperty* ageProperty = m_propertyManager->addProperty(QVariant::Int, "Age");
    ageProperty->setValue(30);
    ageProperty->setAttribute("minimum", 0);  // 设置最小值
    ageProperty->setAttribute("maximum", 100); // 设置最大值
    m_propertyBrowser->addProperty(ageProperty);

    // 添加一个布尔属性
    QtVariantProperty* isEnabledProperty = m_propertyManager->addProperty(QVariant::Bool, "Is Enabled");
    isEnabledProperty->setValue(true);
    m_propertyBrowser->addProperty(isEnabledProperty);
}
// 根据 QVariantMap 构建属性
void QPropertyBrowser::buildPropertiesFromMap(const QVariantMap& map) {
    m_propertyBrowser->clear();  // 清空当前属性
    addPropertiesFromMap(map, nullptr);  // 递归添加属性
}

void QPropertyBrowser::addPropertiesFromMap(const QVariantMap& map, QtVariantProperty* parentProperty,bool readOnly) {
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            const QString& key = it.key();
            const QVariant& value = it.value();

            QtVariantProperty* property = nullptr;

            switch (value.typeId()) {
                case QVariant::String:
                    property = m_propertyManager->addProperty(QVariant::String, key);
                    property->setValue(value.toString());
                    break;
                case QVariant::Int:
                    property = m_propertyManager->addProperty(QVariant::Int, key);
                    property->setValue(value.toInt());
                    break;
                case QVariant::Bool:
                    property = m_propertyManager->addProperty(QVariant::Bool, key);
                    property->setValue(value.toBool());
                    break;
                case QVariant::Double:
                    property = m_propertyManager->addProperty(QVariant::Double, key);
                    property->setValue(value.toDouble());
                    break;
                case QVariant::Size: {
                    property = m_propertyManager->addProperty(QVariant::Size, key);
                    QSize size = value.toSize();
                    property->setValue(size);
                    break;
                }
                case QVariant::Rect: {
                    property = m_propertyManager->addProperty(QVariant::Rect, key);
                    QRect rect = value.toRect();
                    property->setValue(rect);
                    break;
                }
                case QVariant::RectF: {
                    property = m_propertyManager->addProperty(QVariant::RectF, key);
                    QRectF rectF = value.toRectF();
                    property->setValue(rectF);
                    break;
                }
                case QVariant::Point: {
                    property = m_propertyManager->addProperty(QVariant::Point, key);
                    QPoint point = value.toPoint();
                    property->setValue(point);
                    break;
                }
                case QVariant::Map: {
                    // 使用 QVariant::Invalid 表示组属性
                    property = m_propertyManager->addProperty(QVariant::Invalid, key);
                    QVariantMap childMap = value.toMap();
                    addPropertiesFromMap(childMap, property, readOnly);  // 递归添加子属性
                    break;
                }
                default:
                    // 对于不支持的类型，显示数据类型信息
                    property = m_propertyManager->addProperty(QVariant::String, key);
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
    for (QtProperty* property : m_propertyBrowser->properties()) {
        map.insert(property->propertyName(), exportProperty(property));
    }
    return map;
}
QVariant QPropertyBrowser::exportProperty(QtProperty* property) const {
    QVariant value;

    if (QtVariantProperty* variantProperty = dynamic_cast<QtVariantProperty*>(property)) {
        if (variantProperty->valueType() == QVariant::Map) {
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