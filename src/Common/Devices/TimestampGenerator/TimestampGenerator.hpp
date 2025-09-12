#ifndef TIMESTAMPGENERATOR_HPP
#define TIMESTAMPGENERATOR_HPP

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QAtomicInteger>
#include <QDateTime>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>  // 添加这个包含用于std::round和std::max

#if defined(TIMESTAMPGENERATOR_LIBRARY)
#define TIMESTAMPGENERATOR Q_DECL_EXPORT
#else
#define TIMESTAMPGENERATOR Q_DECL_IMPORT
#endif

/**
 * @brief 帧信息结构体
 * 包含帧计数和对应的绝对时间信息
 */
struct TIMESTAMPGENERATOR FrameInfo
{
    qint64 frameCount;          // 帧计数（从0开始）
    qint64 absoluteTimeMs;      // 绝对时间（毫秒，基于系统时间）
    std::chrono::high_resolution_clock::time_point preciseTime; // 高精度时间点
    
    /**
     * @brief 默认构造函数
     */
    FrameInfo() : frameCount(0), absoluteTimeMs(0) {}
    
    /**
     * @brief 构造函数
     * @param count 帧计数
     * @param timeMs 绝对时间（毫秒）
     * @param timePoint 高精度时间点
     */
    FrameInfo(qint64 count, qint64 timeMs, const std::chrono::high_resolution_clock::time_point& timePoint)
        : frameCount(count), absoluteTimeMs(timeMs), preciseTime(timePoint) {}
    
    /**
     * @brief 获取相对于基准时间的偏移（毫秒）
     * @param baseTime 基准时间点
     * @return 偏移时间（毫秒）
     */
    qint64 getRelativeTimeMs(const std::chrono::high_resolution_clock::time_point& baseTime) const
    {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(preciseTime - baseTime);
        return duration.count();
    }
    
    /**
     * @brief 获取当前帧的理论时间（基于帧率）
     * @param frameRate 帧率
     * @return 理论时间（毫秒）
     */
    double getTheoreticalTimeMs(int frameRate) const
    {
        return frameCount * (1000.0 / frameRate);
    }
};

/**
 * @brief 全局音频帧计数器生成器
 * 以50fps的频率生成统一的帧计数，确保所有音频组件使用相同的帧基准
 * 采用单例模式，线程安全，使用高精度计时器，实例化时自动启动
 */
