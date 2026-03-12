#pragma once

#include <QObject>
#include <QReadWriteLock>
#include <QHash>
#include <QVector>
#include <QPointer>
#include <QVariant>
#include <QByteArray>
#include <QMetaType>
#include <QMetaObject>
#include <algorithm>

/**
 * 函数级注释：表示全局事件类型
 * - Command：表示执行类命令（远程操作）
 * - StateFeedback：表示状态反馈（状态镜像）
 */
enum class GlobalEventKind {
    Command,
    Feedback
};

/**
 * 函数级注释：表示一条全局事件总线中的消息
 * - 使用字符串地址进行路由，例如：/dataflow/player/1/play
 * - 使用 QVariant 作为载荷，可以封装任意基础或 Qt 支持的类型
 */
#ifdef STATUSCONTAINER_LIBRARY
#define STATUSCONTAINER_EXPORT Q_DECL_EXPORT
#else
#define STATUSCONTAINER_EXPORT Q_DECL_IMPORT
#endif

struct STATUSCONTAINER_EXPORT GlobalEvent {
    QString address;
    QVariant payload;
    GlobalEventKind kind;

    /**
     * 函数级注释：默认构造函数，生成一个空事件
     */
    GlobalEvent() : kind(GlobalEventKind::Command) {}

    /**
     * 函数级注释：带参构造函数，使用地址、载荷与事件类型初始化
     */
    GlobalEvent(const QString& addr, const QVariant& data, GlobalEventKind k = GlobalEventKind::Command)
        : address(addr), payload(data), kind(k) {}
};

Q_DECLARE_METATYPE(GlobalEvent)

/**
 * 函数级注释：全局事件总线类，负责在模块之间分发基于地址的消息
 * - 支持按地址订阅与取消订阅
 * - 支持多订阅者，所有匹配订阅者都会收到对应事件
 * - 使用 Qt::QueuedConnection 分发事件，保证跨线程安全
 */
class STATUSCONTAINER_EXPORT GlobalEventBus : public QObject {
    Q_OBJECT
public:
    /**
     * 函数级注释：获取全局单例实例（线程安全懒汉式）
     */
    static GlobalEventBus* instance()
    {
        static GlobalEventBus inst;
        return &inst;
    }

    /**
     * 函数级注释：发布一条事件到总线（默认视为命令事件）
     * @param address  事件地址（如 /dataflow/alias/nodeId/path）
     * @param payload  事件载荷（可选，默认空）
     */
    void publish(const QString& address, const QVariant& payload = QVariant(), GlobalEventKind kind = GlobalEventKind::Command)
    {
        if (address.isEmpty()) {
            return;
        }
        GlobalEvent event(address, payload, kind);
        dispatch(event);
        emit eventPublished(event);
    }

    /**
     * 函数级注释：发布命令事件（远程操作）
     */
    void publishCommand(const QString& address, const QVariant& payload = QVariant())
    {
        publish(address, payload, GlobalEventKind::Command);
    }

    /**
     * 函数级注释：发布状态反馈事件（状态镜像）
     */
    void publishState(const QString& address, const QVariant& payload = QVariant())
    {
        publish(address, payload, GlobalEventKind::Feedback);
    }

