//
// Created by 吴斌 on 2024/1/16.
//

#include "CustomFlowGraphicsScene.h"

#include <QMessageBox>
#include <QTableWidget>

#include "QtNodes/internal/ConnectionGraphicsObject.hpp"
#include "CustomGraphicsView.h"
#include "QtNodes/internal/GraphicsView.hpp"
#include "QtNodes/internal/NodeDelegateModelRegistry.hpp"
#include "QtNodes/internal/NodeGraphicsObject.hpp"
#include "QtNodes/internal/UndoCommands.hpp"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsSceneMoveEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidgetAction>
#include <QListWidget>
#include <QStyledItemDelegate>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>

#include <stdexcept>
#include <utility>
using QtNodes::NodeGraphicsObject;
//using QtNodes::CreateCommand;

CustomFlowGraphicsScene::CustomFlowGraphicsScene(CustomDataFlowGraphModel &graphModel, QObject *parent)
            : BasicGraphicsScene(graphModel, parent)
            , _graphModel(graphModel)
    {

        connect(&_graphModel,
                &CustomDataFlowGraphModel::inPortDataWasSet,
                [this](NodeId const nodeId, PortType const, PortIndex const) { onNodeUpdated(nodeId); });

//        connect(this,&CustomFlowGraphicsScene::nodeContextMenu,this,&CustomFlowGraphicsScene::test);
//    connect(&_graphModel,&CustomDataFlowGraphModel::nodeDeleted,this,&CustomFlowGraphicsScene::test);

    }

// TODO constructor for an empyt scene?

    std::vector<NodeId> CustomFlowGraphicsScene::selectedNodes() const
    {
        QList<QGraphicsItem *> graphicsItems = selectedItems();

        std::vector<NodeId> result;
        result.reserve(graphicsItems.size());

        for (QGraphicsItem *item : graphicsItems) {
            auto ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item);

            if (ngo != nullptr) {
                result.push_back(ngo->nodeId());

            }
        }

        return result;
    }

    void CustomFlowGraphicsScene::test(const NodeId node){
        qDebug()<<node;
}

