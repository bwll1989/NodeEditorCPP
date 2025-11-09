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

/**
 * 创建横向标签页式的场景菜单（使用表格布局，从上到下从左到右排列，每列12项）
 * @param scenePos 场景位置
 * @return 返回创建的菜单指针
 */
QMenu *CustomFlowGraphicsScene::createSceneMenu(QPointF const scenePos)
{
    QMenu *modelMenu = new QMenu();
    // auto const &flowViewStyle = StyleCollection::flowViewStyle();
    // 创建主容器widget
    QWidget *mainWidget = new QWidget(modelMenu);
    mainWidget->setMinimumSize(600, 400);

    // 创建垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 添加搜索框
    QLineEdit *txtBox = new QLineEdit(mainWidget);
    txtBox->setPlaceholderText(QStringLiteral("Filter"));
    txtBox->setClearButtonEnabled(true);
    txtBox->setFixedHeight(30);
    mainLayout->addWidget(txtBox);

    // 创建标签页控件
    QTabWidget *tabWidget = new QTabWidget(mainWidget);
    tabWidget->setTabPosition(QTabWidget::North);
    tabWidget->setDocumentMode(true);
    tabWidget->setMovable(false);
    tabWidget->setUsesScrollButtons(false);
    mainLayout->addWidget(tabWidget);

    // 获取注册表
    auto registry = _graphModel.dataModelRegistry();

    // 为每个分类创建标签页
    QMap<QString, QTableWidget*> categoryWidgets;

    for (auto const &cat : registry->categories()) {
        // 创建表格控件
        QTableWidget *tableWidget = new QTableWidget();

        // 设置表格属性 - 每列12项，所以固定12行
        tableWidget->setRowCount(12);
        tableWidget->setColumnCount(1); // 先设置1列，后续根据数据动态调整
        tableWidget->setHorizontalHeaderLabels(QStringList() << "");
        tableWidget->horizontalHeader()->setVisible(false); // 隐藏列标题
        tableWidget->verticalHeader()->setVisible(false);   // 隐藏行标题
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setAlternatingRowColors(false);
        tableWidget->setShowGrid(true);
        tableWidget->setGridStyle(Qt::NoPen);
        // 设置单元格大小
        tableWidget->verticalHeader()->setDefaultSectionSize(30);   // 行高30像素
        tableWidget->horizontalHeader()->setDefaultSectionSize(150); // 列宽180像素
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

        // 添加到标签页
        tabWidget->addTab(tableWidget, cat);
        categoryWidgets[cat] = tableWidget;
        // 设置滚动条策略
        tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        // 连接点击事件
        connect(tableWidget, &QTableWidget::itemClicked,
                [this, modelMenu, scenePos](QTableWidgetItem *item) {
                    if (!item || item->text().isEmpty()) return;

                    // 检查项目是否可选择
                    if (!(item->flags() & Qt::ItemIsSelectable)) return;

                    this->undoStack().push(new QtNodes::CreateCommand(this, item->text(), scenePos));
                    modelMenu->close();
                });
    }

    // 填充模型到对应的分类标签页
    QMap<QString, QStringList> categoryModels;

    // 先收集每个分类的所有模型
    for (auto const &assoc : registry->registeredModelsCategoryAssociation()) {
        QString modelName = assoc.first;
        QString categoryName = assoc.second;
        categoryModels[categoryName].append(modelName);
    }

   for (auto it = categoryModels.begin(); it != categoryModels.end(); ++it) {
    QString categoryName = it.key();
    QStringList models = it.value();

    if (categoryWidgets.contains(categoryName)) {
        QTableWidget *tableWidget = categoryWidgets[categoryName];
        int itemsPerColumn = 12;
        int columnCount = (models.size() + itemsPerColumn - 1) / itemsPerColumn;

        // // 设置表格尺寸
        // tableWidget->setRowCount(itemsPerColumn);  // 添加这行：设置行数
        // tableWidget->setColumnCount(columnCount);
        tableWidget->clear();

        // 重新设置表格尺寸（clear()可能会重置尺寸）
        tableWidget->setRowCount(itemsPerColumn);
        tableWidget->setColumnCount(columnCount);
        // 设置滚动条策略
        for (int i = 0; i < models.size(); ++i) {
            int col = i / itemsPerColumn;  // 列索引
            int row = i % itemsPerColumn;  // 行索引

            QTableWidgetItem *item = new QTableWidgetItem(models[i]);
            item->setTextAlignment(Qt::AlignCenter);
            item->setToolTip(models[i]); // 添加工具提示

            // 设置项目标志
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

            // 设置默认颜色
            item->setBackground(QBrush(QColor(43, 43, 43))); // 正常背景色
            item->setForeground(QBrush(QColor(255, 255, 255))); // 白色文字

            tableWidget->setItem(row, col, item);
        }
    }
}

    connect(txtBox, &QLineEdit::textChanged, [tabWidget, categoryWidgets](const QString &text) {
        for (auto it = categoryWidgets.begin(); it != categoryWidgets.end(); ++it) {
            QTableWidget *tableWidget = it.value();
            int visibleCount = 0;

            // 遍历所有单元格进行过滤
            for (int row = 0; row < tableWidget->rowCount(); ++row) {
                for (int col = 0; col < tableWidget->columnCount(); ++col) {
                    QTableWidgetItem *item = tableWidget->item(row, col);
                    if (item && !item->text().isEmpty()) {
                        bool match = text.isEmpty() || item->text().contains(text, Qt::CaseInsensitive);

                        if (match) {
                            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                            item->setBackground(QBrush(QColor(43, 43, 43))); // 正常背景色
                            item->setForeground(QBrush(QColor(255, 255, 255))); // 白色文字
                            visibleCount++;
                        } else {
                            item->setFlags(Qt::ItemIsEnabled); // 移除可选择标志
                            item->setBackground(QBrush(QColor(25, 25, 25))); // 更暗的背景色
                            item->setForeground(QBrush(QColor(100, 100, 100))); // 灰色文字
                        }
                    }
                }
            }

            int tabIndex = tabWidget->indexOf(tableWidget);
            if (tabIndex >= 0) {
                QString tabText = it.key();
                if (!text.isEmpty()) {
                    tabWidget->setTabText(tabIndex, QString("%1 (%2)").arg(tabText).arg(visibleCount));
                } else {
                    tabWidget->setTabText(tabIndex, tabText);
                }
            }
        }
    });

    // 创建菜单动作并设置主widget
    QWidgetAction *mainAction = new QWidgetAction(modelMenu);
    mainAction->setDefaultWidget(mainWidget);
    modelMenu->addAction(mainAction);

    // 设置焦点到搜索框
    txtBox->setFocus();

    // 设置菜单属性
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


