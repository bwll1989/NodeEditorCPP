#pragma once
#include <QObject>
#include <QPointer>
#include "DockManager.h"
#include "DockWidget.h"
#ifdef DOCKHUB_LIBRARY
#define DOCKHUB_EXPORT Q_DECL_EXPORT
#else
#define DOCKHUB_EXPORT Q_DECL_IMPORT
#endif
/**
 * @brief DockHub: Dock 管理中枢单例
 * @details 由主窗口注册 CDockManager，其他模块通过本类统一挂载 Dock。
 */
class DOCKHUB_EXPORT DockHub : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(DockHub)

public:
    /**
     * @brief 获取单例实例
     * @return DockHub 单例引用
     */
    static DockHub& instance();

    /**
     * @brief 注册主窗口创建的 DockManager
     * @param mgr 主窗口持有的 ads::CDockManager 指针（父对象应为主窗口）
     */
    void setDockManager(ads::CDockManager* mgr);

    /**
     * @brief 获取当前注册的 DockManager
     * @return 当前的 ads::CDockManager 指针，可能为 nullptr
     */
    ads::CDockManager* dockManager() const;

    /**
     * @brief 将外部创建的 DockWidget 挂载到当前 DockManager
     * @param dock 外部创建的 ads::CDockWidget
     * @param area 停靠区域（默认右侧）
     * @return 是否挂载成功
     */
    bool attachDock(QWidget* dock,
                    ads::DockWidgetArea area = ads::RightDockWidgetArea);

    /**
     * @brief 以 QWidget 创建临时 Dock 并挂载（简化用法）
     * @param title Dock 标题
     * @param widget 作为内容的 QWidget（不可为 nullptr）
     * @param area 停靠区域（默认右侧）
     * @param icon Dock 图标（可选）
     * @return 创建并挂载的 Dock 指针；失败返回 nullptr
     */
    ads::CDockWidget* createAndAttachDock(const QString& title,
                                          QWidget* widget,
                                          ads::DockWidgetArea area = ads::RightDockWidgetArea,
                                          const QIcon& icon = QIcon());

private:
    DockHub() = default;

private:
    QPointer<ads::CDockManager> m_mgr;
};