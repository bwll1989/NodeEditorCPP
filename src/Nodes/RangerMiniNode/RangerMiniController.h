#pragma once

#include <QObject>
#include <QTimer>
#include <QVariantMap>

#include "Common/Devices/CanBus/CanBus.h"

/**
 * @brief AgileX RANGER MINI 3.0 底盘 CAN 控制器
 *
 * 基于 candleLight / gs_usb（WinUSB）适配器，按官方手册 CAN 协议控制四轮四转底盘。
 *
 * 协议要点：
 * - CAN 2.0B，波特率固定 500 kbps，字节序为大端（Motorola）
 * - 运动指令帧 ID = 0x111，接收超时约 500 ms；本类以 20 ms 心跳持续下发
 * - 上电默认待机模式，需先发 0x421=0x01 进入「CAN 指令模式」后才响应运动指令
 * - 遥控器优先级最高；遥控抢权时本类指令会被屏蔽
 *
 * 节点间单位约定：线速度 m/s、转角 rad、自旋 rad/s。
 * CAN 边界再换算：线速度 → mm/s；转角/自旋 → 0.001 rad（×1000）。
 */
class RangerMiniController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 底盘运动模型（对应协议 0x141）
     */
    enum MotionMode : int {
        Ackermann = 0, ///< 前后阿克曼：线速度 + 转向角
        Diagonal = 1,  ///< 斜移：线速度 + 转向角（转角范围更大，可达横移）
        Spin = 2,      ///< 自旋：仅使用自旋角速度
        Park = 3       ///< 驻车：X 形锁轮，速度指令无效
    };

    explicit RangerMiniController(QObject *parent = nullptr);
    ~RangerMiniController() override;

    /** 枚举本机可用的 candleLight / gs_usb 设备列表 */
    static QList<CanBusDeviceInfo> availableDevices() { return CanBus::availableDevices(); }

public slots:
    /**
     * @brief 打开指定索引的 USB-CAN 设备
     * @param deviceIndex 设备索引；<0 时关闭当前连接
     * @note 固定以 500 kbps、Normal 模式打开（非 Loopback）
     */
    void openDevice(int deviceIndex);

    /** 关闭 CAN 连接，并先停止运动控制 */
    void closeDevice();

    /**
     * @brief 进入 CAN 指令控制流程
     *
     * 依次发送：控制模式 0x421=0x01 → 清故障 0x441=0x00 → 当前运动模型 0x141
     */
    void enterCanControl();

    /**
     * @brief 清除全部非严重故障（含急停释放后清除）
     * @note 对应协议 0x441，数据 0x00
     */
    void clearFaults();

    /**
     * @brief 设置运动模型
     * @param mode MotionMode 枚举值 0~3；总线已连接时立即下发 0x141
     */
    void setMotionMode(int mode);

    /**
     * @brief 使能 / 关闭运动控制心跳
     * @param enabled true：进入 CAN 模式并启动 20 ms 心跳；false：发若干零速帧后停止心跳
     */
    void setControlEnabled(bool enabled);

    /**
     * @brief 设置线速度（节点间单位）
     * @param mps 单位 m/s，前进为正、倒退为负；CAN 下发时 ×1000 → mm/s
     */
    void setLinearVelocity(double mps);

    /**
     * @brief 设置转向内转角
     * @param radians 单位 rad，左转为正；阿克曼/斜移模式生效
     * @note CAN 下发时 ×1000 → 协议 0.001 rad
     */
    void setSteerRadians(double radians);

    /**
     * @brief 设置自旋角速度
     * @param radPerSec 单位 rad/s，逆时针为正；仅 Spin 模式生效
     * @note CAN 下发时 ×1000 → 协议 0.001 rad/s
     */
    void setSpinRadPerSec(double radPerSec);

signals:
    /** CAN 总线是否已打开 */
    void isReady(bool ready);
    /** 运动控制使能状态变化（UI / 属性同步用） */
    void controlEnabledChanged(bool enabled);
    /** 底层或协议层错误信息 */
    void errorOccurred(const QString &message);
    /**
     * 系统状态更新（主要来自 0x211 / 0x291）
     * 常用键：control_mode / battery_v / estop / fault / motion_mode 等
     */
    void statusUpdated(const QVariantMap &status);
    /**
     * 运动反馈更新（来自 0x221）
     * 常用键：linear_mps / spin_rad_s / steer_rad
     */
    void feedbackUpdated(const QVariantMap &feedback);

private slots:
    /** CanBus 连接状态回调 */
    void onCanReady(bool ready);
    /** 解析底盘反馈 CAN 帧 */
    void onCanFrame(const QVariantMap &msg);
    /** 20 ms 定时器：周期发送运动指令 0x111 */
    void onHeartbeat();

private:
    /** 发送标准帧（11-bit ID，非扩展、非 RTR） */
    void sendRaw(quint32 canId, const QByteArray &data);
    /** 0x421：控制模式（0x00 待机 / 0x01 CAN 指令） */
    void sendControlMode(quint8 mode);
    /** 0x141：运动模型 */
    void sendMotionMode(quint8 mode);
    /** 0x441：清故障码 */
    void sendClearFault(quint8 code);
    /** 组装并发送 0x111 运动控制帧 */
    void sendMotionFrame();

    /** 大端写入有符号 16 位（协议 Motorola） */
    static void writeI16BE(QByteArray &data, int offset, qint16 value);
    /** 大端读取有符号 16 位 */
    static qint16 readI16BE(const QByteArray &data, int offset);
    /** 大端读取无符号 16 位 */
    static quint16 readU16BE(const QByteArray &data, int offset);

    /**
     * 按当前模式与转向角限制后的线速度（协议 mm/s）
     * 自旋/驻车强制 0；转角>|20°|（约 0.349 rad）时限 ±700，否则 ±2000
     */
    qint16 clampedLinearMmPerSec() const;
    /** 自旋模式：rad/s → 0.001 rad/s，范围 ±3259；非自旋强制 0 */
    qint16 clampedSpinMradPerSec() const;
    /** 阿克曼/斜移：rad → 0.001 rad；阿克曼 ±0.698，斜移 ±1.571；自旋/驻车强制 0 */
    qint16 clampedSteerMrad() const;

    CanBus *m_can = nullptr;       ///< USB-CAN 门面（内部 Worker 线程）
    QTimer *m_heartbeat = nullptr; ///< 运动指令心跳定时器

    bool m_busReady = false;       ///< CAN 是否已连接
    bool m_controlEnabled = false; ///< 是否正在下发运动心跳
    int m_motionMode = Ackermann;  ///< 当前运动模型

    double m_linearMps = 0.0;      ///< 目标线速度 m/s（节点间）
    double m_steerRad = 0.0;       ///< 目标转向角 rad
    double m_spinRadPerSec = 0.0;  ///< 目标自旋角速度 rad/s

    QVariantMap m_lastStatus;   ///< 最近一次 STATUS 缓存（合并 0x211/0x291）
    QVariantMap m_lastFeedback; ///< 最近一次 FEEDBACK 缓存（0x221）
};
