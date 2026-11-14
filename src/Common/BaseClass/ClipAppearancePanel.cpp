// ============================================================================
//  ClipAppearancePanel.cpp —— 片段外观属性通用面板实现
//  （收编自合并前 QtTimeLine 体系；2026-11-15 成员命名现代化对齐新风格，
//   行为逐行保持原实现）
// ============================================================================

#include "ClipAppearancePanel.hpp"

#include "AbstractClipModel.hpp"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
bool sameRgb(const QColor& a, const QColor& b)
{
    return a.isValid() && b.isValid() && (a.rgb() & 0x00ffffff) == (b.rgb() & 0x00ffffff);
}
}

ClipAppearancePanel::ClipAppearancePanel(AbstractClipModel* owner, QWidget* parent)
    : QGroupBox(parent)
    , owner_(owner)
{
    setTitle(tr("片段属性"));
    setFlat(true); // 带文字的分割线（2026-11-13 用户定案：四区块统一去框）

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto* colorRow = new QHBoxLayout();
    colorRow->setSpacing(4);
    colorRow->addWidget(new QLabel(tr("颜色:"), this));

    current_color_ = new QLabel(this);
    current_color_->setFixedSize(22, 22);
    colorRow->addWidget(current_color_);

    colors_ = new QButtonGroup(this);
    colors_->setExclusive(true);
    const QList<QColor> presets = titleColorPresets();
    for (int i = 0; i < presets.size(); ++i) {
        const QColor preset = presets.at(i);
        auto* button = new QToolButton(this);
        button->setCheckable(true);
        button->setFixedSize(22, 22);
        button->setToolTip(preset.name(QColor::HexRgb));
        colors_->addButton(button, i);
        colorRow->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, preset]() {
            if (syncing_ || !owner_)
                return;
            owner_->setColor(preset);
        });
    }
    colorRow->addStretch(1);
    layout->addLayout(colorRow);

    auto* noteRow = new QHBoxLayout();
    noteRow->setSpacing(4);
    noteRow->addWidget(new QLabel(tr("备注名:"), this));
    note_edit_ = new QLineEdit(this);
    note_edit_->setPlaceholderText(tr("留空则显示类型名"));
    noteRow->addWidget(note_edit_, 1);
    layout->addLayout(noteRow);

    connect(note_edit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (syncing_ || !owner_)
            return;
        owner_->setNoteName(text);
    });
    if (owner_) {
        connect(owner_, &AbstractClipModel::appearanceChanged, this, [this]() {
            syncFromOwner();
        });
    }
    syncFromOwner();
}

void ClipAppearancePanel::changeEvent(QEvent* event)
{
    QGroupBox::changeEvent(event);
    // QSS（qproperty-*）重新生效后，用新的边框颜色重建色块样式
    if (event->type() == QEvent::StyleChange) {
        syncFromOwner();
    }
}

void ClipAppearancePanel::syncFromOwner()
{
    if (!owner_)
        return;
    syncing_ = true;

    const QColor current = owner_->color();

    // 当前颜色指示块：背景为片段颜色数据，边框样式来自 QSS 可配置属性
    current_color_->setStyleSheet(QStringLiteral(
        "background-color: %1; border: 1px solid %2; border-radius: 2px;")
        .arg(current.name(QColor::HexRgb),
             current_color_border_color_.name(QColor::HexRgb)));

    // 预设色块：背景为预设颜色数据，边框样式来自 QSS 可配置属性
    const QList<QColor> presets = titleColorPresets();
    const QList<QAbstractButton*> buttons = colors_->buttons();
    for (int i = 0; i < buttons.size() && i < presets.size(); ++i) {
        buttons.at(i)->setStyleSheet(QStringLiteral(
            "QToolButton { background-color: %1; border: 1px solid %2; border-radius: 2px; }"
            "QToolButton:checked { border: 2px solid %3; }")
            .arg(presets.at(i).name(QColor::HexRgb),
                 swatch_border_color_.name(QColor::HexRgb),
                 swatch_checked_border_color_.name(QColor::HexRgb)));
    }

    int matched = -1;
    if (owner_->hasCustomColor()) {
        for (int i = 0; i < presets.size(); ++i) {
            if (sameRgb(presets.at(i), current)) {
                matched = i;
                break;
            }
        }
    }
    colors_->setExclusive(false);
    for (QAbstractButton* button : buttons)
        button->setChecked(false);
    if (matched >= 0) {
        if (QAbstractButton* button = colors_->button(matched))
            button->setChecked(true);
    }
    colors_->setExclusive(true);

    {
        QSignalBlocker blocker(note_edit_);
        if (note_edit_->text() != owner_->noteName())
            note_edit_->setText(owner_->noteName());
    }
    syncing_ = false;
}
