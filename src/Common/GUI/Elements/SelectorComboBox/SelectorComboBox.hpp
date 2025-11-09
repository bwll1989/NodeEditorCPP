#pragma once
#include <QLineEdit>
#include <QCompleter>
#include <QStringList>
#include <QToolButton>
#include <QListView>
#include <QStringListModel>
#include <QFrame>
#include "../devices/MediaLibrary/MediaLibrary.h"
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief 具有侧边折叠候选面板的可编辑行输入控件（LineEdit）
 *
 * 功能：
 * - 手动输入（默认可编辑）
 * - 侧边按钮展开候选面板（列表选择）
 * - 可选自动补全（基于当前候选列表）
 * - 可选编辑结束时自动将新文本加入候选列表
 * - 支持占位文本与校验器
 *
 * 信号：
 * - selectionChanged(text)：输入或选择变更都会触发
 * - textEdited(text)：仅用户编辑触发
 */
class GUI_ELEMENTS_EXPORT SelectorComboBox final : public QLineEdit {
    Q_OBJECT
public:
    explicit SelectorComboBox(MediaLibrary::Category category,QWidget* parent = nullptr);

    // 替换候选项列表
    void updateItems(MediaLibrary::Category category);
    // 若不存在则追加
    bool addIfNotExists(const QString& item);
    // 设置当前值（若存在候选则作为选择，否则直接设置文本）
    void setCurrentValue(const QString& value);
    // 启用/禁用可编辑输入（对应 readOnly 反向）
    void setAllowCustomInput(bool enabled);
    // 设置占位提示文本
    void setPlaceholderText(const QString& text);
    // 设置校验器
    void setValidator(QValidator* validator);
    // 启用/禁用自动补全
    void setCompleterEnabled(bool enabled);
    // 编辑完成时自动追加新文本
    void setAutoAddOnEditingFinished(bool enabled);

    bool isCompleterEnabled() const { return m_completerEnabled; }
    bool isAutoAddOnEditingFinished() const { return m_autoAddOnEditingFinished; }

signals:
    // 输入或选择变更
    void selectionChanged(const QString& text);
    // 用户编辑变更
    void textEdited(const QString& text);

protected:
    void resizeEvent(QResizeEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;

private slots:
    void onTogglePopup();
    void onItemClicked(const QModelIndex& index);
    void onEditingFinishedInternal();
    void onTextChangedForward(const QString& text);
    void onTextEditedForward(const QString& text);

private:
    void applyLineEditLocalStyle();
    void rebuildCompleter();
    QStringList allItems() const;
    void updateModel();
    void placePopup();

private:
    // 右侧切换按钮与侧边候选面板
    MediaLibrary* m_mediaLibrary = nullptr;
    QToolButton* m_toggleButton = nullptr;
    QFrame* m_popupFrame = nullptr;
    QListView* m_listView = nullptr;
    QStringListModel* m_model = nullptr;
    MediaLibrary::Category m_category = MediaLibrary::Category::Image;
    QCompleter* m_completer = nullptr;
    QStringList m_items;
    bool m_completerEnabled = true;
    bool m_autoAddOnEditingFinished = false;
    bool m_popupVisible = false;
};