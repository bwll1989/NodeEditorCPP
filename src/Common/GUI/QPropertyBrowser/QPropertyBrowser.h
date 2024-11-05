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

    void addProperties();
    void buildPropertiesFromMap(const QVariantMap& map);
    QVariantMap exportToMap() const;
private:
    void addPropertiesFromMap(const QVariantMap& map, QtVariantProperty* parentProperty,bool readOnly=true);
    QVariant exportProperty(QtProperty* property) const ;
    QtTreePropertyBrowser* m_propertyBrowser;
    QtVariantPropertyManager* m_propertyManager;
    QtVariantEditorFactory* m_editorFactory;
    QVBoxLayout* layout;
};


#endif //QPROPERTYBROWSER_H
