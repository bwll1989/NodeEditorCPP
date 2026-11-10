//
// Created by bwll1 on 2024/9/1.
// 2026/11/8 重构一：队列模式 → 端点状态表 + 50Hz 固定节拍重发（详见 docs/ArtnetTransmitter改动清单.md）
// 2026/11/8 重构二：输入/输出/设备发现合并为单一 ArtnetService（原 ArtnetReceiver /
//   ArtPollResponder 独立类收编，详见类头注释）
//

#include "ArtnetService.h"
#include <QHostAddress>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QtEndian>
#include <QMutexLocker>
#include <QDebug>
#include <QDateTime>
#include <algorithm>
#include <cstring>

namespace {
    /// Art-Net 节点短名称（ArtPollReply 内最多 18 字节）
    constexpr const char* kShortName = "Flow Art-Net";
    /// Art-Net 节点长名称（最多 64 字节）
    constexpr const char* kLongName = "Flow Art-Net Node (Input/Output)";
    /// 节点报告串（最多 64 字节）
    constexpr const char* kNodeReport = "#0001 [0000] Flow Art-Net Node Ready";
    /// ArtPollReply 上报的最大端口数（协议规定每包最多 4 个端口）
    constexpr int kMaxReportPorts = 4;
}

/**
 * @brief 导出获取 Art-Net 服务单例实例的函数
 */
ARTNETSERVICE_EXPORT ArtnetService* getArtnetServiceInstance()
{
    return ArtnetService::getInstance();
}

/**
 * @brief 获取单例实例（Meyers 单例：C++11 保证静态局部变量初始化的线程安全性）
 */
ArtnetService* ArtnetService::getInstance()
{
    static ArtnetService instance;
    return &instance;
}

/**
 * @brief 私有构造函数：创建工作线程，套接字与定时器都在工作线程中运行
 */
ArtnetService::ArtnetService(QObject* parent)
    : QObject(parent), mQueueTimer(new QTimer(this))
{
    // 注册信号传递数值类型（frameSent/frameSendFailed 跨线程携带 ArtnetFrame，
    // receiveArtnet 跨线程携带 QVariantMap）
    qRegisterMetaType<ArtnetFrame>("ArtnetFrame");
    qRegisterMetaType<ArtnetFrame>("ArtnetFrame&");
    qRegisterMetaType<QVariantMap>("QVariantMap&");

    // 创建工作线程，本对象与定时器都移入工作线程
    mThread = new QThread(this);
    this->moveToThread(mThread);
    // 初始发送间隔从配置读取（设置页可改，运行期每 tick 校对即时生效）
    mSendInterval = 1000 / qBound(1, ConfigManager::instance().getArtnetSendFps(), 100);
    mQueueTimer->setInterval(mSendInterval);
    mQueueTimer->moveToThread(mThread);

    connect(mThread, &QThread::started, this, &ArtnetService::initializeSocket);
    connect(mThread, &QThread::finished, this, &ArtnetService::cleanup);
    connect(mQueueTimer, &QTimer::timeout, this, &ArtnetService::sendTick);

    // 启动工作线程（触发 initializeSocket），再异步启动定时器
    mThread->start();
    QMetaObject::invokeMethod(mQueueTimer, "start", Qt::QueuedConnection);
}

/**
 * @brief 析构函数：停止工作线程
 */
ArtnetService::~ArtnetService()
{
    if (mThread) {
        mThread->quit();
        mThread->wait();
    }
}

/**
 * @brief 订阅 Art-Net 输入数据
 *
 * 上下文销毁时通过 destroyed 信号自动退订，订阅方无需手动清理。
 */
void ArtnetService::subscribeInput(QObject* context, int universe)
{
    if (!context) {
        return;
    }
    {
        QMutexLocker locker(&mInputMutex);
        mInputSubs.insert(context, universe);
    }
    // 上下文销毁时自动退订（先断开再连接，保证同一上下文重复订阅时不堆积连接）
    disconnect(context, &QObject::destroyed, this, &ArtnetService::unsubscribeInput);
    connect(context, &QObject::destroyed, this, &ArtnetService::unsubscribeInput,
            Qt::DirectConnection);
}

/**
 * @brief 退订输入数据（幂等）
 */
void ArtnetService::unsubscribeInput(QObject* context)
{
    if (!context) {
        return;
    }
    QMutexLocker locker(&mInputMutex);
    mInputSubs.remove(context);
}

/**
 * @brief 汇总当前订阅的输入 universe 列表（供 ArtPollReply 的 SwIn 字段上报）
 *
 * 只收集绑定了具体 universe 的订阅者；通配订阅（-1 = 收全部）无法枚举成具体端口，跳过。
 */
