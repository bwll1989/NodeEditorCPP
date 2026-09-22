#include "AudioMatrixInterface.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSizePolicy>
#include <QMenu>
#include <QAction>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPen>

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

AudioMatrixInterface::AudioMatrixInterface(int inputCount, int outputCount, QWidget *parent)
    : QWidget(parent)
    , mMatrixWidget(new MatrixWidget(qMax(kMinChannels, inputCount),
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
    mMenuButton->setToolTip(QStringLiteral("矩阵操作"));

    auto *menu = new QMenu(mMenuButton);
    QAction *identityAct = menu->addAction(makeIdentityIcon(iconColor),
                                           QStringLiteral("1:1 直通"));
    identityAct->setToolTip(QStringLiteral("对角线 0 dB，其余静音"));
    QAction *muteAct = menu->addAction(QIcon(QStringLiteral(":/icons/icons/clear.png")),
                                       QStringLiteral("全部静音"));
    muteAct->setToolTip(QStringLiteral("全部设为 -60 dB"));
    mMenuButton->setMenu(menu);

    connect(identityAct, &QAction::triggered, mMatrixWidget, &MatrixWidget::setIdentity);
    connect(muteAct, &QAction::triggered, mMatrixWidget, &MatrixWidget::resetValues);

    auto *cornerLayout = new QVBoxLayout(mCornerPanel);
    cornerLayout->setContentsMargins(0, 0, 0, 0);
    cornerLayout->setSpacing(0);
    cornerLayout->addStretch(1);
    cornerLayout->addWidget(mMenuButton, 0, Qt::AlignCenter);
    cornerLayout->addStretch(1);

    // 与左侧 in 列同宽，否则顶栏 out 列会整体错位
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

    // 间距与 MatrixWidget 内部一致，保证列宽对齐
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
    root->addWidget(mMatrixWidget, 1, 1);
    root->setColumnStretch(0, 0);
    root->setColumnStretch(1, 1);
    root->setRowStretch(0, 0);
    root->setRowStretch(1, 1);

    rebuildAxisLabels();
    updateMinimumSizeForChannels();
}

int AudioMatrixInterface::inputCount() const
{
    return mMatrixWidget->getRows();
}

int AudioMatrixInterface::outputCount() const
{
    return mMatrixWidget->getCols();
}

void AudioMatrixInterface::setChannelCounts(int inputCount, int outputCount)
{
    inputCount = qMax(kMinChannels, inputCount);
    outputCount = qMax(kMinChannels, outputCount);

    const bool sizeChanged = (inputCount != mMatrixWidget->getRows())
                          || (outputCount != mMatrixWidget->getCols());

    if (sizeChanged) {
        mMatrixWidget->resizeMatrix(inputCount, outputCount);
        rebuildAxisLabels();
        updateMinimumSizeForChannels();
        updateGeometry();
        adjustSize();
    }
}

void AudioMatrixInterface::rebuildAxisLabels()
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

    const int rows = mMatrixWidget->getRows();
    const int cols = mMatrixWidget->getCols();

    for (int c = 0; c < cols; ++c) {
        const QString name = QStringLiteral("out %1").arg(c + 1);
        auto *label = new VerticalTextLabel(name, mOutHeader);
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

void AudioMatrixInterface::updateMinimumSizeForChannels()
{
    constexpr int kCellMin = 18;
    constexpr int kPad = 8;

    const int cols = mMatrixWidget->getCols();
    const int rows = mMatrixWidget->getRows();
    setMinimumSize(kAxisWidth + cols * kCellMin + kPad,
                   kOutHeaderHeight + rows * kCellMin + kPad);
}
