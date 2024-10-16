#pragma once

#include <QHBoxLayout>
#include "QGroupBox"
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QtNodes/Definitions>
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "QtAwesome/QtAwesome.h"
using QtNodes::NodeId;
using QtNodes::PortIndex;
using QtNodes::PortType;

class CustomDataFlowGraphModel;

/**
 *                PortEditWidget
 *
 * ```
 *       _left                         _right
 *       layout                        layout
 *     ----------------------------------------
 *     |         |                  |         |
 *     | [+] [-] |                  | [+] [-] |
 *     |         |                  |         |
 *     | [+] [-] |                  | [+] [-] |
 *     |         |                  |         |
 *     | [+] [-] |                  | [+] [-] |
 *     |         |                  |         |
 *     | [+] [-] |                  |         |
 *     |         |                  |         |
 *     |_________|__________________|_________|
 * ```
 *
 * The widget has two main vertical layouts containing groups of buttons for
 * adding and removing ports. Each such a `[+] [-]` group is contained in a
 * dedicated QHVBoxLayout.
 *
 */
class PortEditAddRemoveWidget : public QWidget
{
    Q_OBJECT
public:
    PortEditAddRemoveWidget(NodeId nodeId, CustomDataFlowGraphModel &model, QWidget *parent = nullptr);

    ~PortEditAddRemoveWidget();

    /**
     * Called from constructor, creates all button groups according to models'port
     * counts.
     */
    void populateButtons(PortType portType, unsigned int nPorts);

    /**
     * Adds a single `[+][-]` button group to a given layout.
     */
    QHBoxLayout *addButtonGroupToLayout(QVBoxLayout *vbl, unsigned int portIndex);

    /**
     * Removes a single `[+][-]` button group from a given layout.
     */
    void removeButtonGroupFromLayout(QVBoxLayout *vbl, unsigned int portIndex);

private Q_SLOTS:
    void onPlusClicked();

    void onMinusClicked();

private:
    /**
     * @param buttonIndex is the index of a button in the layout.
     * Plus button has the index 0.
     * Minus button has the index 1.
     */
    std::pair<PortType, PortIndex> findWhichPortWasClicked(QObject *sender, int const buttonIndex);

    fa::QtAwesome *awesome;
    NodeId const _nodeId;
    CustomDataFlowGraphModel &_model;
public:
    QVBoxLayout *_left;

    QVBoxLayout *_right;
};
