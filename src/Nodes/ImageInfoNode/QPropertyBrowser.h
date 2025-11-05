//
// Created by WuBin on 24-11-4.
//

#pragma once

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
    QVariantMap exportToMap() const;
    // QVariant getProperties( const QString &name);
    // void setProperty(const QString &name,const QVariant &val);
//    QVariantMap exportFlatProperties(QtProperty* property) const;
    QtTreePropertyBrowser* m_propertyBrowser;
    QtVariantPropertyManager* m_propertyManager;
    QtVariantEditorFactory* m_editorFactory;
signals:
    void nodeItemValueChanged(const QString& propertyName, const QVariant& newValue);  // 自定义信号
private slots:
    void onNodeItemValueChanged(QtProperty* property,const QVariant& value);

private:
    void updatePropertiesFromMap(const QVariantMap& map, QtProperty* parent = nullptr, bool readOnly=true);
    QVariant exportProperty(QtProperty* property) const ;
    QtVariantProperty * VaribaleItem ;
    QVBoxLayout* layout;
};

