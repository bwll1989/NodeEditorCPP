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
#include <QHash>
#include <QEvent>
#include <QSet>

#include "IntDragValueWidget/IntDragValueWidget.hpp"
#include "FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "SelectorComboBox/SelectorComboBox.hpp"

class DragVariantEditorFactory;

/**
 * 函数级注释：属性浏览器的 staticMetaObject 未导出，信号改由普通 QObject 用字符串方式接收
 */
class DragEditorSync : public QObject
{
    Q_OBJECT
public:
    explicit DragEditorSync(DragVariantEditorFactory* factory, QObject* parent = nullptr)
        : QObject(parent)
        , m_factory(factory)
    {
    }

public slots:
    void onManagerValueChanged(QtProperty* property, const QVariant& value);
    void onManagerAttributeChanged(QtProperty* property, const QString& attribute, const QVariant& value);
    void onManagerPropertyDestroyed(QtProperty* property);

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    DragVariantEditorFactory* m_factory = nullptr;
    QSet<QObject*> m_filterFront;
};

/**
 * 函数级注释：int 用 IntDragValueWidget，double/float 用 FloatDragValueWidget，枚举用 SelectorComboBox，其余类型仍交给 QtVariantEditorFactory
 */
class DragVariantEditorFactory : public QtVariantEditorFactory
{
    friend class DragEditorSync;
public:
    explicit DragVariantEditorFactory(QObject* parent = nullptr)
        : QtVariantEditorFactory(parent)
    {
    }

protected:
    void connectPropertyManager(QtVariantPropertyManager* manager) override
    {
        QtVariantEditorFactory::connectPropertyManager(manager);
        m_manager = manager;
        if (!m_sync) {
            m_sync = new DragEditorSync(this, this);
        }
        m_valueConn = connect(manager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
                              m_sync, SLOT(onManagerValueChanged(QtProperty*, const QVariant&)));
        m_attrConn = connect(manager, SIGNAL(attributeChanged(QtProperty*, const QString&, const QVariant&)),
                             m_sync, SLOT(onManagerAttributeChanged(QtProperty*, const QString&, const QVariant&)));
        m_destroyedConn = connect(manager, SIGNAL(propertyDestroyed(QtProperty*)),
                                  m_sync, SLOT(onManagerPropertyDestroyed(QtProperty*)));
    }

    void disconnectPropertyManager(QtVariantPropertyManager* manager) override
    {
        disconnect(m_valueConn);
        disconnect(m_attrConn);
        disconnect(m_destroyedConn);
        m_valueConn = {};
        m_attrConn = {};
        m_destroyedConn = {};
        if (m_manager == manager) {
            m_manager = nullptr;
        }
        QtVariantEditorFactory::disconnectPropertyManager(manager);
    }

    QWidget* createEditor(QtVariantPropertyManager* manager, QtProperty* property, QWidget* parent) override
    {
        const int propType = manager->propertyType(property);
        if (propType == QMetaType::Int) {
            auto* editor = new IntDragValueWidget(parent);
            prepareEditor(editor, property);
            applyIntAttributes(editor, property, true);
            connect(editor, &IntDragValueWidget::valueChanged, this, [this, editor](int value) {
                commit(editor, value);
            });
            return editor;
        }
        if (propType == QMetaType::Double) {
            auto* editor = new FloatDragValueWidget(parent);
            prepareEditor(editor, property);
            applyFloatAttributes(editor, property, true);
            connect(editor, &FloatDragValueWidget::valueChanged, this, [this, editor](double value) {
                commit(editor, value);
            });
            return editor;
        }
        if (propType == QtVariantPropertyManager::enumTypeId()) {
            auto* editor = new SelectorComboBox(QStringList(), parent);
            editor->setAllowCustomInput(false);
            editor->setCompleterEnabled(false);
            editor->setAutoAddOnEditingFinished(false);
            prepareEditor(editor, property);
            applyEnumAttributes(editor, property, true);
            connect(editor, &SelectorComboBox::selectionChanged, this, [this, editor](const QString& text) {
                commitEnum(editor, text);
            });
            // 属性树代理会在弹出层抢焦点时关掉编辑器；显示后再把过滤器挪到最前才能拦住这次 FocusOut
            if (m_sync) {
                editor->installEventFilter(m_sync);
            }
            return editor;
        }
        return QtVariantEditorFactory::createEditor(manager, property, parent);
    }

private:
    void prepareEditor(QWidget* editor, QtProperty* property)
    {
        editor->setFocusPolicy(Qt::StrongFocus);
        editor->setMinimumHeight(16);
        m_propertyToEditor.insert(property, editor);
        m_editorToProperty.insert(editor, property);
        connect(editor, &QObject::destroyed, this, [this](QObject* object) {
            auto* widget = static_cast<QWidget*>(object);
            QtProperty* property = m_editorToProperty.take(widget);
            if (property && m_propertyToEditor.value(property) == widget) {
                m_propertyToEditor.remove(property);
            }
        });
    }

