//
// Created by WuBin on 2026/4/29.
//

#include "PropertyTreeWidget.h"

#include <QVBoxLayout>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaEnum>
#include <QStringList>
#include <QSignalBlocker>
#include <QScopedValueRollback>
#include <QTreeWidget>

PropertyTreeWidget::PropertyTreeWidget(QWidget* parent)
    : QWidget(parent)
{
    /**
     * 函数级注释：初始化 QtPropertyBrowser 组件，建立属性管理器与编辑器工厂，并接入 valueChanged 回调
     */
    setObjectName("propertyTreeWidget");

    m_manager = new QtVariantPropertyManager(this);
    m_factory = new QtVariantEditorFactory(this);
    m_browser = new QtTreePropertyBrowser(this);
    m_browser->setFactoryForManager(m_manager, m_factory);

    if (auto* tree = m_browser->findChild<QTreeWidget*>()) {
        tree->setObjectName("propertyTreeView");
        tree->setUniformRowHeights(true);
    }

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_browser);

    connect(m_manager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
            this, SLOT(onValueChanged(QtProperty*, const QVariant&)));
}

PropertyTreeWidget::~PropertyTreeWidget()
{
    /**
     * 函数级注释：析构前清理连接与缓存，避免在对象销毁后的异步 notify 导致访问空指针
     */
    clear();
}

void PropertyTreeWidget::setObject(QObject* object)
{
    /**
     * 函数级注释：绑定目标对象并重建属性树；会自动连接所有 Q_PROPERTY 的 notify 信号实现 UI 刷新
     */
    if (m_object == object) {
        refresh();
        return;
    }

    clear();
    m_object = object;

    rebuild();
}

QObject* PropertyTreeWidget::object() const
{
    /**
     * 函数级注释：返回当前绑定的 QObject（可能为空）
     */
    return m_object;
}

void PropertyTreeWidget::rebuild()
{
    /**
     * 函数级注释：扫描 QMetaObject 的 Q_PROPERTY 并构建属性树结构
     */
    if (m_browser) {
        m_browser->clear();
    }
    m_qtPropToMetaIndex.clear();
    m_metaIndexToQtProp.clear();
    m_notifySignalIndexToMetaIndex.clear();

    if (!m_object) {
        return;
    }

    const QMetaObject* mo = m_object->metaObject();
    if (!mo) {
        return;
    }

    const QScopedValueRollback<bool> updatingGuard(m_updating, true);
    const QSignalBlocker managerBlocker(m_manager);

    for (int i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        const QMetaProperty mp = mo->property(i);
        if (!mp.isReadable()) {
            continue;
        }
        if (!mp.isDesignable()) {
            continue;
        }
        const QByteArray name = mp.name();
        if (name == "objectName") {
            continue;
        }

        QtVariantProperty* vp = createVariantProperty(mp);
        if (!vp) {
            continue;
        }

        const QVariant v = readMetaPropertyValue(mp);
        if (mp.isEnumType()) {
            const QMetaEnum en = mp.enumerator();
            const int enumValue = v.toInt();
            int index = 0;
            for (int k = 0; k < en.keyCount(); ++k) {
                if (en.value(k) == enumValue) {
                    index = k;
                    break;
                }
            }
            vp->setValue(index);
        } else {
            vp->setValue(v);
        }

        if (!mp.isWritable()) {
            m_manager->setAttribute(vp, "readOnly", true);
        }

        m_browser->addProperty(vp);
        m_qtPropToMetaIndex.insert(vp, i);
        m_metaIndexToQtProp.insert(i, vp);

        if (mp.hasNotifySignal()) {
            const int sigIndex = mp.notifySignalIndex();
            if (sigIndex >= 0) {
                m_notifySignalIndexToMetaIndex.insert(sigIndex, i);
            }
        }
    }

    connectNotifySignals();
}

void PropertyTreeWidget::refresh()
{
    /**
     * 函数级注释：从对象重新读取属性值刷新到 UI；用于外部逻辑修改属性后的同步
     */
    if (!m_object) {
        return;
    }

    const QMetaObject* mo = m_object->metaObject();
    if (!mo) {
        return;
    }

    m_updating = true;
    for (auto it = m_metaIndexToQtProp.constBegin(); it != m_metaIndexToQtProp.constEnd(); ++it) {
        const int idx = it.key();
        const QMetaProperty mp = mo->property(idx);
        const QVariant v = readMetaPropertyValue(mp);
        QtVariantProperty* vp = it.value();
        if (!vp) {
            continue;
        }

        if (mp.isEnumType()) {
            const QMetaEnum en = mp.enumerator();
            const int enumValue = v.toInt();
            int index = 0;
            for (int k = 0; k < en.keyCount(); ++k) {
                if (en.value(k) == enumValue) {
                    index = k;
                    break;
                }
            }
            vp->setValue(index);
        } else {
            vp->setValue(v);
        }
    }
    m_updating = false;
}

void PropertyTreeWidget::onValueChanged(QtProperty* property, const QVariant& value)
{
    /**
     * 函数级注释：将属性树的编辑值写回目标对象，并触发 propertyEdited 信号
     */
    if (m_updating) {
        return;
    }
    if (!m_object || !property) {
        return;
    }

    const QMetaObject* mo = m_object->metaObject();
    if (!mo) {
        return;
    }

    const auto it = m_qtPropToMetaIndex.find(property);
    if (it == m_qtPropToMetaIndex.end()) {
        return;
    }

    const int idx = it.value();
    const QMetaProperty mp = mo->property(idx);

    if (!mp.isWritable()) {
        syncOne(idx);
        return;
    }

    QVariant newValue;
    if (mp.isEnumType()) {
        const QMetaEnum en = mp.enumerator();
        const int enumIndex = value.toInt();
        newValue = (enumIndex >= 0 && enumIndex < en.keyCount()) ? en.value(enumIndex) : en.value(0);
    } else {
        newValue = value;
    }

    const QMetaType target = mp.metaType();
    if (newValue.metaType() != target) {
        if (!newValue.canConvert(target) || !newValue.convert(target)) {
            syncOne(idx);
            return;
        }
    }
    
    const QVariant cur = mp.read(m_object);
    if (cur.isValid() && cur == newValue) {
        return;
    }

    if (!writeMetaPropertyValue(mp, newValue)) {
        syncOne(idx);
        return;
    }

    emit propertyEdited(QString::fromLatin1(mp.name()), mp.read(m_object));
}

