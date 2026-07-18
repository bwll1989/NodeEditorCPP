#include "NodeCreateSceneMenu.hpp"

#include "QtNodes/internal/BasicGraphicsScene.hpp"
#include "QtNodes/internal/NodeDelegateModelRegistry.hpp"
#include "QtNodes/internal/UndoCommands.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QHash>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QWidgetAction>

#include <algorithm>
#include <memory>
#include <utility>

namespace {

struct NodeHelpInfo
{
    QString htmlRel;
    QString summary;
};

class FixedHeightDelegate : public QStyledItemDelegate
{
public:
    explicit FixedHeightDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(28);
        return size;
    }
};

QString findHelpRootDir()
{
    const QStringList candidates = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/html"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../html"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../docs/html"),
        QDir::currentPath() + QStringLiteral("/docs/html"),
        QDir::currentPath() + QStringLiteral("/html"),
    };
    for (const QString &dir : candidates) {
        const QString indexPath = QDir(dir).filePath(QStringLiteral("help-index.json"));
        if (QFile::exists(indexPath)) {
            return QDir(dir).absolutePath();
        }
    }
    return {};
}

const QHash<QString, NodeHelpInfo> &nodeHelpIndex()
{
    static QHash<QString, NodeHelpInfo> cache;
    static bool loaded = false;
    if (loaded) {
        return cache;
    }
    loaded = true;

    const QString root = findHelpRootDir();
    if (root.isEmpty()) {
        return cache;
    }

    QFile file(QDir(root).filePath(QStringLiteral("help-index.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return cache;
    }

    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        NodeHelpInfo info;
        info.htmlRel = entry.value(QStringLiteral("html")).toString();
        info.summary = entry.value(QStringLiteral("summary")).toString();
        cache.insert(it.key(), info);
    }
    return cache;
}

NodeHelpInfo lookupNodeHelp(const QString &nodeName)
{
    const auto &index = nodeHelpIndex();
    if (auto it = index.constFind(nodeName); it != index.cend()) {
        return it.value();
    }
    const QString compact = QString(nodeName).remove(QLatin1Char(' '));
    if (auto it = index.constFind(compact); it != index.cend()) {
        return it.value();
    }
    return {};
}

QString trMenu(const char *text)
{
    return QCoreApplication::translate("NodeCreateSceneMenu", text);
}

} // namespace

