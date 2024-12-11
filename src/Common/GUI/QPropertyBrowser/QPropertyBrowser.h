//
// Created by WuBin on 24-11-4.
//

#ifndef QPROPERTYBROWSER_H
#define QPROPERTYBROWSER_H



#include <QWidget>
#include <QtTreePropertyBrowser>
#include <QtVariantPropertyManager>
#include <QtVariantEditorFactory>
#include <QtVariantProperty>
#include <QVBoxLayout>
#include <QVariantMap>
class QPropertyBrowser : public QWidget {
    Q_OBJECT

public:
    QPropertyBrowser(QWidget* parent = nullptr);
    void initializeBranches();
    void buildPropertiesFromMap(const QVariantMap& map);

    void addFixedProperties(int propertyType, const QString &name = QString(),const QVariant &value=QVariant());
    QVariantMap exportToMap() const;
    QVariant getProperties( const QString &name);
    void setProperty(const QString &name,const QVariant &val);
//    QVariantMap exportFlatProperties(QtProperty* property) const;
    QtTreePropertyBrowser* m_propertyBrowser;
    QtVariantPropertyManager* m_propertyManager;
    QtVariantEditorFactory* m_editorFactory;
signals:
    void nodeItemValueChanged(const QString& propertyName, const QVariant& newValue);  // 自定义信号
public slots:
    void buildPropertiesFromJson(const QJsonObject& jsonObj);
private slots:
    void onNodeItemValueChanged(QtProperty* property,const QVariant& value);

private:
    void addPropertiesFromMap(const QVariantMap& map, QtVariantProperty* parentProperty,bool readOnly=true);
    void addPropertiesFromJson(const QJsonObject& jsonObj, QtVariantProperty* parentProperty, bool readOnly=true) ;
    QVariant exportProperty(QtProperty* property) const ;
    QtVariantProperty * VaribaleItem ;
    QtVariantProperty * NodeItem;
    QVBoxLayout* layout;
};


#endif //QPROPERTYBROWSER_H