class TIMESTAMPGENERATOR TimestampGenerator : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取全局帧计数器生成器实例（单例模式）
     * 实例化时自动启动帧计数生成
     * @return 全局帧计数器生成器实例
     */
    static TimestampGenerator* getInstance();
    
    /**
     * @brief 手动启动帧计数生成（通常不需要调用，实例化时自动启动）
     */
    void start();
    
    /**
     * @brief 停止帧计数生成
     */
    void stop();
    
    /**
     * @brief 重新启动帧计数生成
     */
    void restart();
    
    /**
     * @brief 获取当前帧信息
     * @return 当前帧信息（包含帧计数和绝对时间）
     */
    FrameInfo getCurrentFrameInfo() const;
    
    /**
     * @brief 获取当前全局帧计数
     * @return 当前帧计数（从0开始的int64）
     */
    qint64 getCurrentFrameCount() const;
    
    /**
     * @brief 获取下一个帧计数
     * @return 下一个帧计数
     */
    qint64 getNextFrameCount();
    
    /**
     * @brief 获取下一个帧信息
     * @return 下一个帧信息
     */
    FrameInfo getNextFrameInfo();
    
    /**
     * @brief 重置帧计数器（用于开始新的播放会话）
     */
    void resetFrameCount();
    
    /**
     * @brief 获取帧率
     * @return 帧率（fps）
     */
    double getFrameRate() const { return FRAME_RATE; }
    
    /**
     * @brief 获取帧间隔
     * @return 间隔（毫秒）
     */
    double getFrameInterval() const { return FRAME_INTERVAL_MS; }
    
    /**
     * @brief 检查帧计数器是否正在运行
     * @return 运行状态
     */
    bool isRunning() const;
    
    /**
     * @brief 获取基准时间点
     * @return 基准时间点
     */
    std::chrono::high_resolution_clock::time_point getBaseTime() const { return startTime_; }
    
    /**
     * @brief 析构函数
     */
    ~TimestampGenerator();

    /**
     * @brief 根据时间差计算帧计数
     * @param timeDeltaMs 时间差（毫秒），可以为正数（向前）或负数（向后）
     * @return 计算得到的帧计数
     */
    qint64 calculateFrameCountByTimeDelta(double timeDeltaMs) const;

    /**
     * @brief 根据时间差计算帧信息
     * @param timeDeltaMs 时间差（毫秒），可以为正数（向前）或负数（向后）
     * @return 计算得到的帧信息
     */
    FrameInfo  calculateFrameInfoByTimeDelta(double timeDeltaMs) const;

    /**
     * @brief 根据绝对时间计算帧计数
     * @param absoluteTimeMs 绝对时间（毫秒）
     * @return 计算得到的帧计数，如果时间早于基准时间则返回-1
     */
    qint64  calculateFrameCountByAbsoluteTime(qint64 absoluteTimeMs) const;

    /**
     * @brief 根据相对时间计算帧计数
     * @param relativeTimeMs 相对于开始时间的时间（毫秒）
     * @return 计算得到的帧计数
     */
    qint64  calculateFrameCountByRelativeTime(double relativeTimeMs) const;

signals:
    /**
     * @brief 帧信息更新信号
     * @param frameInfo 新的帧信息
     */
    void frameInfoUpdated(const FrameInfo& frameInfo);
    
    /**
     * @brief 帧计数更新信号（向后兼容）
     * @param frameCount 新的帧计数
     */
    void frameCountUpdated(qint64 frameCount);
    
    /**
     * @brief 帧计数器启动信号
     */
    void started();
    
    /**
     * @brief 帧计数器停止信号
     */
    void stopped();

private:
    /**
     * @brief 私有构造函数（单例模式）
     * @param parent 父对象
     */
    explicit TimestampGenerator(QObject *parent = nullptr);
    
    /**
     * @brief 高精度计时线程函数
     */
    void timerThreadFunction();
    
    /**
     * @brief 生成帧计数
     */
    void generateFrameCount();
    
    /**
     * @brief 创建当前帧信息
     * @param frameCount 帧计数
     * @return 帧信息
     */
    FrameInfo createFrameInfo(qint64 frameCount) const;
    
    // 静态成员
    static TimestampGenerator* instance_;           // 单例实例
    static QMutex instanceMutex_;                   // 实例创建互斥锁
    static inline double FRAME_RATE = 23.4375;              // 23.4375fps
    static const double FRAME_INTERVAL_MS;         // 帧间隔（毫秒）
    static const std::chrono::nanoseconds FRAME_INTERVAL_NS; // 帧间隔（纳秒）
    
    // 成员变量
    mutable QMutex frameCountMutex_;                // 帧计数保护互斥锁
    QAtomicInteger<qint64> frameCounter_;           // 原子帧计数器
    std::atomic<bool> isRunning_;                   // 运行状态
    std::atomic<bool> shouldStop_;                  // 停止标志
    std::unique_ptr<std::thread> timerThread_;      // 高精度计时线程
    
    std::chrono::high_resolution_clock::time_point startTime_; // 开始时间点
    qint64 baseAbsoluteTime_;                       // 基准绝对时间（毫秒）
};

/**
 * @brief 导出函数：获取帧计数器生成器实例
 * @return 帧计数器生成器实例
 */
TIMESTAMPGENERATOR TimestampGenerator* getTimestampGeneratorInstance();
#endif // TIMESTAMPGENERATOR_HPP