    void commit(QWidget* editor, const QVariant& value)
    {
        if (!m_manager) {
            return;
        }
        QtProperty* property = m_editorToProperty.value(editor, nullptr);
        if (!property) {
            return;
        }
        m_manager->setValue(property, value);
    }

    void syncEditorValue(QtProperty* property, const QVariant& value)
    {
        QWidget* widget = m_propertyToEditor.value(property, nullptr);
        if (!widget) {
            return;
        }
        if (auto* editor = qobject_cast<IntDragValueWidget*>(widget)) {
            if (editor->value() == value.toInt()) {
                return;
            }
            const QSignalBlocker blocker(editor);
            editor->setValue(value.toInt());
            return;
        }
        if (auto* editor = qobject_cast<FloatDragValueWidget*>(widget)) {
            const QSignalBlocker blocker(editor);
            editor->setValue(value.toDouble());
            return;
        }
        if (auto* editor = qobject_cast<SelectorComboBox*>(widget)) {
            const QString text = enumText(property, value.toInt());
            if (editor->text() == text) {
                return;
            }
            const QSignalBlocker blocker(editor);
            editor->setCurrentValue(text);
        }
    }

    void applyAttributes(QtProperty* property, bool includeValue)
    {
        QWidget* widget = m_propertyToEditor.value(property, nullptr);
        if (!widget || !m_manager) {
            return;
        }
        if (auto* editor = qobject_cast<IntDragValueWidget*>(widget)) {
            applyIntAttributes(editor, property, includeValue);
        } else if (auto* editor = qobject_cast<FloatDragValueWidget*>(widget)) {
            applyFloatAttributes(editor, property, includeValue);
        } else if (auto* editor = qobject_cast<SelectorComboBox*>(widget)) {
            applyEnumAttributes(editor, property, includeValue);
        }
    }

    void applyIntAttributes(IntDragValueWidget* editor, QtProperty* property, bool includeValue)
    {
        if (!m_manager) {
            return;
        }
        const QSignalBlocker blocker(editor);
        const QVariant min = m_manager->attributeValue(property, QStringLiteral("minimum"));
        const QVariant max = m_manager->attributeValue(property, QStringLiteral("maximum"));
        const QVariant step = m_manager->attributeValue(property, QStringLiteral("singleStep"));
        if (min.isValid() && max.isValid()) {
            editor->setRange(min.toInt(), max.toInt());
        }
        if (step.isValid() && step.toInt() != 0) {
            editor->setSingleStep(step.toInt());
        }
        if (includeValue) {
            editor->setValue(m_manager->value(property).toInt());
        }
    }

    void applyFloatAttributes(FloatDragValueWidget* editor, QtProperty* property, bool includeValue)
    {
        if (!m_manager) {
            return;
        }
        const QSignalBlocker blocker(editor);
        const QVariant min = m_manager->attributeValue(property, QStringLiteral("minimum"));
        const QVariant max = m_manager->attributeValue(property, QStringLiteral("maximum"));
        const QVariant step = m_manager->attributeValue(property, QStringLiteral("singleStep"));
        const QVariant decimals = m_manager->attributeValue(property, QStringLiteral("decimals"));
        if (min.isValid() && max.isValid()) {
            editor->setRange(min.toDouble(), max.toDouble());
        }
        if (step.isValid() && step.toDouble() != 0.0) {
            editor->setSingleStep(step.toDouble());
        }
        if (decimals.isValid()) {
            editor->setDecimals(decimals.toInt());
        }
        if (includeValue) {
            editor->setValue(m_manager->value(property).toDouble());
        }
    }