namespace NodeCreateSceneMenu {

QMenu *create(QtNodes::BasicGraphicsScene *scene,
              std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry,
              QPointF const &scenePos)
{
    auto *modelMenu = new QMenu();
    modelMenu->setWindowFlags(modelMenu->windowFlags() | Qt::NoDropShadowWindowHint);
    modelMenu->setAttribute(Qt::WA_TranslucentBackground, false);

    auto *container = new QWidget;
    auto *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    auto *txtBox = new QLineEdit;
    txtBox->setPlaceholderText(trMenu("搜索节点"));
    txtBox->setClearButtonEnabled(true);
    auto *searchIconAction = new QAction(QIcon(":/icons/icons/search.png"), trMenu("搜索"), txtBox);
    txtBox->addAction(searchIconAction, QLineEdit::LeadingPosition);
    searchIconAction->setEnabled(false);
    mainLayout->addWidget(txtBox);

    auto *columnsWidget = new QWidget;
    auto *columnsLayout = new QHBoxLayout(columnsWidget);
    columnsLayout->setContentsMargins(0, 0, 0, 0);
    columnsLayout->setSpacing(4);

    auto *categoryView = new QListView;
    auto *nodeView = new QListView;
    categoryView->setItemDelegate(new FixedHeightDelegate(categoryView));
    nodeView->setItemDelegate(new FixedHeightDelegate(nodeView));

    auto *categoriesModel = new QStringListModel(categoryView);
    auto *nodesModel = new QStringListModel(nodeView);

    categoryView->setFixedWidth(120);
    nodeView->setMinimumWidth(200);
    nodeView->setMinimumHeight(300);
    categoryView->setSelectionMode(QAbstractItemView::SingleSelection);
    nodeView->setSelectionMode(QAbstractItemView::SingleSelection);
    nodeView->setMouseTracking(true);
    nodeView->viewport()->setMouseTracking(true);
    categoryView->setModel(categoriesModel);
    nodeView->setModel(nodesModel);

    auto *descFrame = new QFrame;
    descFrame->setFrameShape(QFrame::StyledPanel);
    descFrame->setFixedWidth(240);
    descFrame->setMinimumHeight(300);
    auto *descLayout = new QVBoxLayout(descFrame);
    descLayout->setContentsMargins(8, 8, 8, 8);
    descLayout->setSpacing(8);

    auto *descTitle = new QLabel(trMenu("节点说明"));
    descTitle->setWordWrap(true);
    QFont titleFont = descTitle->font();
    titleFont.setBold(true);
    descTitle->setFont(titleFont);

    auto *descSummary = new QLabel(trMenu("将鼠标移到节点上查看说明"));
    descSummary->setWordWrap(true);
    descSummary->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    descSummary->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *openHelpLink = new QLabel;
    openHelpLink->setTextFormat(Qt::RichText);
    openHelpLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
    openHelpLink->setOpenExternalLinks(false);
    openHelpLink->setVisible(false);

    descLayout->addWidget(descTitle);
    descLayout->addWidget(descSummary, 1);
    descLayout->addWidget(openHelpLink);

    columnsLayout->addWidget(categoryView);
    columnsLayout->addWidget(nodeView);
    columnsLayout->addWidget(descFrame);
    mainLayout->addWidget(columnsWidget);

    const auto associations = registry->registeredModelsCategoryAssociation();
    QStringList catList;
    for (const auto &cat : registry->categories()) {
        catList << cat;
    }
    categoriesModel->setStringList(catList);

    auto showNodeHelp = [descTitle, descSummary, openHelpLink](const QString &name) {
        if (name.isEmpty()) {
            descTitle->setText(trMenu("节点说明"));
            descSummary->setText(trMenu("将鼠标移到节点上查看说明"));
            openHelpLink->clear();
            openHelpLink->setVisible(false);
            openHelpLink->setProperty("helpPath", QString());
            return;
        }

        descTitle->setText(name);
        const NodeHelpInfo info = lookupNodeHelp(name);
        if (!info.summary.isEmpty()) {
            descSummary->setText(info.summary);
        } else {
            descSummary->setText(
                trMenu("暂无文档摘要。可构建帮助站点后重试（docs/build_mkdocs.py）。"));
        }

        const QString helpRoot = findHelpRootDir();
        QString htmlPath;
        if (!helpRoot.isEmpty() && !info.htmlRel.isEmpty()) {
            htmlPath = QDir(helpRoot).filePath(info.htmlRel);
        }
        if (!htmlPath.isEmpty() && QFile::exists(htmlPath)) {
            openHelpLink->setText(
                QStringLiteral("<a href=\"help\" style=\"color:#81a4b2;\">%1</a>")
                    .arg(trMenu("打开完整帮助")));
            openHelpLink->setProperty("helpPath", htmlPath);
            openHelpLink->setVisible(true);
        } else {
            openHelpLink->clear();
            openHelpLink->setProperty("helpPath", QString());
            openHelpLink->setVisible(false);
        }
    };

    QObject::connect(openHelpLink, &QLabel::linkActivated, openHelpLink, [openHelpLink](const QString &) {
        const QString path = openHelpLink->property("helpPath").toString();
        if (!path.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });

    auto updateNodeList = [nodesModel, nodeView, associations, showNodeHelp](const QString &category,
                                                                             const QString &filter) {
        QStringList items;
        const bool isSearching = !filter.isEmpty();

        for (const auto &pair : associations) {
            const QString &name = pair.first;
            const QString &cat = pair.second;
            if (isSearching) {
                if (name.contains(filter, Qt::CaseInsensitive)) {
                    items << name;
                }
            } else if (cat == category) {
                items << name;
            }
        }

        std::sort(items.begin(), items.end(), [](const QString &a, const QString &b) {
            return QString::localeAwareCompare(a, b) < 0;
        });
        nodesModel->setStringList(items);

        if (!items.isEmpty()) {
            nodeView->setCurrentIndex(nodesModel->index(0, 0));
            showNodeHelp(items.first());
        } else {
            showNodeHelp(QString());
        }
    };

    QObject::connect(categoryView, &QListView::clicked,
                     [categoriesModel, updateNodeList, txtBox](const QModelIndex &index) {
                         if (!txtBox->text().isEmpty()) {
                             txtBox->blockSignals(true);
                             txtBox->clear();
                             txtBox->blockSignals(false);
                         }
                         updateNodeList(categoriesModel->data(index, Qt::DisplayRole).toString(),
                                        QString());
                     });

    QObject::connect(txtBox, &QLineEdit::textChanged,
                     [categoryView, categoriesModel, nodesModel, updateNodeList, showNodeHelp](
                         const QString &text) {
                         if (text.isEmpty()) {
                             categoryView->setVisible(true);
                             categoryView->setEnabled(true);
                             const QModelIndex current = categoryView->currentIndex();
                             if (current.isValid()) {
                                 updateNodeList(
                                     categoriesModel->data(current, Qt::DisplayRole).toString(),
                                     QString());
                             } else if (categoriesModel->rowCount() > 0) {
                                 const QModelIndex first = categoriesModel->index(0, 0);
                                 categoryView->setCurrentIndex(first);
                                 updateNodeList(
                                     categoriesModel->data(first, Qt::DisplayRole).toString(),
                                     QString());
                             } else {
                                 nodesModel->setStringList(QStringList());
                                 showNodeHelp(QString());
                             }
                         } else {
                             categoryView->setVisible(false);
                             updateNodeList(QString(), text);
                         }
                     });

    QObject::connect(nodeView, &QAbstractItemView::entered,
                     [nodesModel, showNodeHelp](const QModelIndex &index) {
                         if (index.isValid()) {
                             showNodeHelp(nodesModel->data(index, Qt::DisplayRole).toString());
                         }
                     });

    QObject::connect(nodeView->selectionModel(), &QItemSelectionModel::currentChanged,
                     [nodesModel, showNodeHelp](const QModelIndex &current, const QModelIndex &) {
                         if (current.isValid()) {
                             showNodeHelp(nodesModel->data(current, Qt::DisplayRole).toString());
                         }
                     });

    QObject::connect(nodeView, &QListView::clicked,
                     [scene, nodesModel, modelMenu, scenePos](const QModelIndex &index) {
                         const QString name = nodesModel->data(index, Qt::DisplayRole).toString();
                         if (!name.isEmpty() && scene) {
                             scene->undoStack().push(
                                 new QtNodes::CreateCommand(scene, name, scenePos));
                             modelMenu->close();
                         }
                     });

    if (categoriesModel->rowCount() > 0) {
        const QModelIndex first = categoriesModel->index(0, 0);
        categoryView->setCurrentIndex(first);
        updateNodeList(categoriesModel->data(first, Qt::DisplayRole).toString(), QString());
    }

    auto *action = new QWidgetAction(modelMenu);
    action->setDefaultWidget(container);
    modelMenu->addAction(action);
    modelMenu->setAttribute(Qt::WA_DeleteOnClose);
    txtBox->setFocus();
    return modelMenu;
}

} // namespace NodeCreateSceneMenu
