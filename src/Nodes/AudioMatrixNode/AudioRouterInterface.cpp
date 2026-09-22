#include "AudioRouterInterface.h"

#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QSignalBlocker>
#include <QtGlobal>

using namespace Nodes;

namespace {

QIcon makeEllipsisIcon(const QColor &color)
{
    constexpr int s = 16;
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    for (int i = 0; i < 3; ++i) {
        p.drawEllipse(QPointF(3.5 + i * 4.5, s * 0.5), 1.6, 1.6);
    }
    return QIcon(pm);
}

QIcon makeIdentityIcon(const QColor &color)
{
    constexpr int s = 16;
    QPixmap pm(s, s);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    for (int i = 0; i < 3; ++i) {
        p.drawRoundedRect(QRectF(2.5 + i * 4.0, 2.5 + i * 4.0, 3.5, 3.5), 0.8, 0.8);
    }
    return QIcon(pm);
}

} // namespace

RouterMatrixWidget::RouterMatrixWidget(int rows, int cols, QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    resizeMatrix(qMax(1, rows), qMax(1, cols));
}

void RouterMatrixWidget::resizeMatrix(int rows, int cols)
{
    rows = qMax(1, rows);
    cols = qMax(1, cols);
    const bool sizeChanged = (rows != m_rows) || (cols != m_cols) || m_buttons.isEmpty();
    if (!sizeChanged) {
        return;
    }

    const QVector<int> oldMap = routingMap();
    m_rows = rows;
    m_cols = cols;
    rebuildButtons();

    QVector<int> restored(m_cols, -1);
    if (oldMap.isEmpty()) {
        for (int i = 0; i < qMin(m_rows, m_cols); ++i) {
            restored[i] = i;
        }
    } else {
        for (int c = 0; c < m_cols; ++c) {
            if (c < oldMap.size()) {
                const int src = oldMap[c];
                if (src >= 0 && src < m_rows) {
                    restored[c] = src;
                }
            }
        }
    }
    setRoutingMap(restored);
    updateGeometry();
}

void RouterMatrixWidget::rebuildButtons()
{
    // QGridLayout 删格后行列 stretch 不会收缩，缩小端口时会留下空列，
    // 格子挤到左边而顶栏 out 标签仍均分整宽，造成缩放后列不对齐。
    for (QToolButton *btn : m_buttons) {
        if (btn) {
            if (m_layout) {
                m_layout->removeWidget(btn);
            }
            btn->deleteLater();
        }
    }
    m_buttons.clear();

    if (m_layout) {
        while (QLayoutItem *item = m_layout->takeAt(0)) {
            delete item;
        }
        delete m_layout;
        m_layout = nullptr;
    }

    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(AudioRouterInterface::kGridSpacing);
    m_layout->setHorizontalSpacing(AudioRouterInterface::kGridSpacing);
    m_layout->setVerticalSpacing(AudioRouterInterface::kGridSpacing);

    m_buttons.resize(m_rows * m_cols);
    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            auto *btn = new QToolButton(this);
            btn->setCheckable(true);
            btn->setAutoRaise(false);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            btn->setMinimumSize(18, 18);
            btn->setFocusPolicy(Qt::NoFocus);
            btn->setToolTip(QStringLiteral("IN%1 → OUT%2").arg(r + 1).arg(c + 1));
            btn->setStyleSheet(QStringLiteral(
                "QToolButton {"
                "  border: 1px solid palette(mid);"
                "  background: palette(base);"
                "  border-radius: 2px;"
                "}"
                "QToolButton:checked {"
                "  background: #3d7ea6;"
                "  border-color: #2a5f80;"
                "}"
                "QToolButton:hover {"
                "  border-color: palette(highlight);"
                "}"));

            m_layout->addWidget(btn, r, c);
            m_buttons[r * m_cols + c] = btn;

            // 每列最多一路：选中时清掉同列其他格；再点可断开
            connect(btn, &QToolButton::toggled, this, [this, r, c](bool checked) {
                if (checked) {
                    for (int other = 0; other < m_rows; ++other) {
                        if (other == r) {
                            continue;
                        }
                        if (QToolButton *otherBtn = buttonAt(other, c)) {
                            const QSignalBlocker blocker(otherBtn);
                            otherBtn->setChecked(false);
                        }
                    }
                }
                emitRouting();
            });
        }
        m_layout->setRowStretch(r, 1);
    }
    for (int c = 0; c < m_cols; ++c) {
        m_layout->setColumnStretch(c, 1);
    }
}

