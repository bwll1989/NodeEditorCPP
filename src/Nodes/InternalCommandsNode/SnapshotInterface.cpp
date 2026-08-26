#include "SnapshotInterface.hpp"

#include <QSignalBlocker>
#include <QSizePolicy>
#include <QAbstractButton>
#include <QIcon>
#include <QSize>

namespace Nodes
{

SnapshotInterface::SnapshotInterface(QWidget *parent)
    : QWidget(parent)
    , m_buttonGroup(new QButtonGroup(this))
{
    m_buttonGroup->setExclusive(true);

    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setContentsMargins(4, 4, 4, 4);
    m_rootLayout->setSpacing(6);

    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setSpacing(4);
    m_addButton = new QPushButton(tr("添加"), this);
    m_addButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_addButton->setMinimumHeight(28);
    m_toolbarLayout->addWidget(m_addButton, 1);
    m_rootLayout->addLayout(m_toolbarLayout);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_buttonHost = new QWidget(m_scrollArea);
    m_buttonLayout = new QVBoxLayout(m_buttonHost);
    m_buttonLayout->setContentsMargins(0, 0, 0, 0);
    m_buttonLayout->setSpacing(4);
    m_buttonLayout->addStretch(1);
    m_scrollArea->setWidget(m_buttonHost);
    m_rootLayout->addWidget(m_scrollArea, 1);

    m_statusLabel = new QLabel(tr("选中节点后点预设旁「更新」保存到该预设"), this);
    m_statusLabel->setWordWrap(true);
    m_rootLayout->addWidget(m_statusLabel);

    connect(m_addButton, &QPushButton::clicked, this, &SnapshotInterface::addPresetRequested);

    setMinimumWidth(260);
    setMinimumHeight(160);
}

void SnapshotInterface::setPresets(const QStringList &names, int activeIndex)
{
    m_presetNames = names;
    m_activeIndex = activeIndex;
    rebuildButtons(names);

    if (m_activeIndex >= 0 && m_activeIndex < m_buttonGroup->buttons().size()) {
        QSignalBlocker blocker(m_buttonGroup);
        m_buttonGroup->buttons().at(m_activeIndex)->setChecked(true);
    }
}

void SnapshotInterface::setStatusText(const QString &text)
{
    m_statusLabel->setText(text);
}

void SnapshotInterface::rebuildButtons(const QStringList &names)
{
    m_updatingUi = true;
    const QSignalBlocker groupBlocker(m_buttonGroup);

    for (QAbstractButton *button : m_buttonGroup->buttons()) {
        m_buttonGroup->removeButton(button);
    }

    while (QLayoutItem *item = m_buttonLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    for (int i = 0; i < names.size(); ++i) {
        auto *row = new QWidget(m_buttonHost);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(4);

        auto *button = new QPushButton(names.at(i), row);
        button->setCheckable(true);
        button->setMinimumHeight(32);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_buttonGroup->addButton(button, i);
        connect(button, &QPushButton::clicked, this, [this, i]() {
            onPresetButtonClicked(i);
        });

        auto *updateBtn = new QPushButton(row);
        updateBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/reload.png")));
        updateBtn->setIconSize(QSize(16, 16));
        updateBtn->setFixedSize(28, 28);
        updateBtn->setFlat(true);
        updateBtn->setToolTip(tr("更新：将当前选中节点状态写入此预设"));
        connect(updateBtn, &QPushButton::clicked, this, [this, i]() {
            emit presetUpdateRequested(i);
        });

        auto *removeBtn = new QPushButton(row);
        removeBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/remove.png")));
        removeBtn->setIconSize(QSize(16, 16));
        removeBtn->setFixedSize(28, 28);
        removeBtn->setFlat(true);
        removeBtn->setToolTip(tr("删除此预设"));
        connect(removeBtn, &QPushButton::clicked, this, [this, i]() {
            emit presetRemoveRequested(i);
        });

        rowLayout->addWidget(button, 1);
        rowLayout->addWidget(updateBtn, 0);
        rowLayout->addWidget(removeBtn, 0);
        m_buttonLayout->addWidget(row);
    }

    m_buttonLayout->addStretch(1);
    m_updatingUi = false;
}

void SnapshotInterface::onPresetButtonClicked(int index)
{
    if (m_updatingUi) {
        return;
    }
    m_activeIndex = index;
    emit presetRecallRequested(index);
}

} // namespace Nodes
