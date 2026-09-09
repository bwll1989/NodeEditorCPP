#include "RangerMiniController.h"

#include <QtMath>
#include <algorithm>

namespace {
// —— 主机 → 底盘（控制）——
constexpr quint32 kCmdMotion = 0x111;      // 运动控制：线速度 / 自旋 / 转角，需周期发送
constexpr quint32 kCmdMotionMode = 0x141;  // 运动模型切换
constexpr quint32 kCmdControlMode = 0x421; // 控制模式：待机 / CAN 指令
constexpr quint32 kCmdClearFault = 0x441;  // 清故障

// —— 底盘 → 主机（反馈）——
constexpr quint32 kFbSystem = 0x211;     // 系统状态：车体状态、控制模式、电压、故障
constexpr quint32 kFbMotion = 0x221;     // 运动反馈：实际线速度、自旋、转角
constexpr quint32 kFbMotionMode = 0x291; // 当前运动模型 / 切换中 / 驱动模式

constexpr int kHeartbeatMs = 20;   // 协议建议周期 20 ms（超时约 500 ms）
constexpr int kBitrate = 500000;   // Ranger Mini 固定波特率
} // namespace

RangerMiniController::RangerMiniController(QObject *parent)
    : QObject(parent)
{
    m_can = new CanBus(this);
    m_heartbeat = new QTimer(this);
    m_heartbeat->setInterval(kHeartbeatMs);

    // CanBus 内部已在工作线程，信号用 Queued 默认即可跨线程投递
    connect(m_can, &CanBus::isReady, this, &RangerMiniController::onCanReady);
    connect(m_can, &CanBus::recMsg, this, &RangerMiniController::onCanFrame);
    connect(m_can, &CanBus::errorOccurred, this, &RangerMiniController::errorOccurred);
    connect(m_heartbeat, &QTimer::timeout, this, &RangerMiniController::onHeartbeat);
}

RangerMiniController::~RangerMiniController()
{
    // 析构前先停心跳并关设备，避免 Worker 线程仍在访问设备
    setControlEnabled(false);
    m_can->closeDevice();
}

void RangerMiniController::openDevice(int deviceIndex)
{
    if (deviceIndex < 0) {
        closeDevice();
        return;
    }
    // channel=0，bitrate=500k，modeFlags=0（Normal，禁止 Loopback）
    m_can->openDevice(deviceIndex, 0, kBitrate, 0);
}

void RangerMiniController::closeDevice()
{
    setControlEnabled(false);
    m_can->closeDevice();
}

void RangerMiniController::enterCanControl()
{
    if (!m_busReady) {
        emit errorOccurred(QStringLiteral("CAN bus not connected"));
        return;
    }
    // 手册要求：待机下只响应模式切换；先进入 CAN 指令模式再发运动相关指令
    sendControlMode(0x01);
    sendClearFault(0x00);
    sendMotionMode(static_cast<quint8>(m_motionMode));
}

void RangerMiniController::clearFaults()
{
    if (!m_busReady) {
        return;
    }
    // 0x00 = 清除全部非严重故障（含急停释放后清除）
    sendClearFault(0x00);
}

void RangerMiniController::setMotionMode(int mode)
{
    m_motionMode = std::clamp(mode, 0, 3);
    if (m_busReady) {
        sendMotionMode(static_cast<quint8>(m_motionMode));
    }
}

void RangerMiniController::setControlEnabled(bool enabled)
{
    if (m_controlEnabled == enabled) {
        return;
    }

    if (enabled) {
        if (!m_busReady) {
            emit errorOccurred(QStringLiteral("Connect CAN device first"));
            emit controlEnabledChanged(false);
            return;
        }
        // 使能瞬间完成：切模式 → 清故障 → 设运动模型 → 启动心跳
        enterCanControl();
        m_controlEnabled = true;
        m_heartbeat->start();
        sendMotionFrame(); // 立即发一帧，避免等待第一个定时周期
    } else {
        m_controlEnabled = false;
        m_heartbeat->stop();
        // 连续发若干零速帧，让底盘尽快停稳（而不是干等 500 ms 超时）
        if (m_busReady) {
            const double oldLin = m_linearMps;
            const double oldSteer = m_steerRad;
            const double oldSpin = m_spinRadPerSec;
            m_linearMps = 0.0;
            m_steerRad = 0.0;
            m_spinRadPerSec = 0.0;
            for (int i = 0; i < 3; ++i) {
                sendMotionFrame();
            }
            // 恢复用户设定值，便于再次使能时沿用上次参数
            m_linearMps = oldLin;
            m_steerRad = oldSteer;
            m_spinRadPerSec = oldSpin;
        }
    }
    emit controlEnabledChanged(m_controlEnabled);
}