QToolButton *RouterMatrixWidget::buttonAt(int row, int col) const
{
    if (row < 0 || col < 0 || row >= m_rows || col >= m_cols) {
        return nullptr;
    }
    return m_buttons[row * m_cols + col];
}

QVector<int> RouterMatrixWidget::routingMap() const
{
    QVector<int> map(m_cols, -1);
    for (int c = 0; c < m_cols; ++c) {
        for (int r = 0; r < m_rows; ++r) {
            if (QToolButton *btn = buttonAt(r, c); btn && btn->isChecked()) {
                map[c] = r;
                break;
            }
        }
    }
    return map;
}

void RouterMatrixWidget::setRoutingMap(const QVector<int> &map)
{
    for (int c = 0; c < m_cols; ++c) {
        const int src = (c < map.size()) ? map[c] : -1;
        for (int r = 0; r < m_rows; ++r) {
            if (QToolButton *btn = buttonAt(r, c)) {
                const QSignalBlocker blocker(btn);
                btn->setChecked(r == src);
            }
        }
    }
    emitRouting();
}

void RouterMatrixWidget::setIdentity()
{
    QVector<int> map(m_cols, -1);
    for (int i = 0; i < qMin(m_rows, m_cols); ++i) {
        map[i] = i;
    }
    setRoutingMap(map);
}

void RouterMatrixWidget::clearAll()
{
    setRoutingMap(QVector<int>(m_cols, -1));
}

void RouterMatrixWidget::emitRouting()
{
    emit routingChanged(routingMap());
}

