#include "DataBridgeSelectorBox.hpp"
#include <QValidator>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>

DataBridgeSelectorBox::DataBridgeSelectorBox(QWidget* parent)
    : QLineEdit(parent), m_modelDataBridge(&ModelDataBridge::instance())
{
    // 局部样式与行为
    applyLineEditLocalStyle();

    // 右侧切换按钮
    m_toggleButton = new QToolButton(this);
    m_toggleButton->setCursor(Qt::PointingHandCursor);
    m_toggleButton->setAutoRaise(true);
    m_toggleButton->setText("⋯"); // 简洁指示（可替换为图标）
    m_toggleButton->setToolTip(tr("展开候选"));
    connect(m_toggleButton, &QToolButton::clicked, this, &DataBridgeSelectorBox::onTogglePopup);

    // 侧边弹出面板（修复：将父对象设为 this，使其作为控件的“父弹窗”）
    m_popupFrame = new QFrame(this, Qt::Popup | Qt::FramelessWindowHint);
    m_popupFrame->setAttribute(Qt::WA_TranslucentBackground, false);
    m_popupFrame->setObjectName("DataBridgeSelectorBoxPopup");
    m_popupFrame->setStyleSheet(
        "#DataBridgeSelectorBoxPopup {"
        "  background: palette(Base);"
        "  border: 1px solid palette(Mid);"
        "}"
    );
    auto* layout = new QVBoxLayout(m_popupFrame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listView = new QListView(m_popupFrame);
    m_listView->setUniformItemSizes(true);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_model = new QStringListModel(m_listView);
    m_listView->setModel(m_model);
    layout->addWidget(m_listView);

    connect(m_listView, &QListView::clicked, this, &DataBridgeSelectorBox::onItemClicked);

    // 信号转发
    connect(this, &QLineEdit::textChanged, this, &DataBridgeSelectorBox::onTextChangedForward);
    connect(this, &QLineEdit::textEdited, this, &DataBridgeSelectorBox::onTextEditedForward);
    connect(this, &QLineEdit::editingFinished, this, &DataBridgeSelectorBox::onEditingFinishedInternal);

    // 自动补全
    rebuildCompleter();

    // 初始布局
    resizeEvent(nullptr);
    updateItems();
}

void DataBridgeSelectorBox::updateItems() {
    m_items =m_modelDataBridge->getAllExportRemarks();
    updateModel();
    rebuildCompleter();
}

bool DataBridgeSelectorBox::addIfNotExists(const QString& item) {
    if (m_items.contains(item, Qt::CaseSensitive)) return false;
    m_items.push_back(item);
    updateModel();
    rebuildCompleter();
    return true;
}

void DataBridgeSelectorBox::setCurrentValue(const QString& value) {
    // 若存在候选，则视为选择（不强制），否则仅设置文本
    setText(value);
    // 触发 selectionChanged，以保持旧语义
    emit selectionChanged(value);
}

void DataBridgeSelectorBox::setAllowCustomInput(bool enabled) {
    // lineedit 语义：enabled=true => 可编辑；false => 只读
    setReadOnly(!enabled);
    applyLineEditLocalStyle();
}

void DataBridgeSelectorBox::setPlaceholderText(const QString& text) {
    QLineEdit::setPlaceholderText(text);
}

void DataBridgeSelectorBox::setValidator(QValidator* validator) {
    QLineEdit::setValidator(validator);
}

void DataBridgeSelectorBox::setCompleterEnabled(bool enabled) {
    m_completerEnabled = enabled;
    rebuildCompleter();
}

void DataBridgeSelectorBox::setAutoAddOnEditingFinished(bool enabled) {
    m_autoAddOnEditingFinished = enabled;
}

void DataBridgeSelectorBox::onTogglePopup() {
    if (m_popupVisible) {
        m_popupFrame->hide();
        m_popupVisible = false;
        return;
    }
    updateItems();
    placePopup();
    m_popupFrame->show();
    m_popupVisible = true;

    // 尝试选中当前文本对应的项
    const QString txt = text();
    const int row = m_items.indexOf(txt);
    if (row >= 0) {
        m_listView->setCurrentIndex(m_model->index(row));
    } else {
        m_listView->clearSelection();
    }
}

void DataBridgeSelectorBox::onItemClicked(const QModelIndex& index) {
    if (!index.isValid()) return;
    const QString chosen = m_model->data(index, Qt::DisplayRole).toString();
    setText(chosen);
    emit selectionChanged(chosen);
    m_popupFrame->hide();
    m_popupVisible = false;
}

// 所属方法：DataBridgeSelectorBox::onEditingFinishedInternal
void DataBridgeSelectorBox::onEditingFinishedInternal() {
    // 若未启用或是只读，直接返回（默认不启用，故不会追加）
    if (!m_autoAddOnEditingFinished) return;
    if (isReadOnly()) return;

    const QString txt = text().trimmed();
    if (txt.isEmpty()) return;
    // 不自动将输入文本加入候选项；如需恢复自动追加，可取消注释下一行并将 m_autoAddOnEditingFinished 设为 true
    // addIfNotExists(txt);
}

void DataBridgeSelectorBox::onTextChangedForward(const QString& t) {
    emit selectionChanged(t);
}

void DataBridgeSelectorBox::onTextEditedForward(const QString& t) {
    emit textEdited(t);
}

void DataBridgeSelectorBox::applyLineEditLocalStyle() {
    // 保持与周边一致的样式，给右侧按钮留出内间距
    setFrame(true);
    setContentsMargins(0, 0, 0, 0);
    setStyleSheet(
        "QLineEdit {"
        "  padding-right: 24px;"  // 为右侧按钮留空间
        "}"
    );
    if (m_toggleButton) {
        m_toggleButton->setVisible(true);
        m_toggleButton->setFocusPolicy(Qt::NoFocus);
    }
}

void DataBridgeSelectorBox::rebuildCompleter() {
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

QStringList DataBridgeSelectorBox::allItems() const {
    return m_items;
}

void DataBridgeSelectorBox::updateModel() {
    m_model->setStringList(m_items);
}

void DataBridgeSelectorBox::placePopup() {
    // 将弹出面板放到控件右侧
    const int popupWidth = 240;
    const int popupHeight = qMin(240, qMax(120, m_items.size() * 24));
    const QPoint topLeftGlobal = mapToGlobal(QPoint(width(), 0));
    m_popupFrame->setGeometry(QRect(topLeftGlobal, QSize(popupWidth, popupHeight)));
}

void DataBridgeSelectorBox::resizeEvent(QResizeEvent* e) {
    QLineEdit::resizeEvent(e);
    if (!m_toggleButton) return;
    const int btnW = 20;
    const int btnH = height() - 4;
    m_toggleButton->setFixedSize(btnW, qMax(16, btnH));
    const int x = width() - btnW - 2;
    const int y = 2;
    m_toggleButton->move(x, y);

    // 若弹出层可见，尺寸变化后重新定位，避免错位
    if (m_popupVisible) {
        placePopup();
    }
}

/**
 * 控件移动事件
 * 控件位置发生变化时，若弹出层可见则重新定位。
 */
void DataBridgeSelectorBox::moveEvent(QMoveEvent* e) {
    QLineEdit::moveEvent(e);
    if (m_popupVisible) {
        placePopup();
    }
}

void DataBridgeSelectorBox::focusOutEvent(QFocusEvent* e) {
    QLineEdit::focusOutEvent(e);
    if (m_popupVisible) {
        // 若焦点离开控件与弹出层，则关闭
        if (!m_popupFrame->isActiveWindow() && !m_popupFrame->underMouse()) {
            m_popupFrame->hide();
            m_popupVisible = false;
        }
    }
}

void DataBridgeSelectorBox::keyPressEvent(QKeyEvent* e) {
    if (m_popupVisible) {
        if (e->key() == Qt::Key_Escape) {
            m_popupFrame->hide();
            m_popupVisible = false;
            e->accept();
            return;
        }
    }
    QLineEdit::keyPressEvent(e);
}