    /**
     * 函数级注释：订阅指定地址上的事件
     * @param address  要订阅的事件地址
     * @param receiver 槽函数所在对象指针（QObject 派生类）
     * @param method   槽函数签名，形式如 SLOT(onGlobalEvent(GlobalEvent))
     * - 同一对象对同一地址、同一槽函数的重复订阅将被忽略
     */
    void subscribe(const QString& address, QObject* receiver, const char* method)
    {
        if (address.isEmpty() || !receiver || !method) {
            return;
        }

        // 处理由 SLOT()/SIGNAL() 宏传入的 method 字符串：
        // 1. 去掉前导数字（如 "\1onGlobalEvent(GlobalEvent)" -> "onGlobalEvent(GlobalEvent)"）
        // 2. 去掉参数列表，只保留函数名（"onGlobalEvent(GlobalEvent)" -> "onGlobalEvent"）
        QByteArray sig(method);
        if (!sig.isEmpty() && sig[0] >= '0' && sig[0] <= '2') {
            sig = sig.mid(1);
        }
        int parenIndex = sig.indexOf('(');
        if (parenIndex > 0) {
            sig = sig.left(parenIndex);
        }
        sig = QMetaObject::normalizedSignature(sig.constData());

        QWriteLocker locker(&_lock);
        QVector<HandlerEndpoint>& list = _handlers[address];
        for (const HandlerEndpoint& ep : list) {
            if (ep.target == receiver && ep.method == sig) {
                return;
            }
        }
        HandlerEndpoint ep;
        ep.target = receiver;
        ep.method = sig;
        list.push_back(ep);
    }

    /**
     * 函数级注释：取消某对象在指定地址上的所有槽函数订阅
     * @param address  事件地址
     * @param receiver 槽函数所在对象指针
     */
    void unsubscribe(const QString& address, QObject* receiver)
    {
        if (address.isEmpty() || !receiver) {
            return;
        }
        QWriteLocker locker(&_lock);
        auto it = _handlers.find(address);
        if (it == _handlers.end()) {
            return;
        }
        QVector<HandlerEndpoint>& list = it.value();
        list.erase(std::remove_if(list.begin(), list.end(),
                                  [receiver](const HandlerEndpoint& ep) {
                                      return ep.target == receiver;
                                  }),
                   list.end());
        if (list.isEmpty()) {
            _handlers.erase(it);
        }
    }

    /**
     * 函数级注释：取消某对象在所有地址上的订阅
     * @param receiver 槽函数所在对象指针
     */
    void unsubscribe(QObject* receiver)
    {
        if (!receiver) {
            return;
        }
        QWriteLocker locker(&_lock);
        for (auto it = _handlers.begin(); it != _handlers.end();) {
            QVector<HandlerEndpoint>& list = it.value();
            list.erase(std::remove_if(list.begin(), list.end(),
                                      [receiver](const HandlerEndpoint& ep) {
                                          return ep.target == receiver;
                                      }),
                       list.end());
            if (list.isEmpty()) {
                it = _handlers.erase(it);
            } else {
                ++it;
            }
        }
    }

signals:
    /**
     * 函数级注释：有新事件发布到总线时发出的信号
     * - 可用于日志、监控或调试
     */
    void eventPublished(const GlobalEvent& event);

private:
    /**
     * 函数级注释：构造函数，注册元类型并初始化内部状态
     */
    explicit GlobalEventBus(QObject* parent = nullptr)
        : QObject(parent)
    {
        qRegisterMetaType<GlobalEvent>("GlobalEvent");
    }

    Q_DISABLE_COPY(GlobalEventBus)

    /**
     * 函数级注释：将事件分发给所有已订阅该地址的接收者
     * - 使用只读锁复制订阅列表，在锁外进行实际调用
     * - 使用 Qt::QueuedConnection 保证跨线程调用安全
     */
    void dispatch(const GlobalEvent& event)
    {
        QVector<HandlerEndpoint> endpoints;
        {
            QReadLocker locker(&_lock);
            auto it = _handlers.find(event.address);
            if (it != _handlers.end()) {
                endpoints = it.value();
            }
        }
        for (const HandlerEndpoint& ep : endpoints) {
            if (!ep.target) {
                continue;
            }
            QMetaObject::invokeMethod(ep.target,
                                      ep.method.constData(),
                                      Qt::QueuedConnection,
                                      Q_ARG(GlobalEvent, event));
        }
    }

    /**
     * 函数级注释：内部订阅端点结构，记录目标对象与槽函数信息
     */
    struct HandlerEndpoint {
        QPointer<QObject> target;
        QByteArray method;
    };

    mutable QReadWriteLock _lock;
    QHash<QString, QVector<HandlerEndpoint>> _handlers;
};