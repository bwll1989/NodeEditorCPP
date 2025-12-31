#pragma once

#include <QObject>
#include <QReadWriteLock>
#include <QQueue>
#include <QHash>
#include <QVector>
#include <QVariant>
#include <QString>
#include "OSCMessage.h"
#include "StatusItem.h"


#ifdef STATUSCONTAINER_LIBRARY
#define STATUSCONTAINER_EXPORT Q_DECL_EXPORT
#else
#define STATUSCONTAINER_EXPORT Q_DECL_IMPORT
#endif

class STATUSCONTAINER_EXPORT StatusContainer : public QObject {
    Q_OBJECT
public:
    /**
     * 函数级注释：获取全局单例实例（线程安全懒汉式）
     */
    static StatusContainer* instance();

    /**
     * 函数级注释：写入一条状态消息（地址唯一）。存储时不保留 host/port，仅保存 address/type/value。
     * 内部会自动转换为 StatusItem 存储。
     */
    bool updateState(const OSCMessage& message);

    /**
     * 函数级注释：按地址写入状态的便捷方法（不保留 host/port）。
     */
    bool updateState(const QString& address, const QVariant& value, const QString& type = QString());

    /**
     * 函数级注释：判断是否存在指定地址的状态
     */
    bool contains(const QString& address) const;

    /**
     * 函数级注释：获取指定地址的最近消息（若不存在返回默认值）；返回的 host 为空、port 为 0。
     */
    StatusItem last(const QString& address) const;

    /**
     * 函数级注释：按前缀查询最近消息集合
     */
    QVector<StatusItem> queryByPrefix(const QString& prefix) const;

    /**
     * 函数级注释：弹出当前队列中的全部写入事件（host/port 为空/0）
     */
    QVector<StatusItem> drain();

    /**
     * 函数级注释：清空所有已存储状态（工程切换时调用）
     */
    void clearAll();
    /**
     * 函数级注释：注册一个控件到状态容器，关联其ID和地址
     */
    void registerWidget(QWidget* p, const QString& id);
    /**
     * 函数级注释：根据地址获取注册的控件指针
     */
    QWidget* getWidget(const QString& address);
    
public slots:
    public slots:
    /**
     * 处理接收到的UDP数据
     * @param const QVariantMap &data 接收到的UDP数据
     */
    void parseOSC(const OSCMessage &message);  // 处理接收到的UDP数据
    /**
     * 处理接收到的状态数据
     * @param const StatusItem& message 接收到的状态数据
     */
    void parseStatus(const StatusItem& message);

signals:
    /**
     * 函数级注释：新的状态消息写入事件（不包含 host/port）
     */
    void statusUpdated(const StatusItem& message);

private:
    explicit StatusContainer(QObject* parent = nullptr);
    Q_DISABLE_COPY(StatusContainer)

    mutable QReadWriteLock _lock;
    QHash<QString, StatusItem> _latest; // key: address
    QQueue<StatusItem> _queue;
};