QMenu *CustomFlowGraphicsScene::createSceneMenu(QPointF const scenePos)
{
    /**
     * @brief 创建节点添加菜单（深色主题，扁平列表，选中高亮）
     *
     * 调整：
     * - 提升列表控件的固定高度以显示更多项（默认显示约 18 行）；
     * - 保持滚动条按需显示。
     */
    QMenu *modelMenu = new QMenu();
    // 搜索框
    auto *txtBox = new QLineEdit(modelMenu);
    txtBox->setPlaceholderText(QStringLiteral("Search"));
    txtBox->setClearButtonEnabled(true);
    txtBox->addAction(QIcon(":/icons/icons/search.png"), QLineEdit::LeadingPosition);
    txtBox->setStyleSheet(
        "QLineEdit {"
        "  background:transparent;"
        "  border:1px solid #444444;"
        "  border-radius:6px;"
        "  padding:4px 8px 4px 0px;"
        "}"
        "QLineEdit:hover { border-color:#5a5a5a; }"
        "QLineEdit:focus { border-color:#6e9cfa; }"
    );
    auto *txtBoxAction = new QWidgetAction(modelMenu);
    txtBoxAction->setDefaultWidget(txtBox);
    modelMenu->addAction(txtBoxAction);

    // 扁平列表（不使用 setItemWidget，改用委托绘制选中态）
    auto *listWidget = new QListWidget(modelMenu);
    listWidget->setFrameShape(QFrame::NoFrame);
    listWidget->setUniformItemSizes(true);
    listWidget->setMouseTracking(true);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listWidget->setStyleSheet(
        "QListWidget {"
        "  background:#1e1e1e;"
        "  color:#dddddd;"
        "  outline:0;"
        "  border:none;"
        "}"
    );
    // 关键：提升菜单内列表的可视高度，减少滚动
    const int kRowHeight   = 28;   // 与委托的行高保持一致
    const int kVisibleRows = 18;   // 目标可视行数（可调）
    listWidget->setFixedHeight(kRowHeight * kVisibleRows + 8); // +8 作为上下内边距
    listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    auto *listAction = new QWidgetAction(modelMenu);
    listAction->setDefaultWidget(listWidget);
    modelMenu->addAction(listAction);
    // 菜单整体样式
    modelMenu->setStyleSheet(
        "QMenu {"
        "  background:#3c3c3c;"
        "  border:1px solid #3b3b3b;"
        "  padding:0px;"
        "}"
    );

    // 自定义委托：绘制“分组 ▸ 节点名”，并处理选中/悬停背景
    class NodeListItemDelegate : public QStyledItemDelegate {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;
        void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override {
            QStyleOptionViewItem option(opt);
            p->save();

            const QRect r = option.rect;

            // 背景：选中更深、悬停稍浅、其余透明（交由视图背景）
            if (option.state & QStyle::State_Selected) {
                p->fillRect(r, QColor("#191919")); // 深选中
            } else if (option.state & QStyle::State_MouseOver) {
                p->fillRect(r, QColor("#262626"));
            }

            // 文本内容
            const QString category = idx.data(Qt::UserRole + 1).toString();
            const QString name     = idx.data(Qt::DisplayRole).toString();

            int x = r.x() + 8;
            const int y = r.y();
            const int h = r.height();

            // 分组（灰）
            p->setPen(QColor("#b0b0b0"));
            QFont groupFont = option.font;
            QFontMetrics gm(groupFont);
            const int groupWidth = gm.horizontalAdvance(category);
            p->setFont(groupFont);
            p->drawText(QRect(x, y, groupWidth + 2, h), Qt::AlignVCenter | Qt::AlignLeft, category);
            x += groupWidth + 6;

            // 箭头（更灰）
            p->setPen(QColor("#9a9a9a"));
            const QString arrow = QStringLiteral("▸");
            const int arrowWidth = gm.horizontalAdvance(arrow);
            p->drawText(QRect(x, y, arrowWidth + 2, h), Qt::AlignVCenter | Qt::AlignLeft, arrow);
            x += arrowWidth + 6;

            // 名称（白+半粗）
            QFont nameFont = option.font;
            nameFont.setWeight(QFont::DemiBold);
            p->setFont(nameFont);
            p->setPen(QColor("#ffffff"));
            p->drawText(QRect(x, y, r.right() - x - 8, h), Qt::AlignVCenter | Qt::AlignLeft, name);

            p->restore();
        }
        QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &idx) const override {
            QSize s = QStyledItemDelegate::sizeHint(opt, idx);
            s.setHeight(28);
            return s;
        }
    };

    listWidget->setItemDelegate(new NodeListItemDelegate(listWidget));

    // 填充列表条目（分组与名称）
    auto registry = _graphModel.dataModelRegistry();
    for (const auto &assoc : registry->registeredModelsCategoryAssociation()) {
        const QString nodeName = assoc.first;
        const QString category = assoc.second;

        auto *item = new QListWidgetItem();
        item->setText(nodeName);                     // DisplayRole：节点名
        item->setData(Qt::UserRole + 1, category);   // 自定义角色：分组
        item->setSizeHint(QSize(item->sizeHint().width(), 28));
        listWidget->addItem(item);
    }

    // 点击创建节点
    connect(listWidget, &QListWidget::itemClicked,
            [this, modelMenu, scenePos](QListWidgetItem *item) {
                const QString nodeName = item->text();
                this->undoStack().push(new QtNodes::CreateCommand(this, nodeName, scenePos));
                modelMenu->close();
            });

    // 搜索：按分组或名称匹配
    connect(txtBox, &QLineEdit::textChanged, [listWidget](const QString &text) {
        const QString t = text.trimmed();
        for (int i = 0; i < listWidget->count(); ++i) {
            auto *it = listWidget->item(i);
            const QString node = it->text();
            const QString cat  = it->data(Qt::UserRole + 1).toString();
            const bool match = t.isEmpty()
                               || node.contains(t, Qt::CaseInsensitive)
                               || cat.contains(t, Qt::CaseInsensitive);
            it->setHidden(!match);
        }
    });

    txtBox->setFocus();
    modelMenu->setAttribute(Qt::WA_DeleteOnClose);
    return modelMenu;
}


    bool CustomFlowGraphicsScene::save() const
    {
        QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                        tr("Open Flow Scene"),
                                                        QDir::homePath(),
                                                        tr("Flow Scene Files (*.childflow)"));

        if (!fileName.isEmpty()) {
            if (!fileName.endsWith("childflow", Qt::CaseInsensitive))
                fileName += ".childflow";

            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(QJsonDocument(_graphModel.save()).toJson());
                return true;
            }
        }
        return false;
    }

    bool CustomFlowGraphicsScene::load()
    {
        QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                        tr("Open Child Flow "),
                                                        QDir::homePath(),
                                                        tr("Flow Scene Files (*.childflow)"));

        if (!QFileInfo::exists(fileName))
            return false;

        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(nullptr, tr("打开失败"),
                tr("无法打开文件 %1:\n%2").arg(fileName).arg(file.errorString()));
            return false;
        }



        QByteArray const wholeFile = file.readAll();
        auto jsonDoc = QJsonDocument::fromJson(wholeFile);
        if (jsonDoc.isNull()) {
            QMessageBox::warning(nullptr, tr("打开失败"),
                tr("无法解析文件 %1").arg(fileName));
            return false;
        }
        clearScene();
        _graphModel.load(jsonDoc.object());

        Q_EMIT sceneLoaded();

        return true;
}

    void CustomFlowGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {

        QPointF scenePos = event->scenePos();
        QGraphicsItem *item = itemAt(scenePos, QTransform());
        if(_graphModel.getNodesLocked()){
            return;
        }
        if (!item) {
            auto menu= createSceneMenu(event->scenePos());
            menu->exec(event->screenPos());
        }else {
            QGraphicsScene::mouseDoubleClickEvent(event);
        }
}