    QStringList enumNames(QtProperty* property) const
    {
        if (!m_manager) {
            return {};
        }
        return m_manager->attributeValue(property, QStringLiteral("enumNames")).toStringList();
    }

    static QString enumTextAt(const QStringList& names, int index)
    {
        if (index < 0 || index >= names.size()) {
            return {};
        }
        return names.at(index);
    }

    void applyEnumAttributes(SelectorComboBox* editor, QtProperty* property, bool includeValue)
    {
        if (!m_manager) {
            return;
        }
        const QSignalBlocker blocker(editor);
        const QStringList names = enumNames(property);
        editor->setItems(names);
        if (!includeValue) {
            return;
        }
        editor->setCurrentValue(enumTextAt(names, m_manager->value(property).toInt()));
    }

    void commitEnum(SelectorComboBox* editor, const QString& text)
    {
        if (!m_manager) {
            return;
        }
        QtProperty* property = m_editorToProperty.value(editor, nullptr);
        if (!property) {
            return;
        }
        const int index = enumNames(property).indexOf(text);
        if (index < 0 || m_manager->value(property).toInt() == index) {
            return;
        }
        m_manager->setValue(property, index);
    }

    QString enumText(QtProperty* property, int index) const
    {
        return enumTextAt(enumNames(property), index);
    }

    void forgetProperty(QtProperty* property)
    {
        QWidget* editor = m_propertyToEditor.take(property);
        if (editor) {
            m_editorToProperty.remove(editor);
        }
    }

    QtVariantPropertyManager* m_manager = nullptr;
    QHash<QtProperty*, QWidget*> m_propertyToEditor;
    QHash<QWidget*, QtProperty*> m_editorToProperty;
    DragEditorSync* m_sync = nullptr;
    QMetaObject::Connection m_valueConn;
    QMetaObject::Connection m_attrConn;
    QMetaObject::Connection m_destroyedConn;
};

void DragEditorSync::onManagerValueChanged(QtProperty* property, const QVariant& value)
{
    if (m_factory) {
        m_factory->syncEditorValue(property, value);
    }
}

void DragEditorSync::onManagerAttributeChanged(QtProperty* property, const QString&, const QVariant&)
{
    if (m_factory) {
        m_factory->applyAttributes(property, false);
    }
}

void DragEditorSync::onManagerPropertyDestroyed(QtProperty* property)
{
    if (m_factory) {
        m_factory->forgetProperty(property);
    }
}

bool DragEditorSync::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::Show) {
        if (!m_filterFront.contains(watched)) {
            m_filterFront.insert(watched);
            watched->installEventFilter(this);
        }
        return false;
    }
    if (event->type() == QEvent::Destroy) {
        m_filterFront.remove(watched);
        return false;
    }
    if (event->type() == QEvent::FocusOut) {
        if (auto* editor = qobject_cast<SelectorComboBox*>(watched)) {
            if (editor->isPopupVisible()) {
                return true;
            }
        }
    }
    return QObject::eventFilter(watched, event);
}

namespace {

int enumToIndex(const QMetaEnum& en, int enumValue)
{
    for (int i = 0; i < en.keyCount(); ++i) {
        if (en.value(i) == enumValue) {
            return i;
        }
    }
    return 0;
}

QVariant indexToEnumValue(const QMetaEnum& en, int index)
{
    if (index >= 0 && index < en.keyCount()) {
        return en.value(index);
    }
    return en.value(0);
}

class TreeUpdateGuard
{
public:
    explicit TreeUpdateGuard(QTreeWidget* tree)
        : m_tree(tree)
    {
        if (m_tree) {
            m_tree->setUpdatesEnabled(false);
        }
    }

