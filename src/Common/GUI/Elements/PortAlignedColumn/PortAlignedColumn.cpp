#include "PortAlignedColumn.hpp"

#include <QHBoxLayout>
#include <QSizePolicy>
#include <QVBoxLayout>

PortAlignedColumn::PortAlignedColumn(QWidget *parent)
    : QWidget(parent)
    , m_rowHeight(rowPitch())
{
    setObjectName(QStringLiteral("PortAlignedColumn"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_rowsLayout = new QVBoxLayout(this);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(0);
    m_rowsLayout->addStretch(1);
    setLayout(m_rowsLayout);
}

PortAlignedColumn::~PortAlignedColumn() = default;

int PortAlignedColumn::rowCount() const
{
    return m_shells.size();
}

QWidget *PortAlignedColumn::rowContent(int index) const
{
    if (index < 0 || index >= m_contents.size()) {
        return nullptr;
    }
    return m_contents.at(index);
}

QWidget *PortAlignedColumn::makeShell(QWidget *content)
{
    auto *shell = new QWidget(this);
    shell->setFixedHeight(m_rowHeight);
    shell->setMinimumHeight(m_rowHeight);
    shell->setMaximumHeight(m_rowHeight);
    shell->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *lay = new QHBoxLayout(shell);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    if (content) {
        content->setParent(shell);
        content->setFixedHeight(m_rowHeight);
        content->setMinimumHeight(m_rowHeight);
        content->setMaximumHeight(m_rowHeight);
        content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        lay->addWidget(content, 1);
    }

    return shell;
}

void PortAlignedColumn::insertShell(int index, QWidget *shell, QWidget *content)
{
    index = qBound(0, index, m_shells.size());
    // stretch 始终在末尾
    const int layoutIndex = qMin(index, qMax(0, m_rowsLayout->count() - 1));
    m_rowsLayout->insertWidget(layoutIndex, shell);
    m_shells.insert(index, shell);
    m_contents.insert(index, content);
    updateGeometry();
    emit rowCountChanged(m_shells.size());
}

int PortAlignedColumn::appendRow(QWidget *content)
{
    const int index = m_shells.size();
    insertShell(index, makeShell(content), content);
    return index;
}

void PortAlignedColumn::insertRow(int index, QWidget *content)
{
    insertShell(index, makeShell(content), content);
}

bool PortAlignedColumn::removeRow(int index)
{
    if (index < 0 || index >= m_shells.size()) {
        return false;
    }

    QWidget *shell = m_shells.takeAt(index);
    m_contents.removeAt(index);
    m_rowsLayout->removeWidget(shell);
    shell->deleteLater();
    updateGeometry();
    emit rowCountChanged(m_shells.size());
    return true;
}

void PortAlignedColumn::clear()
{
    while (!m_shells.isEmpty()) {
        removeRow(m_shells.size() - 1);
    }
}

QSize PortAlignedColumn::sizeHint() const
{
    const int rows = qMax(1, m_shells.size());
    return QSize(200, rows * m_rowHeight);
}

QSize PortAlignedColumn::minimumSizeHint() const
{
    const int rows = qMax(1, m_shells.size());
    return QSize(120, rows * m_rowHeight);
}
