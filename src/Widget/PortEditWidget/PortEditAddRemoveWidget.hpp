#pragma once

#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QtNodes/Definitions>
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"

using QtNodes::NodeId;
using QtNodes::PortIndex;
using QtNodes::PortType;

class CustomDataFlowGraphModel;

/**
 * PortEditWidget：左右两列 [+][-]，每行固定高度 = 端口 step
 * （与 PortAlignedColumn / DefaultHorizontalNodeGeometry 一致）。
 *
 * 每行用固定高度 QWidget 包裹，避免全局 QSS 的 padding/border 撑破行高导致累积错位。
 * 行高与 PortAlignedColumn::rowPitch() 一致。
 */
class PortEditAddRemoveWidget : public QWidget
{
    Q_OBJECT
public:
    PortEditAddRemoveWidget(NodeId nodeId, CustomDataFlowGraphModel &model, QWidget *parent = nullptr);

    ~PortEditAddRemoveWidget();

    void populateButtons(PortType portType, unsigned int nPorts);

    /** 在指定索引插入一行；返回行内按钮布局 */
    QHBoxLayout *addButtonGroupToLayout(QVBoxLayout *vbl, unsigned int portIndex);

    void removeButtonGroupFromLayout(QVBoxLayout *vbl, unsigned int portIndex);

    /** 与几何 port step 相同：fontHeight + 10 */
    static int portRowHeight();

private Q_SLOTS:
    void onPlusClicked();

    void onMinusClicked();

private:
    std::pair<PortType, PortIndex> findWhichPortWasClicked(QObject *sender, int const buttonIndex);

    NodeId const _nodeId;
    CustomDataFlowGraphModel &_model;
    int _rowHeight = 23;

public:
    QVBoxLayout *_left;
    QVBoxLayout *_right;
};
