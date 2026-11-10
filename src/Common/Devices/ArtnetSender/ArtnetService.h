//
// Created by bwll1 on 2024/9/1.
// 2026/11/8 重构一：队列模式 → 端点状态表 + 50Hz 固定节拍重发（详见 docs/ArtnetTransmitter改动清单.md）
// 2026/11/8 重构二：原 ArtnetTransmitter（发送）/ ArtnetReceiver（每个输入节点各一个）/ ArtPollResponder
//   （设备发现）三套独立的线程+socket 合并为单一 ArtnetService——
//   全进程唯一一个 Art-Net 工作线程 + 唯一一个绑定 :6454 的 UDP 套接字，内部三个子模块：
//     收（OpDmx → 订阅分发） / 答（OpPoll → ArtPollReply） / 发（50Hz 状态表重发）
//   合并动机：多 socket 同绑 6454 时 Windows 对单播包只投递给最后绑定者（潜在丢包），
//   且每个输入节点都在重复解析全网流量、各占一个线程。
//

#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QHash>
#include <QtNetwork/QUdpSocket>
#include <QTimer>
#include <QtCore/qglobal.h>
#include "ArtnetFrame.h"
#include "ArtnetStateTable.h"
#include "Common/AppConfig/ConfigManager.h"

#if defined(ARTNETSERVICE_LIBRARY)
#define ARTNETSERVICE_EXPORT Q_DECL_EXPORT
#else
#define ARTNETSERVICE_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief Art-Net 统一收发服务（单例）
 *
 * 职责划分：
 *  - ArtnetStateTable（成员 mStore）：输出侧唯一的权威状态——端点(host:port) × universe × 写入者桶，
 *    生产者数据变化时随时写入（updateFrame），支持多源 HTP/LTP 合成与属主释放
 *  - 发送心跳（sendTick）：唯一的 50Hz 节拍——每周期从状态表取合成快照，锁外逐包 UDP 发送
 *  - 接收分发（processPendingDatagrams）：解析收到的 OpDmx 并按订阅表分发给各输入节点；
 *    无订阅者或 universe 不匹配时零分配旁路
 *  - 设备发现（收 OpPoll → 回 ArtPollReply）：原 ArtPollResponder 收编，
 *    应答内容实时反映状态表中当前活跃的输出 universe（最多 4 个端口）
 *
 * 数据流：
 *   输出：生产者 --updateFrame--> [状态表] --snapshot--> [50Hz 发送] --> UDP :6454
 *   输入：UDP :6454 --OpDmx--> [解析一次] --订阅分发--> 各 ArtnetInNode
 *   发现：UDP :6454 --OpPoll--> [buildReply] --> 单播回询问方
 */
class ARTNETSERVICE_EXPORT ArtnetService : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例（Meyers 单例，线程安全；首次调用即启动工作线程）
     * @return ArtnetService* 单例实例指针
     */
    static ArtnetService* getInstance();

private:
    /**
     * @brief 私有构造函数：启动工作线程，套接字绑定、定时器都在工作线程内创建
     */
    explicit ArtnetService(QObject* parent = nullptr);

    /**
     * @brief 析构函数：停止工作线程
     */
    ~ArtnetService() override;

    // 禁用拷贝构造和赋值操作
    ArtnetService(const ArtnetService&) = delete;
    ArtnetService& operator=(const ArtnetService&) = delete;

public:
    // ======================== 发送侧（原 ArtnetTransmitter API，保持不变） ========================

    /**
     * @brief 写入/覆盖一个 universe 的状态（推荐的写入接口）
     *
     * 生产者数据变化时调用；只做状态覆盖，不排队、不直接发送。
     * @param frame Art-Net 数据帧（host/universe/dmx 生效）
     * @param owner 写入者身份标签，惯例传生产者节点的 this 指针；nullptr 表示匿名写入者
     */
    void updateFrame(const ArtnetFrame& frame, void* owner)
    {
        mStore.updateFrame(frame, owner);
    }

    /**
     * @brief 释放某属主参与的所有状态桶（节点析构/配置变化时调用）
     *
     * 桶空删槽、端点空删端点；zeroFirst=true 时对清空的槽在下一个发送周期补发一包全零。
     * @param owner     属主身份标签（与 updateFrame 传入的一致）
     * @param zeroFirst 是否对清空的槽补发全零（"删节点归零"策略）
     */
    void releaseOwner(void* owner, bool zeroFirst = false)
    {
        mStore.releaseOwner(owner, zeroFirst);
    }

    /**
     * @brief 查询当前有数据的输出 universe 列表（供 ArtPollReply 上报端口信息）
     * @param maxCount 最多返回的数量
     * @return 升序 universe 列表
     */
    QList<quint16> activeOutputUniverses(int maxCount = 4)
    {
        return mStore.activeOutputUniverses(maxCount);
    }

    // ======================== 接收侧（原 ArtnetReceiver 收编为订阅制） ========================

    /**
     * @brief 订阅 Art-Net 输入数据（替代原"每节点自建 ArtnetReceiver"的模式）
     *
     * 订阅只用于收包侧的"是否有感兴趣者"快速判断（无订阅者时整包跳过、零分配）；
     * 分发仍是广播信号 receiveArtnet，各订阅方自行按 universe 过滤（与原 ArtnetReceiver
     * 的 universeFilter 语义一致：-1 或未设置 = 收全部）。
     * @param context  订阅者上下文对象（惯例传节点 this；销毁时自动退订）
     * @param universe 关心的 universe（-1 = 全部）；仅用于快速旁路判断
     */
    void subscribeInput(QObject* context, int universe = -1);

    /**
     * @brief 退订输入数据（节点析构时也会通过 destroyed 信号自动触发）
     * @param context 订阅时的上下文对象
     */
    void unsubscribeInput(QObject* context);

