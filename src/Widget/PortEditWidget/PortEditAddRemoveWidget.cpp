#include "PortEditAddRemoveWidget.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QSizePolicy>

#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"

int PortEditAddRemoveWidget::portRowHeight()
{
    // 必须与 DefaultHorizontalNodeGeometry 的 step = _portSize + _portSpasing 一致
    return QFontMetrics(QFont()).height() + 10;
}

PortEditAddRemoveWidget::PortEditAddRemoveWidget(NodeId nodeId, CustomDataFlowGraphModel &model, QWidget *parent)
    : QWidget(parent)
    , _nodeId(nodeId)
    , _model(model)
    , _rowHeight(portRowHeight())
{
    setObjectName(QStringLiteral("PortEditAddRemoveWidget"));
    setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

    // Qt Style Sheet：min-height 是内容区高度，border 叠在外面。
    // 外框要等于行高，内容尺寸必须减去左右/上下边框。
    constexpr int kBorder = 1;
    const int inner = qMax(1, _rowHeight - 2 * kBorder);
    setStyleSheet(QStringLiteral(
        "#PortEditAddRemoveWidget QPushButton {"
        "  min-height: %1px; max-height: %1px;"
        "  min-width: %1px; max-width: %1px;"
        "  padding: 0px; margin: 0px;"
        "  border: %2px solid #555; border-radius: 2px;"
        "  font-size: 8px;"
        "}").arg(inner).arg(kBorder));

    auto *hl = new QHBoxLayout(this);
    setLayout(hl);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(0);

    _left = new QVBoxLayout();
    _left->setSpacing(0);
    _left->setContentsMargins(0, 0, 0, 0);
    _left->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    _left->addStretch();

    _right = new QVBoxLayout();
    _right->setSpacing(0);
    _right->setContentsMargins(0, 0, 0, 0);
    _right->setAlignment(Qt::AlignRight | Qt::AlignTop);
    _right->addStretch();

    // 左右只占内容宽度，中间弹簧撑开（输入/输出编辑区分开）
    hl->addLayout(_left, 0);
    hl->addStretch(1);
    hl->addLayout(_right, 0);

    populateButtons(PortType::In, _model.nodeData(_nodeId, QtNodes::NodeRole::InPortCount).toInt());
    populateButtons(PortType::Out, _model.nodeData(_nodeId, QtNodes::NodeRole::OutPortCount).toInt());
}

PortEditAddRemoveWidget::~PortEditAddRemoveWidget() = default;

void PortEditAddRemoveWidget::populateButtons(PortType portType, unsigned int nPorts)
{
    QVBoxLayout *vl = (portType == PortType::In) ? _left : _right;

    // count()-1：末尾始终有一个 stretch
    while (vl->count() - 1 < static_cast<int>(nPorts)) {
        addButtonGroupToLayout(vl, 0);
    }
    while (vl->count() - 1 > static_cast<int>(nPorts)) {
        removeButtonGroupFromLayout(vl, 0);
    }
}

QHBoxLayout *PortEditAddRemoveWidget::addButtonGroupToLayout(QVBoxLayout *vbl, unsigned int portIndex)
{
    // 固定高度行容器；宽度随 +/- 紧挨的内容，不横向撑开
    auto *row = new QWidget(this);
    row->setFixedHeight(_rowHeight);
    row->setMinimumHeight(_rowHeight);
    row->setMaximumHeight(_rowHeight);
    row->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    auto *l = new QHBoxLayout(row);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(0);

    auto *plusBtn = new QPushButton(QStringLiteral("+"), row);
    // 外框 = 行高；样式里的 min-height 已减去边框，避免内容+border 超出被裁切
    plusBtn->setFixedSize(_rowHeight, _rowHeight);
    plusBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    plusBtn->setFocusPolicy(Qt::NoFocus);
    connect(plusBtn, &QPushButton::clicked, this, &PortEditAddRemoveWidget::onPlusClicked);

    auto *minusBtn = new QPushButton(QStringLiteral("-"), row);
    minusBtn->setFixedSize(_rowHeight, _rowHeight);
    minusBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    minusBtn->setFocusPolicy(Qt::NoFocus);
    connect(minusBtn, &QPushButton::clicked, this, &PortEditAddRemoveWidget::onMinusClicked);

    // 加减紧挨，不加 stretch
    l->addWidget(plusBtn, 0);
    l->addWidget(minusBtn, 0);

    vbl->insertWidget(static_cast<int>(portIndex), row);
    return l;
}