AudioRouterInterface::AudioRouterInterface(int inputCount, int outputCount, QWidget *parent)
    : QWidget(parent)
    , mRouterWidget(new RouterMatrixWidget(qMax(kMinChannels, inputCount),
                                           qMax(kMinChannels, outputCount)))
    , mMenuButton(new QToolButton())
    , mCornerPanel(new QWidget(this))
    , mOutHeader(new QWidget(this))
    , mInHeader(new QWidget(this))
    , mOutHeaderLayout(new QHBoxLayout(mOutHeader))
    , mInHeaderLayout(new QVBoxLayout(mInHeader))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const QColor iconColor = palette().color(QPalette::ButtonText);

    mMenuButton->setIcon(makeEllipsisIcon(iconColor));
    mMenuButton->setIconSize(QSize(14, 14));
    mMenuButton->setText({});
    mMenuButton->setAutoRaise(true);
    mMenuButton->setPopupMode(QToolButton::InstantPopup);
    mMenuButton->setFixedSize(24, 24);
    mMenuButton->setFocusPolicy(Qt::NoFocus);
    mMenuButton->setToolTip(QStringLiteral("路由操作"));

    auto *menu = new QMenu(mMenuButton);
    QAction *identityAct = menu->addAction(makeIdentityIcon(iconColor),
                                           QStringLiteral("1:1 直通"));
    identityAct->setToolTip(QStringLiteral("对角线接通，其余断开"));
    QAction *clearAct = menu->addAction(QIcon(QStringLiteral(":/icons/icons/clear.png")),
                                        QStringLiteral("全部断开"));
    clearAct->setToolTip(QStringLiteral("清除所有路由"));
    mMenuButton->setMenu(menu);

    connect(identityAct, &QAction::triggered, mRouterWidget, &RouterMatrixWidget::setIdentity);
    connect(clearAct, &QAction::triggered, mRouterWidget, &RouterMatrixWidget::clearAll);

    auto *cornerLayout = new QVBoxLayout(mCornerPanel);
    cornerLayout->setContentsMargins(0, 0, 0, 0);
    cornerLayout->setSpacing(0);
    cornerLayout->addStretch(1);
    cornerLayout->addWidget(mMenuButton, 0, Qt::AlignCenter);
    cornerLayout->addStretch(1);

    mCornerPanel->setFixedWidth(kAxisWidth);
    mInHeader->setFixedWidth(kAxisWidth);
    mCornerPanel->setMinimumHeight(kOutHeaderHeight);
    mOutHeader->setMinimumHeight(kOutHeaderHeight);
    mCornerPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    mOutHeader->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    mInHeader->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    mCornerPanel->setAutoFillBackground(true);
    mOutHeader->setAutoFillBackground(true);
    mInHeader->setAutoFillBackground(true);

    mOutHeaderLayout->setContentsMargins(0, 0, 0, 0);
    mOutHeaderLayout->setSpacing(kGridSpacing);
    mInHeaderLayout->setContentsMargins(0, 0, 0, 0);
    mInHeaderLayout->setSpacing(kGridSpacing);

    auto *root = new QGridLayout(this);
    root->setContentsMargins(2, 2, 2, 2);
    root->setHorizontalSpacing(kGridSpacing);
    root->setVerticalSpacing(kGridSpacing);
    root->addWidget(mCornerPanel, 0, 0);
    root->addWidget(mOutHeader, 0, 1);
    root->addWidget(mInHeader, 1, 0);
    root->addWidget(mRouterWidget, 1, 1);
    root->setColumnStretch(0, 0);
    root->setColumnStretch(1, 1);
    root->setRowStretch(0, 0);
    root->setRowStretch(1, 1);

    rebuildAxisLabels();
    updateMinimumSizeForChannels();
}

int AudioRouterInterface::inputCount() const
{
    return mRouterWidget->getRows();
}

int AudioRouterInterface::outputCount() const
{
    return mRouterWidget->getCols();
}

void AudioRouterInterface::setChannelCounts(int inputCount, int outputCount)
{
    inputCount = qMax(kMinChannels, inputCount);
    outputCount = qMax(kMinChannels, outputCount);

    const bool sizeChanged = (inputCount != mRouterWidget->getRows())
                          || (outputCount != mRouterWidget->getCols());
    if (!sizeChanged) {
        return;
    }

    mRouterWidget->resizeMatrix(inputCount, outputCount);
    rebuildAxisLabels();
    updateMinimumSizeForChannels();
    updateGeometry();
    adjustSize();
}

void AudioRouterInterface::rebuildAxisLabels()
{
    while (QLayoutItem *item = mOutHeaderLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            delete w;
        }
        delete item;
    }
    while (QLayoutItem *item = mInHeaderLayout->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            delete w;
        }
        delete item;
    }

    const int rows = mRouterWidget->getRows();
    const int cols = mRouterWidget->getCols();

    for (int c = 0; c < cols; ++c) {
        const QString name = QStringLiteral("out %1").arg(c + 1);
        auto *label = new RouterVerticalTextLabel(name, mOutHeader);
        mOutHeaderLayout->addWidget(label, 1);
    }

    for (int r = 0; r < rows; ++r) {
        const QString name = QStringLiteral("in %1").arg(r + 1);
        auto *label = new QLabel(name, mInHeader);
        label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        label->setToolTip(name);
        label->setMargin(4);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        mInHeaderLayout->addWidget(label, 1);
    }
}

void AudioRouterInterface::updateMinimumSizeForChannels()
{
    constexpr int kCellMin = 18;
    constexpr int kPad = 8;

    const int cols = mRouterWidget->getCols();
    const int rows = mRouterWidget->getRows();
    setMinimumSize(kAxisWidth + cols * kCellMin + kPad,
                   kOutHeaderHeight + rows * kCellMin + kPad);
}