    ~TreeUpdateGuard()
    {
        if (m_tree) {
            m_tree->setUpdatesEnabled(true);
        }
    }

private:
    QTreeWidget* m_tree;
};

} // namespace

PropertyTreeWidget::PropertyTreeWidget(QWidget* parent)
    : QWidget(parent)
{
    /**
     * 函数级注释：初始化 QtPropertyBrowser 组件，建立属性管理器与编辑器工厂，并接入 valueChanged 回调
     */
    setObjectName("propertyTreeWidget");

    m_manager = new QtVariantPropertyManager(this);
    m_factory = new DragVariantEditorFactory(this);
    m_browser = new QtTreePropertyBrowser(this);
    m_browser->setFactoryForManager(m_manager, m_factory);
    m_notifySlotIndex = metaObject()->indexOfSlot("onAnyPropertyNotify()");

    m_tree = m_browser->findChild<QTreeWidget*>();
    if (m_tree) {
        m_tree->setObjectName("propertyTreeView");
        m_tree->setUniformRowHeights(true);
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
    disconnectNotifies();
    const TreeUpdateGuard updateGuard(m_tree);
    if (m_browser) {
        m_browser->clear();
    }
    m_qtPropToMetaIndex.clear();
    m_metaIndexToQtProp.clear();
    m_notifySignalToMetaIndexes.clear();

    if (!m_object || !m_manager || !m_browser) {
        return;
    }

    const QMetaObject* mo = m_object->metaObject();
    if (!mo) {
        return;
    }

    const int offset = mo->propertyOffset();
    const int count = mo->propertyCount() - offset;
    m_qtPropToMetaIndex.reserve(count);
    m_metaIndexToQtProp.reserve(count);

    const QScopedValueRollback<bool> updatingGuard(m_updating, true);
    const QSignalBlocker managerBlocker(m_manager);

    for (int i = offset; i < mo->propertyCount(); ++i) {
        const QMetaProperty mp = mo->property(i);
        if (!mp.isReadable() || !mp.isDesignable() || qstrcmp(mp.name(), "objectName") == 0) {
            continue;
        }

        QtVariantProperty* vp = createVariantProperty(mp);
        if (!vp) {
            continue;
        }

        const QVariant raw = mp.read(m_object);
        vp->setValue(mp.isEnumType() ? enumToIndex(mp.enumerator(), raw.toInt()) : raw);

        m_browser->addProperty(vp);
        m_qtPropToMetaIndex.insert(vp, i);
        m_metaIndexToQtProp.insert(i, vp);

        if (mp.hasNotifySignal()) {
            const int sigIndex = mp.notifySignalIndex();
            if (sigIndex >= 0) {
                m_notifySignalToMetaIndexes[sigIndex].append(i);
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

    const QScopedValueRollback<bool> updatingGuard(m_updating, true);
    for (auto it = m_metaIndexToQtProp.cbegin(); it != m_metaIndexToQtProp.cend(); ++it) {
        applyMetaValue(mo, it.key());
    }
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

    QVariant newValue = mp.isEnumType() ? indexToEnumValue(mp.enumerator(), value.toInt()) : value;
    const QMetaType target = mp.metaType();
    if (newValue.metaType() != target && (!newValue.canConvert(target) || !newValue.convert(target))) {
        syncOne(idx);
        return;
    }

    const QVariant current = mp.read(m_object);
    if (current.isValid() && current == newValue) {
        return;
    }

    bool wrote = false;
    {
        // 挡住 write 同步触发的 notify，避免拖拽时再读一遍属性刷回界面
        const QScopedValueRollback<bool> updatingGuard(m_updating, true);
        wrote = mp.write(m_object, newValue);
    }
    if (!wrote) {
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
    if (!m_object || m_updating) {
        return;
    }

    const int sigIndex = senderSignalIndex();
    const auto it = m_notifySignalToMetaIndexes.constFind(sigIndex);
    if (it == m_notifySignalToMetaIndexes.cend()) {
        return;
    }

    const QMetaObject* mo = m_object->metaObject();
    if (!mo) {
        return;
    }

    const QScopedValueRollback<bool> updatingGuard(m_updating, true);
    for (int metaIndex : it.value()) {
        applyMetaValue(mo, metaIndex);
    }
}

void PropertyTreeWidget::clear()
{
    /**
     * 函数级注释：断开所有已建立的动态连接，并清理 UI 与索引缓存
     */
    m_object = nullptr;
    rebuild();
}

void PropertyTreeWidget::disconnectNotifies()
{
    for (const auto& connection : m_connections) {
        QObject::disconnect(connection);
    }
    m_connections.clear();
}

void PropertyTreeWidget::connectNotifySignals()
{
    /**
     * 函数级注释：每个 notify 信号只连一次，一个信号可刷新它对应的全部属性
     */
    if (!m_object || m_notifySlotIndex < 0) {
        return;
    }

    m_connections.reserve(m_notifySignalToMetaIndexes.size());
    for (auto it = m_notifySignalToMetaIndexes.cbegin(); it != m_notifySignalToMetaIndexes.cend(); ++it) {
        const int sigIndex = it.key();
        if (sigIndex < 0) {
            continue;
        }
        m_connections.push_back(QMetaObject::connect(
            m_object, sigIndex, this, m_notifySlotIndex, Qt::AutoConnection));
    }
}

void PropertyTreeWidget::syncOne(int metaPropertyIndex)
{
    /**
     * 函数级注释：从对象读取指定属性并更新到对应 UI
     */
    if (!m_object) {
        return;
    }
    const QMetaObject* mo = m_object->metaObject();
    if (!mo) {
        return;
    }
    const QScopedValueRollback<bool> updatingGuard(m_updating, true);
    applyMetaValue(mo, metaPropertyIndex);
}

void PropertyTreeWidget::applyMetaValue(const QMetaObject* metaObject, int metaPropertyIndex)
{
    QtVariantProperty* vp = m_metaIndexToQtProp.value(metaPropertyIndex, nullptr);
    if (!vp || !m_object) {
        return;
    }

    const QMetaProperty mp = metaObject->property(metaPropertyIndex);
    const QVariant raw = mp.read(m_object);
    vp->setValue(mp.isEnumType() ? enumToIndex(mp.enumerator(), raw.toInt()) : raw);
}

QtVariantProperty* PropertyTreeWidget::createVariantProperty(const QMetaProperty& mp)
{
    /**
     * 函数级注释：根据 QMetaProperty 的类型创建对应的 QtVariantProperty；不支持类型将降级为字符串
     */
    if (!m_manager) {
        return nullptr;
    }

    const QString name = QString::fromLatin1(mp.name());

    if (mp.isEnumType()) {
        QtVariantProperty* property = m_manager->addProperty(QtVariantPropertyManager::enumTypeId(), name);
        if (!property) {
            return nullptr;
        }
        const QMetaEnum en = mp.enumerator();
        QStringList enumNames;
        enumNames.reserve(en.keyCount());
        for (int i = 0; i < en.keyCount(); ++i) {
            enumNames.push_back(QString::fromLatin1(en.key(i)));
        }
        m_manager->setAttribute(property, QStringLiteral("enumNames"), enumNames);
        return property;
    }

    int typeId = mp.metaType().id();
    // QtVariantPropertyManager 没有 float 编辑器，按 double 建属性，写回时再转回 float
    if (typeId == QMetaType::Float) {
        typeId = QMetaType::Double;
    }
    QtVariantProperty* property = m_manager->addProperty(typeId, name);
    if (!property) {
        return m_manager->addProperty(QMetaType::QString, name);
    }
    if (typeId == QMetaType::Double) {
        // 与 FloatDragValueWidget 默认步进一致；Shift 精调，Ctrl/Alt 粗调
        m_manager->setAttribute(property, QStringLiteral("singleStep"), 0.1);
    }
    return property;
}

#include "PropertyTreeWidget.moc"


