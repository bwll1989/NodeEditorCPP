#include "SelectorComboBox.hpp"
#include <QAbstractItemView>
#include <QValidator>

/**
 * @brief 构造函数：默认启用可编辑、自动补全与编辑后自动追加
 * @param parent 父对象
 */
SelectorComboBox::SelectorComboBox(QWidget* parent)
    : QComboBox(parent)
{
    // 可编辑输入，允许手动填充名称
    setEditable(true);
    // 默认不由 QComboBox 自行插入，交给 onEditingFinished 控制
    setInsertPolicy(QComboBox::NoInsert);
    // 让下拉视图在较暗主题下也可读
    view()->setAlternatingRowColors(false);

    // 应用内置 QLineEdit 的局部样式，避免受全局 QSS 的宽度/边距影响
    applyLineEditLocalStyle();

    // 信号转发
    connect(this, &QComboBox::currentTextChanged,
            this, &SelectorComboBox::onCurrentTextChangedForward);
    connect(this, &QComboBox::editTextChanged,
            this, &SelectorComboBox::onEditTextChangedForward);

    if (lineEdit()) {
        connect(lineEdit(), &QLineEdit::editingFinished,
                this, &SelectorComboBox::onEditingFinished);
    }

    // 构建自动补全器
    rebuildCompleter();
}

/**
 * @brief 设置下拉选项列表（替换全部）
 * @param items 条目字符串列表
 */
void SelectorComboBox::setItems(const QStringList& items) {
    clear();
    addItems(items);
    rebuildCompleter();
}

/**
 * @brief 若条目不存在则追加
 * @param item 待追加内容
 * @return bool 是否实际追加
 */
bool SelectorComboBox::addIfNotExists(const QString& item) {
    const int idx = findText(item, Qt::MatchExactly);
    if (idx >= 0) return false;
    addItem(item);
    rebuildCompleter();
    return true;
}

/**
 * @brief 设置当前值（优先选择现有条目，否则作为编辑文本）
 * @param value 目标文本
 */
void SelectorComboBox::setCurrentValue(const QString& value) {
    const int idx = findText(value, Qt::MatchExactly);
    if (idx >= 0) {
        setCurrentIndex(idx);
    } else {
        setEditText(value);
    }
}

/**
 * @brief 启用/禁用可编辑输入
 * @param enabled true 可编辑；false 只读选择
 */
void SelectorComboBox::setAllowCustomInput(bool enabled) {
    setEditable(enabled);
    applyLineEditLocalStyle();
}

/**
 * @brief 设置占位提示文本
 * @param text 占位文本
 */
void SelectorComboBox::setPlaceholderText(const QString& text) {
    if (lineEdit()) {
        lineEdit()->setPlaceholderText(text);
    }
}

/**
 * @brief 为内置 QLineEdit 设置校验器
 * @param validator 外部提供的 QValidator 指针
 */
void SelectorComboBox::setValidator(QValidator* validator) {
    if (lineEdit()) {
        lineEdit()->setValidator(validator);
    }
}

/**
 * @brief 启用/禁用自动补全
 * @param enabled true 启用；false 关闭
 */
void SelectorComboBox::setCompleterEnabled(bool enabled) {
    m_completerEnabled = enabled;
    rebuildCompleter();
}

/**
 * @brief 设置是否在编辑完成时自动将新文本追加到列表
 * @param enabled true 追加；false 不追加
 */
void SelectorComboBox::setAutoAddOnEditingFinished(bool enabled) {
    m_autoAddOnEditingFinished = enabled;
}

/**
 * @brief 转发 currentTextChanged 到 selectionChanged
 * @param text 当前文本
 */
void SelectorComboBox::onCurrentTextChangedForward(const QString& text) {
    emit selectionChanged(text);
}

/**
 * @brief 编辑中文本变更转发
 * @param text 当前编辑文本
 */
void SelectorComboBox::onEditTextChangedForward(const QString& text) {
    emit textEdited(text);
}

/**
 * @brief 编辑完成处理：自动追加输入（若启用且不存在）
 */
void SelectorComboBox::onEditingFinished() {
    if (!m_autoAddOnEditingFinished) return;
    if (!isEditable() || !lineEdit()) return;

    const QString text = lineEdit()->text().trimmed();
    if (text.isEmpty()) return;

    addIfNotExists(text);
}

/**
 * @brief 应用内置 QLineEdit 的局部样式与属性
 * 
 * 样式目标：
 * - 让行编辑器占满 QComboBox 内容区域（不被无关 margin/padding 挤压）
 * - 去掉额外边框，使用透明背景让整体视觉与主题一致
 */
void SelectorComboBox::applyLineEditLocalStyle() {
    if (!lineEdit() && isEditable()) {
        // 确保可编辑模式实际创建了 QLineEdit
        setEditable(true);
    }
    if (auto* le = lineEdit()) {
        le->setFrame(false);
        le->setContentsMargins(0, 0, 0, 0);
        // 局部样式仅作用于这个 lineEdit，避免影响全局
        le->setStyleSheet(
            "QLineEdit {"
            "  margin: 0px;"
            "  padding: 0 4px;"
            "  border: none;"
            "  background: transparent;"
            "}"
        );
    }
}

/**
 * @brief 重建自动补全器
 */
void SelectorComboBox::rebuildCompleter() {
    if (!isEditable()) {
        if (m_completer) {
            // 不可编辑时移除补全器
            setCompleter(nullptr);
        }
        return;
    }

    if (!m_completerEnabled) {
        setCompleter(nullptr);
        return;
    }

    if (!m_completer) {
        m_completer = new QCompleter(this);
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        m_completer->setCompletionMode(QCompleter::PopupCompletion);
    }

    m_completer->setModel(new QStringListModel(allItems(), m_completer));
    setCompleter(m_completer);
}

/**
 * @brief 获取所有条目组成的列表
 * @return QStringList 
 */
QStringList SelectorComboBox::allItems() const {
    QStringList list;
    list.reserve(count());
    for (int i = 0; i < count(); ++i) {
        list.push_back(itemText(i));
    }
    return list;
}