QList<quint16> ArtnetService::subscribedInputUniverses(int maxCount) const
{
    QMutexLocker locker(&mInputMutex);
    QSet<quint16> set;
    for (auto it = mInputSubs.constBegin(); it != mInputSubs.constEnd(); ++it) {
        if (it.value() >= 0) {
            set.insert(static_cast<quint16>(it.value()));
        }
    }
    QList<quint16> list = set.values();
    std::sort(list.begin(), list.end());
    if (list.size() > maxCount) {
        list = list.mid(0, maxCount);
    }
    return list;
}

/**
 * @brief 快速判断是否有订阅者关心该 universe
 *
 * 无任何输入节点时整包旁路，不构建 QVariantMap（零分配）。
 */
bool ArtnetService::hasInterestedSubscriber(quint16 universe) const
{
    QMutexLocker locker(&mInputMutex);
    for (auto it = mInputSubs.constBegin(); it != mInputSubs.constEnd(); ++it) {
        const int uni = it.value();
        if (uni == -1 || uni == static_cast<int>(universe)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 初始化 UDP 套接字（工作线程内调用）
 *
 * 收发共用一个套接字，绑定 6454：
 *  - Art-Net 规范要求节点在 6454 端口收发，ArtPollReply 的源端口也应为 6454；
 *  - 全进程仅此一个绑定，消除了原先"每节点 + 每应答器各自绑定 6454"时
 *    Windows 对单播包只投递给最后绑定者的歧义。
 */
void ArtnetService::initializeSocket() {
    mSocket = new QUdpSocket(this);
    if (mSocket->bind(QHostAddress::AnyIPv4, AppConstants::ARTNET_PORT,
                      QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        connect(mSocket, &QUdpSocket::readyRead, this, &ArtnetService::processPendingDatagrams);
        qDebug() << "ArtnetService: 套接字已绑定 6454（收发/设备发现共用）";
    } else {
        // 绑定失败仍允许发送（降级），但收包与设备发现不可用
        qWarning() << "ArtnetService: 6454 端口绑定失败，接收与设备发现不可用："
                   << mSocket->errorString();
    }
}

/**
 * @brief 清理资源（工作线程结束时调用）
 */
void ArtnetService::cleanup()
{
    if (mQueueTimer) {
        mQueueTimer->stop();
        mQueueTimer->deleteLater();
        mQueueTimer = nullptr;
    }

    if (mSocket) {
        mSocket->close();
        mSocket->deleteLater();
        mSocket = nullptr;
    }

    qDebug() << "ArtnetService cleaned up";
}

/**
 * @brief 处理收到的 UDP 数据包（工作线程内调用）
 *
 * 只认两种 opcode：
 *  - OpDmx (0x5000)：输入数据，解析后经 receiveArtnet 信号分发给订阅者；
 *    无订阅者或 universe 不匹配时零分配旁路
 *  - OpPoll (0x2000)：设备发现询问，构建 ArtPollReply 单播回询问方
 *    （原 ArtPollResponder 收编；其余 opcode 静默忽略）
 */
void ArtnetService::processPendingDatagrams()
{
    while (mSocket && mSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(mSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort = 0;
        mSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // 基本合法性：长度 + 标识符
        if (datagram.size() < 14) {
            continue;
        }
        const char* header = datagram.constData();
        if (strncmp(header, "Art-Net", 7) != 0) {
            continue;
        }

        // OpCode 小端序（Art-Net 规范：低字节在前）
        const quint16 opCode = qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar*>(header + 8));

        if (opCode == 0x5000) { // ---- OpDmx：输入数据 ----
            if (datagram.size() < 18) {
                continue;
            }
            // Port-Address 按规范"低字节在前"传输（byte14=SubUni，byte15=Net），
            // LE 读出即 15 位端口地址
            const quint16 universe = qFromLittleEndian<quint16>(
                reinterpret_cast<const uchar*>(header + 14));

            // 快速旁路：没有订阅者关心该 universe 就不构建数据（零分配）
            if (!hasInterestedSubscriber(universe)) {
                continue;
            }

            // 解析字段并填充到 512 字节（不足补零），形状与原 ArtnetReceiver 一致
            QVariantMap result;
            result["universe"] = universe;
            result["sequence"] = static_cast<quint8>(header[12]);
            result["physical"] = static_cast<quint8>(header[13]);
            result["host"] = sender.toString();
            const QByteArray dmxData = datagram.mid(18).leftJustified(512, 0x00);
            result["hex"] = dmxData.toHex();
            result["default"] = dmxData;

            // 广播给所有订阅者，各订阅方自行按 universe 过滤
            emit receiveArtnet(result);
        } else if (opCode == 0x2000) { // ---- OpPoll：设备发现询问 ----
            // 构建 ArtPollReply 并单播回询问方（使用其实际来源端口）
            const QByteArray reply = buildReply(sender);
            mSocket->writeDatagram(reply, sender, senderPort);
        }
        // 其他 opcode 静默忽略
    }
}

/**
 * @brief 50Hz 发送心跳：从状态表取合成快照，锁外逐包发送
 *
 * 每周期做三件事：
 *  1. 读取当前合并策略（全局设置，即时生效）
 *  2. snapshot() 在锁内完成拷贝 + HTP/LTP 合成 + 序号推进
 *  3. 锁外把快照逐包构包、UDP 发送
 */
void ArtnetService::sendTick()
{
    // 发送频率即时生效：每 tick 校对配置值，变化时重设定时器间隔（开销可忽略）
    const int fps = qBound(1, ConfigManager::instance().getArtnetSendFps(), 100);
    const int intervalMs = 1000 / fps;
    if (intervalMs != mSendInterval) {
        mSendInterval = intervalMs;
        if (mQueueTimer) {
            mQueueTimer->setInterval(intervalMs);
        }
    }

    // 读取全局合并策略（纯 int 读取，开销可忽略）
    const auto mergeMode = static_cast<ArtnetMergeMode>(ConfigManager::instance().getArtnetMergeMode());

    // 取快照：返回后即为锁外数据，发送慢也不会阻塞生产者写入
    const QList<ArtnetStateTable::SnapshotEntry> snapshot = mStore.snapshot(mergeMode);
    if (snapshot.isEmpty()) {
        return;
    }

    // 逐包发送：把快照条目还原为 ArtnetFrame，复用既有构包/发送链路
    for (const ArtnetStateTable::SnapshotEntry& entry : snapshot) {
        ArtnetFrame frame(entry.endpoint.host, entry.universe, entry.dmx, entry.sequence);
        frame.port = entry.endpoint.port;
        frame.timestamp = QDateTime::currentMSecsSinceEpoch();

        // 发送失败仅记录并丢弃（UDP 场景下个周期会继续重发最新状态）
        sendFrameInternal(frame);
    }
}

/**
 * @brief 实际发送数据包的内部函数
 */
bool ArtnetService::sendFrameInternal(const ArtnetFrame& frame)
{
    if (!mSocket) {
        emit frameSendFailed(frame, "Socket not initialized");
        return false;
    }

    // 创建 Art-Net 数据包
    QByteArray packet = createArtnetPacket(frame);

    // 发送数据包
    QHostAddress targetAddress(frame.host);
    qint64 bytesWritten = mSocket->writeDatagram(packet, targetAddress, frame.port);

    if (bytesWritten == -1) {
        QString errorString = mSocket->errorString();
        qWarning() << "Failed to send Art-Net packet:" << errorString;
        emit frameSendFailed(frame, errorString);
        return false;
    }

    emit frameSent(frame, bytesWritten);
    return true;
}

/**
 * @brief 创建 Art-Net 数据包（ArtDmx，协议版本 14）
 *
 * 包结构（Art-Net 4 规范）：
 *   [0-7]   "Art-Net\0" 标识符
 *   [8-9]   OpCode = OpDmx(0x5000)，小端序
 *   [10-11] 协议版本 14，大端序
 *   [12]    Sequence 序列号（发送端每包自增，255 回绕到 1，0 = 接收端不校验）
 *   [13]    Physical 物理端口，固定 0
 *   [14-15] Port-Address：规范规定"低字节在前"——byte14 = SubUni（低 8 位），
 *           byte15 = Net（高 7 位），按小端写入 16 位端口地址
 *   [16-17] 数据长度 512，大端序
 *   [18-529] DMX 数据
 */
QByteArray ArtnetService::createArtnetPacket(const ArtnetFrame& frame)
{
    QByteArray packet;
    packet.resize(18 + 512); // Art-Net 头部 18 字节 + DMX 数据 512 字节

    // Art-Net 标识符 "Art-Net\0"
    packet[0] = 'A'; packet[1] = 'r'; packet[2] = 't'; packet[3] = '-';
    packet[4] = 'N'; packet[5] = 'e'; packet[6] = 't'; packet[7] = 0x00;

    // 操作码 OpDmx (0x5000) - 小端序
    qToLittleEndian<quint16>(0x5000, reinterpret_cast<uchar*>(packet.data() + 8));

    // 协议版本 (0x000E) - 大端序
    qToBigEndian<quint16>(0x000E, reinterpret_cast<uchar*>(packet.data() + 10));

    // 序列号（由状态表按槽自增）
    packet[12] = frame.sequence;

    // 物理端口
    packet[13] = 0x00;

    // 宇宙编号 - Port-Address 按规范"低字节在前"（byte14=SubUni 低 8 位，byte15=Net 高 7 位）；
    // 与接收侧 processPendingDatagrams 的 LE 读取互为镜像
    qToLittleEndian<quint16>(frame.universe, reinterpret_cast<uchar*>(packet.data() + 14));

    // 数据长度 (512) - 大端序
    qToBigEndian<quint16>(512, reinterpret_cast<uchar*>(packet.data() + 16));

    // DMX 数据
    QByteArray dmxData = frame.getFormattedDmxData();
    packet.replace(18, 512, dmxData);

    return packet;
}

/**
 * @brief 选择通往指定目标 IP 的本机网卡地址
 *
 * 优先选择与目标同网段的非回环 IPv4；否则取第一个非回环 IPv4。
 */
QString ArtnetService::localIpFor(const QHostAddress& targetIp)
{
    const QHostAddress target(targetIp.toIPv4Address());
    QString fallback;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            !(iface.flags() & QNetworkInterface::IsRunning) ||
            (iface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            const QHostAddress addr = entry.ip();
            if (addr.protocol() != QAbstractSocket::IPv4Protocol) {
                continue;
            }
            const QString s = addr.toString();
            // 同网段判断：IPv4 地址与掩码按位与后比较（Qt 6 的 isInSubnet
            // 需要 QHostAddress+前缀的 pair 形式，这里手动做掩码匹配更直观）
            const QHostAddress netmask = entry.netmask();
            if (netmask.protocol() == QAbstractSocket::IPv4Protocol) {
                const quint32 mask = netmask.toIPv4Address();
                if ((addr.toIPv4Address() & mask) == (target.toIPv4Address() & mask)) {
                    return s;
                }
            }
            if (fallback.isEmpty()) {
                fallback = s;
            }
        }
    }
    return fallback;
}

/**
 * @brief 构建 ArtPollReply 数据包（239 字节，Art-Net 4 规范）
 *
 * 关键字段布局：
 *   [0-7]   "Art-Net\0"
 *   [8-9]   OpCode = OpPollReply(0x2100)，小端序
 *   [10-11] 协议版本 14，大端序
 *   [12-15] 本机 IP（4 字节）
 *   [16-17] 端口 6454，小端序
 *   [20]    NetSwitch（第一个活跃 universe 的 Net 段）
 *   [21]    SubSwitch（Sub-Net 段）
 *   [28-45] ShortName（18 字节）
 *   [46-109] LongName（64 字节）
 *   [110-173] NodeReport（64 字节）
 *   [174-175] 端口数量（小端序）
 *   [176-179] PortTypes（bit7 = 可输出，bit6 = 可输入，低 6 位协议 = 0）
 *   [180-183] GoodInput（活跃输入口 0x00，无输入 0x08）
 *   [184-187] GoodOutput（正在输出 = 0x80）
 *   [188-191] SwIn / [192-195] SwOut（各端口 universe 低字节）
 *   [202]   Style = 0x01（Node）
 *   [203-208] 本机 MAC
 *   [209-212] BindIP
 */
QByteArray ArtnetService::buildReply(const QHostAddress& pollerIp)
{
    QByteArray packet(239, 0x00); // ArtPollReply 固定 239 字节，余量填零

    // 标识符 "Art-Net\0"
    packet[0] = 'A'; packet[1] = 'r'; packet[2] = 't'; packet[3] = '-';
    packet[4] = 'N'; packet[5] = 'e'; packet[6] = 't'; packet[7] = 0x00;

    // OpCode = OpPollReply(0x2100) - 小端序
    qToLittleEndian<quint16>(0x2100, reinterpret_cast<uchar*>(packet.data() + 8));

    // 协议版本 14 - 大端序
    qToBigEndian<quint16>(0x000E, reinterpret_cast<uchar*>(packet.data() + 10));

    // 本机出口网卡信息（IP/MAC，选与询问方同网段的接口）
    const QString localIp = localIpFor(pollerIp);
    quint32 ipV4 = 0;
    QByteArray macBytes(6, 0x00);
    for (const QNetworkInterface& iface : QNetworkInterface::allInterfaces()) {
        if (iface.hardwareAddress().isEmpty()) {
            continue;
        }
        for (const QNetworkAddressEntry& entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                entry.ip().toString() == localIp) {
                ipV4 = entry.ip().toIPv4Address();
                // 硬件地址形如 "AA:BB:CC:DD:EE:FF"，解析为 6 字节
                const QStringList parts = iface.hardwareAddress().split(':');
                for (int i = 0; i < 6 && i < parts.size(); ++i) {
                    macBytes[i] = static_cast<char>(parts[i].toInt(nullptr, 16) & 0xFF);
                }
                break;
            }
        }
        if (ipV4 != 0) break;
    }
    // [12-15] 本机 IP
    qToBigEndian<quint32>(ipV4, reinterpret_cast<uchar*>(packet.data() + 12));
    // [16-17] Art-Net 端口 6454 - 小端序
    qToLittleEndian<quint16>(AppConstants::ARTNET_PORT, reinterpret_cast<uchar*>(packet.data() + 16));

    // 汇总端口表：输出口（状态表中有数据的 universe）∪ 输入口（订阅表中的 universe），
    // 同一 universe 可能既收又发，去重后按升序取前 4 个（协议每包最多 4 个端口）
    const QList<quint16> outUniverses = mStore.activeOutputUniverses(kMaxReportPorts);
    const QList<quint16> inUniverses = subscribedInputUniverses(kMaxReportPorts);
    QSet<quint16> portSet;
    for (quint16 u : outUniverses) {
        portSet.insert(u);
    }
    for (quint16 u : inUniverses) {
        portSet.insert(u);
    }
    QList<quint16> ports = portSet.values();
    std::sort(ports.begin(), ports.end());
    if (ports.size() > kMaxReportPorts) {
        ports = ports.mid(0, kMaxReportPorts);
    }
    const int numPorts = ports.size();

    // [20] NetSwitch / [21] SubSwitch：取第一个端口 universe 的 Net 段与 Sub-Net 段
    if (!ports.isEmpty()) {
        packet[20] = static_cast<char>((ports.first() >> 8) & 0x7F); // Net（7 位）
        packet[21] = static_cast<char>((ports.first() >> 4) & 0x0F); // Sub-Net（4 位）
    }

    // [18-19] 版本信息
    packet[18] = 1; // VersInfoH
    packet[19] = 7; // VersInfoL（对齐产品版本 1.7）

    // [28-45] ShortName（最多 18 字节，含结尾 \0）
    QByteArray shortName(kShortName);
    shortName = shortName.left(17);
    packet.replace(28, shortName.size(), shortName);

    // [46-109] LongName（最多 64 字节）
    QByteArray longName(kLongName);
    longName = longName.left(63);
    packet.replace(46, longName.size(), longName);

    // [110-173] NodeReport（最多 64 字节）
    QByteArray report(kNodeReport);
    report = report.left(63);
    packet.replace(110, report.size(), report);

    // [174-175] 端口数量 - 小端序
    qToLittleEndian<quint16>(numPorts, reinterpret_cast<uchar*>(packet.data() + 174));

    // [176-179] PortTypes（Art-Net 4 规范）：bit7 = 可从 Art-Net 输出，bit6 = 可向 Art-Net 输入，
    // 低 6 位 = 协议类型（0 = DMX512）；同一 universe 既收又发的端口 = 0xC0
    // [180-183] GoodInput：bit7 = 已收到数据，bit3 = 输入禁用；
    //           活跃输入口 = 0x00（健康），非输入口 = 0x08（输入禁用）
    // [184-187] GoodOutput：bit7 = 正在输出数据
    // [188-191] SwIn / [192-195] SwOut：端口地址低 8 位（该端口不具备的方向填 0）
    for (int i = 0; i < numPorts && i < 4; ++i) {
        const quint16 u = ports[i];
        const bool isOut = outUniverses.contains(u);
        const bool isIn = inUniverses.contains(u);
        packet[176 + i] = static_cast<char>((isOut ? 0x80 : 0x00) | (isIn ? 0x40 : 0x00));
        packet[180 + i] = static_cast<char>(isIn ? 0x00 : 0x08);
        packet[184 + i] = static_cast<char>(isOut ? 0x80 : 0x00);
        if (isIn) {
            packet[188 + i] = static_cast<char>(u & 0xFF);
        }
        if (isOut) {
            packet[192 + i] = static_cast<char>(u & 0xFF);
        }
    }

    // [202] Style = 0x01（Node：Art-Net → DMX 网关形态）
    packet[202] = static_cast<char>(0x01);

    // [203-208] 本机 MAC
    packet.replace(203, 6, macBytes);

    // [209-212] BindIP（本机 IP）
    qToBigEndian<quint32>(ipV4, reinterpret_cast<uchar*>(packet.data() + 209));
    // [213] BindIndex：单节点绑定为 0

    return packet;
}