void RangerMiniController::setLinearVelocity(double mps)
{
    // 仅缓存目标值（m/s）；真正下发在心跳 sendMotionFrame 中换算为 mm/s 并限幅
    m_linearMps = mps;
}

void RangerMiniController::setSteerRadians(double radians)
{
    m_steerRad = radians;
}

void RangerMiniController::setSpinRadPerSec(double radPerSec)
{
    m_spinRadPerSec = radPerSec;
}

void RangerMiniController::onCanReady(bool ready)
{
    m_busReady = ready;
    if (!ready) {
        // 总线断开时强制停控，避免假使能状态
        setControlEnabled(false);
    }
    emit isReady(ready);
}

void RangerMiniController::onCanFrame(const QVariantMap &msg)
{
    // 忽略本机发送回显（Loopback/设备 echo），只处理底盘真实反馈
    if (msg.value(QStringLiteral("type")).toString() == QLatin1String("echo")) {
        return;
    }

    const quint32 canId = msg.value(QStringLiteral("id")).toUInt();
    QByteArray payload = msg.value(QStringLiteral("data_raw")).toByteArray();
    if (payload.isEmpty()) {
        // 兼容仅提供 HEX 字符串的情况
        const QString hex = msg.value(QStringLiteral("data")).toString();
        payload = QByteArray::fromHex(QString(hex).remove(QLatin1Char(' ')).toLatin1());
    }
    if (payload.size() < 1) {
        return;
    }

    // —— 0x211 系统状态回馈（周期约 20 ms）——
    if (canId == kFbSystem && payload.size() >= 8) {
        QVariantMap status = m_lastStatus;
        // byte0: 车体状态 0x00 正常 / 0x02 异常
        status.insert(QStringLiteral("vehicle_state"), static_cast<int>(static_cast<quint8>(payload[0])));
        // byte1: 控制模式 0x00 待机 / 0x01 CAN / 0x03 遥控
        status.insert(QStringLiteral("control_mode"), static_cast<int>(static_cast<quint8>(payload[1])));
        // byte2-3: 电池电压，实际电压 = 值 / 10（V）
        status.insert(QStringLiteral("battery_v"), readU16BE(payload, 2) / 10.0);
        // byte4-7: 32 位故障位图（详见手册故障表）
        const quint32 fault = (static_cast<quint32>(static_cast<quint8>(payload[4])) << 24)
            | (static_cast<quint32>(static_cast<quint8>(payload[5])) << 16)
            | (static_cast<quint32>(static_cast<quint8>(payload[6])) << 8)
            | static_cast<quint32>(static_cast<quint8>(payload[7]));
        status.insert(QStringLiteral("fault"), fault);
        // byte7 bit7：急停标志
        status.insert(QStringLiteral("estop"), (static_cast<quint8>(payload[7]) & 0x80) != 0);
        status.insert(QStringLiteral("default"), status.value(QStringLiteral("control_mode")));
        m_lastStatus = status;
        emit statusUpdated(status);
    }
    // —— 0x221 运动控制回馈 ——
    else if (canId == kFbMotion && payload.size() >= 8) {
        QVariantMap fb = m_lastFeedback;
        // byte0-1: 协议为 mm/s（或「×1000 的 0.001m/s」）→ 节点间统一为 m/s
        fb.insert(QStringLiteral("linear_mps"), readI16BE(payload, 0) / 1000.0);
        // byte2-3: 自旋角速度，单位 0.001 rad/s → rad/s
        fb.insert(QStringLiteral("spin_rad_s"), readI16BE(payload, 2) / 1000.0);
        // byte6-7: 转向角，单位 0.001 rad → rad
        fb.insert(QStringLiteral("steer_rad"), readI16BE(payload, 6) / 1000.0);
        fb.insert(QStringLiteral("default"), fb.value(QStringLiteral("linear_mps")));
        m_lastFeedback = fb;
        emit feedbackUpdated(fb);
    }
    // —— 0x291 当前运动模式回馈 ——
    else if (canId == kFbMotionMode && payload.size() >= 3) {
        QVariantMap status = m_lastStatus;
        status.insert(QStringLiteral("motion_mode"), static_cast<int>(static_cast<quint8>(payload[0])));
        // 0x01 表示切换过程中，此时不响应速度指令
        status.insert(QStringLiteral("mode_switching"), static_cast<int>(static_cast<quint8>(payload[1])));
        // 0x00 电流驱动 / 0x01 电压驱动
        status.insert(QStringLiteral("drive_mode"), static_cast<int>(static_cast<quint8>(payload[2])));
        m_lastStatus = status;
        emit statusUpdated(status);
    }
}