void PropertyTreeWidget::onAnyPropertyNotify()
{
    /**
     * 函数级注释：根据 senderSignalIndex() 找到对应的属性并刷新该属性的 UI 显示
     */
    if (!m_object) {
        return;
    }

    const int sigIndex = senderSignalIndex();
    const auto it = m_notifySignalIndexToMetaIndex.find(sigIndex);
    if (it == m_notifySignalIndexToMetaIndex.end()) {
        return;
    }

    syncOne(it.value());
}

void PropertyTreeWidget::clear()
{
    /**
     * 函数级注释：断开所有已建立的动态连接，并清理 UI 与索引缓存
     */
    for (const auto& c : m_connections) {
        QObject::disconnect(c);
    }
    m_connections.clear();

    if (m_browser) {
        m_browser->clear();
    }

    m_qtPropToMetaIndex.clear();
    m_metaIndexToQtProp.clear();
    m_notifySignalIndexToMetaIndex.clear();
    m_object = nullptr;
}

void PropertyTreeWidget::connectNotifySignals()
{
    /**
     * 函数级注释：将每个属性的 notify 信号动态连接到 onAnyPropertyNotify 以实现外部变化同步
     */
    if (!m_object) {
        return;
    }

    const QMetaObject* mo = m_object->metaObject();
    if (!mo) {
        return;
    }

    const int slotIndex = this->metaObject()->indexOfSlot("onAnyPropertyNotify()");
    if (slotIndex < 0) {
        return;
    }

    for (auto it = m_notifySignalIndexToMetaIndex.constBegin(); it != m_notifySignalIndexToMetaIndex.constEnd(); ++it) {
        const int sigIndex = it.key();
        if (sigIndex < 0) {
            continue;
        }
        m_connections.push_back(QMetaObject::connect(m_object, sigIndex, this, slotIndex, Qt::AutoConnection));
    }
}

void PropertyTreeWidget::syncOne(int metaPropertyIndex)
{
    /**
     * 函数级注释：从对象读取指定 metaPropertyIndex 的值并更新到对应 UI 属性项
     */
    if (!m_object) {
        return;
    }

    const QMetaObject* mo = m_object->metaObject();
    if (!mo) {
        return;
    }

    QtVariantProperty* vp = m_metaIndexToQtProp.value(metaPropertyIndex, nullptr);
    if (!vp) {
        return;
    }

    const QMetaProperty mp = mo->property(metaPropertyIndex);
    const QVariant v = readMetaPropertyValue(mp);

    m_updating = true;
    if (mp.isEnumType()) {
        const QMetaEnum en = mp.enumerator();
        const int enumValue = v.toInt();
        int index = 0;
        for (int k = 0; k < en.keyCount(); ++k) {
            if (en.value(k) == enumValue) {
                index = k;
                break;
            }
        }
        vp->setValue(index);
    } else {
        vp->setValue(v);
    }
    m_updating = false;
}

QtVariantProperty* PropertyTreeWidget::createVariantProperty(const QMetaProperty& mp) const
{
    /**
     * 函数级注释：根据 QMetaProperty 的类型创建对应的 QtVariantProperty；不支持类型将降级为只读字符串
     */
    if (!m_manager) {
        return nullptr;
    }

    const QString name = QString::fromLatin1(mp.name());

    if (mp.isEnumType()) {
        QtVariantProperty* p = m_manager->addProperty(QtVariantPropertyManager::enumTypeId(), name);
        const QMetaEnum en = mp.enumerator();
        QStringList enumNames;
        enumNames.reserve(en.keyCount());
        for (int i = 0; i < en.keyCount(); ++i) {
            enumNames.push_back(QString::fromLatin1(en.key(i)));
        }
        m_manager->setAttribute(p, "enumNames", enumNames);
        return p;
    }

    const int typeId = mp.metaType().id();
    QtVariantProperty* p = m_manager->addProperty(typeId, name);
    if (!p) {
        p = m_manager->addProperty(QMetaType::QString, name);
        m_manager->setAttribute(p, "readOnly", true);
    }
    return p;
}

QVariant PropertyTreeWidget::readMetaPropertyValue(const QMetaProperty& mp) const
{
    /**
     * 函数级注释：从 QObject 读取属性值并做必要的降级处理
     */
    if (!m_object) {
        return {};
    }

    QVariant v = mp.read(m_object);
    if (!v.isValid()) {
        return {};
    }

    if (mp.isEnumType()) {
        return v.toInt();
    }

    return v;
}

bool PropertyTreeWidget::writeMetaPropertyValue(const QMetaProperty& mp, const QVariant& value) const
{
    /**
     * 函数级注释：将值写入 QObject 的属性；对可转换类型做转换尝试
     */
    if (!m_object) {
        return false;
    }

    if (!mp.isWritable()) {
        return false;
    }

    QVariant v = value;
    const QMetaType target = mp.metaType();
    if (v.metaType() != target) {
        if (!v.canConvert(target)) {
            return false;
        }
        if (!v.convert(target)) {
            return false;
        }
    }

    return mp.write(m_object, v);
}


