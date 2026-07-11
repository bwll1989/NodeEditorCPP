#include "SnapshotInterface.hpp"

#include <QSignalBlocker>
#include <QSizePolicy>

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
    m_removeButton = new QPushButton(tr("删除"), this);
    m_captureButton = new QPushButton(tr("捕获选中"), this);

    const auto makeToolbarButton = [](QPushButton *button) {
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setMinimumHeight(28);
    };
    makeToolbarButton(m_addButton);
    makeToolbarButton(m_removeButton);
    makeToolbarButton(m_captureButton);

    m_toolbarLayout->addWidget(m_addButton, 1);
    m_toolbarLayout->addWidget(m_removeButton, 1);
    m_toolbarLayout->addWidget(m_captureButton, 1);
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

    m_statusLabel = new QLabel(tr("选中节点后点「捕获选中」保存当前预设"), this);
    m_statusLabel->setWordWrap(true);
    m_rootLayout->addWidget(m_statusLabel);

    connect(m_addButton, &QPushButton::clicked, this, &SnapshotInterface::addPresetRequested);
    connect(m_removeButton, &QPushButton::clicked, this, &SnapshotInterface::removePresetRequested);
    connect(m_captureButton, &QPushButton::clicked, this, &SnapshotInterface::captureRequested);

    setMinimumWidth(220);
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
        m_buttonLayout->removeWidget(button);
        button->deleteLater();
    }

    for (int i = 0; i < names.size(); ++i) {
        auto *button = new QPushButton(names.at(i), m_buttonHost);
        button->setCheckable(true);
        button->setMinimumHeight(32);
        m_buttonGroup->addButton(button, i);
        m_buttonLayout->insertWidget(i, button);
        connect(button, &QPushButton::clicked, this, [this, i]() {
            onPresetButtonClicked(i);
        });
    }

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
