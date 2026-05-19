//
// Created by WuBin on 2026/4/29.
//

#pragma once

#include <QWidget>
#include <QPointer>
#include <QHash>
#include <QVector>

#include "QtTreePropertyBrowser.h"
#include <QtVariantPropertyManager>
#include <QtVariantEditorFactory>
#include <QtVariantProperty>
#if defined(PROPERTYTREEWIDGET_LIBRARY)
#define PROPERTYTREEWIDGET_EXPORT Q_DECL_EXPORT
#else
#define PROPERTYTREEWIDGET_EXPORT Q_DECL_IMPORT
#endif
class PROPERTYTREEWIDGET_EXPORT PropertyTreeWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * 函数级注释：构造属性树控件，初始化 QtPropertyBrowser 组件与默认编辑工厂
     */
    explicit PropertyTreeWidget(QWidget* parent = nullptr);

    /**
     * 函数级注释：析构控件并安全断开与目标对象的通知连接
     */
    ~PropertyTreeWidget() override;

    /**
     * 函数级注释：绑定一个 QObject，自动扫描其 Q_PROPERTY 并构建属性树
     */
    void setObject(QObject* object);

    /**
     * 函数级注释：获取当前绑定的 QObject
     */
    QObject* object() const;

    /**
     * 函数级注释：重新扫描并重建属性树（用于对象元信息或可见性变化场景）
     */
    void rebuild();

    /**
     * 函数级注释：从对象读取当前属性值并刷新到 UI（不重建结构）
     */
    void refresh();

signals:
    /**
     * 函数级注释：当用户在属性树中编辑任意属性后发出通知
     */
    void propertyEdited(const QString& name, const QVariant& value);

private slots:
    /**
     * 函数级注释：处理属性树编辑事件，将值写回到 QObject 的对应 Q_PROPERTY
     */
    void onValueChanged(QtProperty* property, const QVariant& value);

    /**
     * 函数级注释：处理任意 notify 信号，依据 senderSignalIndex() 映射到对应属性并刷新 UI
     */
    void onAnyPropertyNotify();

private:
    void clear();
    void connectNotifySignals();
    void syncOne(int metaPropertyIndex);

    QtVariantProperty* createVariantProperty(const QMetaProperty& mp) const;
    QVariant readMetaPropertyValue(const QMetaProperty& mp) const;
    bool writeMetaPropertyValue(const QMetaProperty& mp, const QVariant& value) const;

private:
    QPointer<QObject> m_object;

    QtTreePropertyBrowser* m_browser = nullptr;
    QtVariantPropertyManager* m_manager = nullptr;
    QtVariantEditorFactory* m_factory = nullptr;

    QHash<QtProperty*, int> m_qtPropToMetaIndex;
    QHash<int, QtVariantProperty*> m_metaIndexToQtProp;
    QHash<int, int> m_notifySignalIndexToMetaIndex;
    QVector<QMetaObject::Connection> m_connections;

    bool m_updating = false;
};