void PortEditAddRemoveWidget::removeButtonGroupFromLayout(QVBoxLayout *vbl, unsigned int portIndex)
{
    if (vbl->count() <= 1) {
        return;
    }

    QLayoutItem *item = vbl->itemAt(static_cast<int>(portIndex));
    if (!item) {
        return;
    }

    if (QWidget *row = item->widget()) {
        vbl->removeWidget(row);
        row->deleteLater();
    } else if (QLayout *inner = item->layout()) {
        // 兼容旧版直接 insertLayout 的残留
        while (QLayoutItem *child = inner->takeAt(0)) {
            if (QWidget *w = child->widget()) {
                w->deleteLater();
            }
            delete child;
        }
        vbl->removeItem(item);
        delete item;
    }
}

void PortEditAddRemoveWidget::onPlusClicked()
{
    constexpr int plusButtonIndex = 0;

    PortType portType;
    PortIndex portIndex;
    std::tie(portType, portIndex) = findWhichPortWasClicked(QObject::sender(), plusButtonIndex);

    addButtonGroupToLayout((portType == PortType::In) ? _left : _right, portIndex + 1);
    _model.addPort(_nodeId, portType, portIndex + 1);
}

void PortEditAddRemoveWidget::onMinusClicked()
{
    constexpr int minusButtonIndex = 1;

    PortType portType;
    PortIndex portIndex;
    std::tie(portType, portIndex) = findWhichPortWasClicked(QObject::sender(), minusButtonIndex);

    const int portCount = _model.nodeData(
        _nodeId,
        (portType == PortType::In) ? QtNodes::NodeRole::InPortCount
                                   : QtNodes::NodeRole::OutPortCount).toInt();
    if (portCount <= 1) {
        return;
    }

    removeButtonGroupFromLayout((portType == PortType::In) ? _left : _right, portIndex);
    _model.removePort(_nodeId, portType, portIndex);
}

std::pair<PortType, PortIndex> PortEditAddRemoveWidget::findWhichPortWasClicked(QObject *sender,
                                                                                int const buttonIndex)
{
    PortType portType = PortType::None;
    PortIndex portIndex = QtNodes::InvalidPortIndex;

    auto checkOneSide = [&](QVBoxLayout *sideLayout) {
        for (int i = 0; i < sideLayout->count(); ++i) {
            QLayoutItem *layoutItem = sideLayout->itemAt(i);
            if (!layoutItem) {
                continue;
            }

            QHBoxLayout *hLayout = nullptr;
            if (QWidget *row = layoutItem->widget()) {
                hLayout = qobject_cast<QHBoxLayout *>(row->layout());
            } else {
                hLayout = dynamic_cast<QHBoxLayout *>(layoutItem);
            }
            if (!hLayout || hLayout->count() <= buttonIndex) {
                continue;
            }

            QLayoutItem *btnItem = hLayout->itemAt(buttonIndex);
            if (btnItem && btnItem->widget() == sender) {
                portIndex = static_cast<PortIndex>(i);
                break;
            }
        }
    };

    checkOneSide(_left);
    if (portIndex != QtNodes::InvalidPortIndex) {
        portType = PortType::In;
    } else {
        checkOneSide(_right);
        if (portIndex != QtNodes::InvalidPortIndex) {
            portType = PortType::Out;
        }
    }

    return std::make_pair(portType, portIndex);
}
