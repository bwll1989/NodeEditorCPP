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

class QTreeWidget;
#if defined(PROPERTYTREEWIDGET_LIBRARY)
#define PROPERTYTREEWIDGET_EXPORT Q_DECL_EXPORT
#else
#define PROPERTYTREEWIDGET_EXPORT Q_DECL_IMPORT
#endif
class PROPERTYTREEWIDGET_EXPORT PropertyTreeWidget : public QWidget
{
    Q_OBJECT
    /**
     * 属性级注释：属性树选中/悬停配色，可通过 QSS 的 qproperty-* 在主题文件中配置。
     * 实现说明：本控件常嵌入节点 QGraphicsScene 代理层级，QSS 的 ::item 规则可能
     * 传播不到该层级，故 setter 把选中色写入内部 QTreeWidget 的 QPalette
     * （palette 是 item 绘制的最终依据，不依赖 QSS 规则匹配），并同步控件本地
     * 样式表处理悬停；构造函数以品牌绿为默认值兜底，即使 QSS 未注入也不会回退系统蓝。
     */
    Q_PROPERTY(QColor selectedColor READ selectedColor WRITE setSelectedColor)
    Q_PROPERTY(QColor selectedTextColor READ selectedTextColor WRITE setSelectedTextColor)
    Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor)
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

    QColor selectedColor() const;
    void setSelectedColor(const QColor& color);

    QColor selectedTextColor() const;
    void setSelectedTextColor(const QColor& color);

    QColor hoverColor() const;
    void setHoverColor(const QColor& color);

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
    void disconnectNotifies();
    void connectNotifySignals();
    void syncOne(int metaPropertyIndex);
    void applyMetaValue(const QMetaObject* metaObject, int metaPropertyIndex);
    void applyTreeColors();

    QtVariantProperty* createVariantProperty(const QMetaProperty& mp);

    QPointer<QObject> m_object;

    // 选中/悬停配色（默认值 = Flow 品牌色板，QSS 可覆盖）
    QColor m_selectedColor{QColor("#79AE31")};     // 流动绿 500（选中底）
    QColor m_selectedTextColor{QColor("#141A0D")}; // 墨绿黑（绿底禁白小字）
    QColor m_hoverColor{QColor(121, 174, 49, 60)}; // 品牌绿低透明（悬停底）

    QtTreePropertyBrowser* m_browser = nullptr;
    QTreeWidget* m_tree = nullptr;
    QtVariantPropertyManager* m_manager = nullptr;
    QtVariantEditorFactory* m_factory = nullptr;

    QHash<QtProperty*, int> m_qtPropToMetaIndex;
    QHash<int, QtVariantProperty*> m_metaIndexToQtProp;
    QHash<int, QVector<int>> m_notifySignalToMetaIndexes;
    QVector<QMetaObject::Connection> m_connections;
    int m_notifySlotIndex = -1;

    bool m_updating = false;
};