void RangerMiniController::onHeartbeat()
{
    if (!m_controlEnabled || !m_busReady) {
        return;
    }
    sendMotionFrame();
}

void RangerMiniController::sendRaw(quint32 canId, const QByteArray &data)
{
    // extended=false, rtr=false：标准数据帧
    m_can->sendFrame(canId, data, false, false);
}

void RangerMiniController::sendControlMode(quint8 mode)
{
    sendRaw(kCmdControlMode, QByteArray(1, static_cast<char>(mode)));
}

void RangerMiniController::sendMotionMode(quint8 mode)
{
    sendRaw(kCmdMotionMode, QByteArray(1, static_cast<char>(mode)));
}

void RangerMiniController::sendClearFault(quint8 code)
{
    sendRaw(kCmdClearFault, QByteArray(1, static_cast<char>(code)));
}

void RangerMiniController::sendMotionFrame()
{
    // 0x111 共 8 字节：
    // [0-1] 线速度 mm/s
    // [2-3] 自旋 0.001 rad/s
    // [4-5] 保留
    // [6-7] 转向内转角 0.001 rad
    QByteArray data(8, '\0');
    writeI16BE(data, 0, clampedLinearMmPerSec());
    writeI16BE(data, 2, clampedSpinMradPerSec());
    writeI16BE(data, 6, clampedSteerMrad());
    sendRaw(kCmdMotion, data);
}

void RangerMiniController::writeI16BE(QByteArray &data, int offset, qint16 value)
{
    data[offset] = static_cast<char>((value >> 8) & 0xFF);
    data[offset + 1] = static_cast<char>(value & 0xFF);
}

qint16 RangerMiniController::readI16BE(const QByteArray &data, int offset)
{
    return static_cast<qint16>((static_cast<quint8>(data[offset]) << 8)
                               | static_cast<quint8>(data[offset + 1]));
}

quint16 RangerMiniController::readU16BE(const QByteArray &data, int offset)
{
    return static_cast<quint16>((static_cast<quint8>(data[offset]) << 8)
                                | static_cast<quint8>(data[offset + 1]));
}

qint16 RangerMiniController::clampedLinearMmPerSec() const
{
    if (m_motionMode == Spin || m_motionMode == Park) {
        return 0;
    }
    // 节点间 m/s → 协议 mm/s；有效 ±2000 mm/s；转角 >20° 时 ±700
    const double mm = m_linearMps * 1000.0;
    const double limit = (qAbs(m_steerRad) > qDegreesToRadians(20.0)) ? 700.0 : 2000.0;
    return static_cast<qint16>(std::clamp(mm, -limit, limit));
}

qint16 RangerMiniController::clampedSpinMradPerSec() const
{
    if (m_motionMode != Spin) {
        return 0;
    }
    // 协议单位 0.001 rad/s，有效值 ±3259
    const double mrad = m_spinRadPerSec * 1000.0;
    return static_cast<qint16>(std::clamp(mrad, -3259.0, 3259.0));
}

qint16 RangerMiniController::clampedSteerMrad() const
{
    if (m_motionMode == Spin || m_motionMode == Park) {
        return 0;
    }
    // 节点侧单位 rad；CAN 写入「转角 ×1000」（0.001 rad），左转为正
    const double mrad = m_steerRad * 1000.0;
    // 阿克曼 ±0.698 rad，斜移 ±1.571 rad
    const double limit = (m_motionMode == Diagonal) ? 1571.0 : 698.0;
    return static_cast<qint16>(std::clamp(mrad, -limit, limit));
}
