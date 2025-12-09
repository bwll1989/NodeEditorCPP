#include "DockHub.hpp"
#include <QWidget>
#include <QIcon>

/**
 * @brief 获取单例实例
 * @return DockHub 单例引用
 */
DockHub& DockHub::instance() {
    static DockHub hub;
    return hub;
}

/**
 * @brief 注册主窗口创建的 DockManager
 * @param mgr 主窗口持有的 ads::CDockManager 指针
 */
void DockHub::setDockManager(ads::CDockManager* mgr) {
    m_mgr = mgr;
}

/**
 * @brief 获取当前注册的 DockManager
 * @return ads::CDockManager 指针，可能为 nullptr
 */
ads::CDockManager* DockHub::dockManager() const {
    return m_mgr;
}

/**
 * @brief 将外部创建的 DockWidget 挂载到当前 DockManager
 * @param dock 外部创建的 ads::CDockWidget
 * @param area 停靠区域（默认右侧）
 * @return 是否挂载成功
 */
bool DockHub::attachDock(QWidget* dock, ads::DockWidgetArea area) {
    if (!m_mgr || !dock)
    {
        qDebug("DockHub::attachDock: null ptr");
        return false;
        }

    auto wd= m_mgr->findDockWidget("property");
    if(!wd)
    {
        wd= new ads::CDockWidget(m_mgr, dock->windowTitle());
    }
    // 注册到 DockHub，供外部模块统一挂载
    wd->setWidget(dock);
  
    return true;
}

/**
 * @brief 以 QWidget 创建临时 Dock 并挂载（简化用法）
 * @param title Dock 标题
 * @param widget 作为内容的 QWidget（不可为 nullptr）
 * @param area 停靠区域（默认右侧）
 * @param icon Dock 图标（可选）
 * @return 创建并挂载的 Dock 指针；失败返回 nullptr
 */
ads::CDockWidget* DockHub::createAndAttachDock(const QString& title,
                                               QWidget* widget,
                                               ads::DockWidgetArea area,
                                               const QIcon& icon) {
    if (!m_mgr || !widget) return nullptr;
    auto* dock = new ads::CDockWidget(m_mgr, title);
    if (!icon.isNull()) {
        dock->setIcon(icon);
    }
    dock->setWidget(widget);
    m_mgr->addDockWidget(area, dock);
    return dock;
}