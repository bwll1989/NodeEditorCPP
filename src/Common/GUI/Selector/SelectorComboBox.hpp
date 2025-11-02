#pragma once
#include <QComboBox>
#include <QLineEdit>
#include <QCompleter>
#include <QStringList>

/**
 * @brief 基于 QComboBox 的可编辑选择器控件
 * 
 * 功能特点：
 * - 支持手动输入（默认可编辑）
 * - 支持从预设列表中选择条目
 * - 可选自动补全（从当前条目列表中匹配）
 * - 可选在输入结束时自动将新文本加入列表（避免重复添加）
 * - 提供占位文本、校验器等常用接口
 * 
 * 设计说明：
 * - 为避免全局 QSS 导致的宽度不一致，本控件会对内置 QLineEdit 设置局部样式（只影响本控件），使其占满 QComboBox 内容区域。
 * - 所有对 QLineEdit 的操作均通过 QComboBox::lineEdit() 获取，并保证在 setEditable(true) 后可用。
 */
class SelectorComboBox final : public QComboBox {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     * 
     * 默认：
     * - setEditable(true)
     * - 启用 NoInsert（不自动插入），可选在编辑完成时由控件逻辑插入
     * - 局部样式确保行编辑器填满内容区
     */
    explicit SelectorComboBox(QWidget* parent = nullptr);

    /**
     * @brief 设置下拉选项列表
     * @param items 条目字符串列表
     * 
     * 会清空现有条目并替换为新的列表；若启用自动补全，会同步更新补全词典。
     */
    void setItems(const QStringList& items);

    /**
     * @brief 追加一个条目（若不存在）
     * @param item 待追加内容
     * @return bool 是否实际追加（不存在时追加返回 true，存在时返回 false）
     */
    bool addIfNotExists(const QString& item);

    /**
     * @brief 设置当前值（优先选择现有条目，若不存在则作为编辑文本）
     * @param value 目标文本
     */
    void setCurrentValue(const QString& value);

    /**
     * @brief 启用/禁用可编辑输入
     * @param enabled true 可编辑；false 只读选择
     * 
     * 同步维护内置 QLineEdit 的样式与占位文本。
     */
    void setAllowCustomInput(bool enabled);

    /**
     * @brief 设置占位提示文本（仅在可编辑模式下生效）
     * @param text 占位文本
     */
    void setPlaceholderText(const QString& text);

    /**
     * @brief 为内置 QLineEdit 设置校验器
     * @param validator 外部提供的 QValidator 指针（生命周期由外部管理）
     */
    void setValidator(QValidator* validator);

    /**
     * @brief 启用/禁用基于当前条目列表的自动补全
     * @param enabled true 启用自动补全；false 关闭
     * 
     * 自动补全从当前 items() 列表构建。
     */
    void setCompleterEnabled(bool enabled);

    /**
     * @brief 设置是否在编辑结束（回车或失焦）时将新输入追加到列表
     * @param enabled true 追加；false 不追加
     */
    void setAutoAddOnEditingFinished(bool enabled);

    /**
     * @brief 获取当前是否启用自动补全
     * @return bool 
     */
    bool isCompleterEnabled() const { return m_completerEnabled; }

    /**
     * @brief 获取当前是否启用“编辑结束时自动追加”
     * @return bool 
     */
    bool isAutoAddOnEditingFinished() const { return m_autoAddOnEditingFinished; }

signals:
    /**
     * @brief 选择变更信号（条目选择或输入变更都会触发）
     * @param text 当前文本
     */
    void selectionChanged(const QString& text);

    /**
     * @brief 编辑中文本变更信号（仅在可编辑模式下由用户输入触发）
     * @param text 当前编辑文本
     */
    void textEdited(const QString& text);

private slots:
    /**
     * @brief 内部槽：转发 currentTextChanged 到 selectionChanged
     * @param text 当前文本
     */
    void onCurrentTextChangedForward(const QString& text);

    /**
     * @brief 内部槽：处理编辑文本变更（转发 textEdited）
     * @param text 当前编辑文本
     */
    void onEditTextChangedForward(const QString& text);

    /**
     * @brief 内部槽：编辑完成（回车或焦点离开）
     * 
     * 根据 m_autoAddOnEditingFinished 决定是否将新输入追加为条目。
     */
    void onEditingFinished();

private:
    /**
     * @brief 应用内置 QLineEdit 的局部样式与属性
     * 
     * - 去除边距与边框，保持与 QComboBox 内容区一致
     * - 设置适度的左右 padding，保证文本显示舒适
     */
    void applyLineEditLocalStyle();

    /**
     * @brief 重建自动补全器（根据当前 items 列表）
     */
    void rebuildCompleter();

    /**
     * @brief 获取当前所有条目（便于重建补全器）
     */
    QStringList allItems() const;

private:
    QCompleter* m_completer = nullptr;
    bool m_completerEnabled = true;
    bool m_autoAddOnEditingFinished = true;
};