public slots:
    /**
     * @brief 兼容接口：旧式入队（语义已改为状态覆盖，匿名写入者）
     *
     * 保留原签名以兼容未参与构建的老 src/Clips/ArtnetClip；新代码请使用 updateFrame。
     * @param frame Art-Net 数据帧
     */
    void enqueueFrame(const ArtnetFrame& frame)
    {
        updateFrame(frame, nullptr);
    }

signals:
    /**
     * @brief 数据帧发送成功信号
     * @param frame 已发送的数据帧
     * @param bytesWritten 实际发送的字节数
     */
    void frameSent(const ArtnetFrame& frame, qint64 bytesWritten);

    /**
     * @brief 数据帧发送失败信号
     * @param frame 发送失败的数据帧
     * @param errorString 错误信息
     */
    void frameSendFailed(const ArtnetFrame& frame, const QString& errorString);

    /**
     * @brief 收到 ArtDmx 输入数据（信号形状与原 ArtnetReceiver::receiveArtnet 一致）
     *
     * 全部 OpDmx 都从这里广播，各订阅方自行按 universe 过滤。
     * @param data 包含 universe/sequence/physical/host/hex/default 字段
     */
    void receiveArtnet(QVariantMap& data);

private slots:
    /**
     * @brief 初始化 UDP 套接字（工作线程启动时调用）
     *
     * 绑定 6454 端口（收发共用；Art-Net 规范要求节点在 6454 收发），
     * 同时承担：接收 OpDmx/OpPoll、发送 ArtDmx/ArtPollReply。
     */
    void initializeSocket();

    /**
     * @brief 清理资源（工作线程结束时调用）
     */
    void cleanup();

    /**
     * @brief 50Hz 发送心跳：从状态表取合成快照并逐包发送
     *
     * 每 20ms 触发一次。锁内拷贝在 snapshot() 内部完成，此处只在锁外构包与发送。
     */
    Q_INVOKABLE void sendTick();

    /**
     * @brief 处理收到的 UDP 数据包：OpDmx 走订阅分发，OpPoll 走设备发现应答
     */
    void processPendingDatagrams();

private:
    /**
     * @brief 快速判断是否有订阅者关心该 universe（无则整包旁路，零分配）
     * @param universe 包内的 15 位端口地址
     * @return true = 至少一个订阅者匹配（或存在通配订阅 -1）
     */
    bool hasInterestedSubscriber(quint16 universe) const;

    /**
     * @brief 汇总当前订阅的输入 universe 列表（供 ArtPollReply 的 SwIn 字段上报）
     *
     * 只收集绑定了具体 universe 的订阅者；通配订阅（-1 = 收全部）无法枚举成端口，跳过。
     * @param maxCount 最多返回的数量（协议每包最多 4 个端口）
     * @return 升序 universe 列表
     */
    QList<quint16> subscribedInputUniverses(int maxCount = 4) const;

    /**
     * @brief 构建 ArtPollReply 数据包（239 字节，Art-Net 4 规范）
     * @param pollerIp 询问方 IP（用于选择本机出口网卡，填写应答包内的 IP/MAC 字段）
     * @return QByteArray 构建好的应答包
     */
    QByteArray buildReply(const QHostAddress& pollerIp);

    /**
     * @brief 选择通往指定目标 IP 的本机网卡地址（IPv4，非回环优先同网段）
     * @param targetIp 目标 IP
     * @return 本机 IPv4 地址字符串；找不到时返回空串
     */
    static QString localIpFor(const QHostAddress& targetIp);

    /**
     * @brief 创建 Art-Net 数据包
     * @param frame Art-Net 数据帧（sequence 字段使用槽内自增值）
     * @return QByteArray 构建的 Art-Net 数据包
     */
    QByteArray createArtnetPacket(const ArtnetFrame& frame);

    /**
     * @brief 实际发送数据包的内部函数
     * @param frame Art-Net 数据帧
     * @return bool 发送是否成功
     */
    bool sendFrameInternal(const ArtnetFrame& frame);

private:
    // 工作线程（收/发/应答共用）
    QThread* mThread = nullptr;

    // UDP 套接字（绑定 :6454，收发共用——全进程唯一一个 Art-Net 套接字）
    QUdpSocket* mSocket = nullptr;

    // 端点状态表（输出侧唯一的权威状态，互斥锁自保护）
    ArtnetStateTable mStore;

    // 发送定时器（工作线程）
    QTimer* mQueueTimer = nullptr;

    // 发送间隔（毫秒）= 1000 / 配置的发送频率，默认 20ms（50Hz）；
    // 每 tick 从 ConfigManager 校对，设置页修改后即时生效
    int mSendInterval = static_cast<int>(1000 / AppConstants::ARTNET_OUTPUT_FPS);

    // 输入订阅表：上下文对象 → 关心的 universe（-1 = 全部）
    // 订阅发生在主线程（节点构造），收包在工作线程，用互斥锁保护
    QHash<QObject*, int> mInputSubs;
    mutable QMutex mInputMutex;
};

/**
 * @brief 导出获取 Art-Net 服务单例实例的函数
 * @return ArtnetService* 单例实例指针
 */
ARTNETSERVICE_EXPORT ArtnetService* getArtnetServiceInstance();
