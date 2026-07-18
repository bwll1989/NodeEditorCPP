#pragma once

#include <QPointF>
#include <memory>

class QMenu;

namespace QtNodes {
class BasicGraphicsScene;
class NodeDelegateModelRegistry;
}

/**
 * 场景空白处双击 / 右键用的「创建节点」菜单（分类 / 节点 / Doc 摘要）。
 */
namespace NodeCreateSceneMenu {

QMenu *create(QtNodes::BasicGraphicsScene *scene,
              std::shared_ptr<QtNodes::NodeDelegateModelRegistry> registry,
              QPointF const &scenePos);

} // namespace NodeCreateSceneMenu
