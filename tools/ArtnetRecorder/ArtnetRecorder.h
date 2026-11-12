// 移除复杂的头文件引用
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QGridLayout>
#include <QWidget>
#include "ArtnetReceiver.h"
#include "DmxGridView.h"
#include "VideoRecorder.h"
#include <QElapsedTimer>
#include <QHash>

class IntDragValueWidget;

/**
 * @brief Artnet录制器主窗口类
 * 
 * 提供音频录制、播放控制界面和Artnet数据可视化
 */
class ArtnetRecorder : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit ArtnetRecorder(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ArtnetRecorder();

// 移除复杂的方法声明
private slots:
    void onStartClicked();
    void onNewClicked();                    // 新增：New按钮点击事件
    void onUniverseSettingsChanged();
    void onArtnetDataReceived(QVariantMap &data);
    void captureFrame();
    /**
     * @brief 等待非零DMX信号的定时器槽
     * 轮询检测是否有非零DMX数据，一旦检测到则自动开始录制。
     */
    void handleWaitTimer();

    /**
     * @brief 定时刷新信号源信息条（500ms，低频批量刷新UI）
     * 高频计数在 onArtnetDataReceived 中累加，此处只做归并与显示
     */
    void refreshStats();

    /**
     * @brief 状态灯闪烁节拍（500ms）
     * 信号正常=每拍翻转（1Hz 心跳）；无信号=每两拍翻转（0.5Hz 缓闪）
     */
    void onBlinkTick();

private:
    /**
     * @brief 初始化界面UI
     */
    void setupUI();
    
    /**
     * @brief 初始化界面连接
     */
    void setupConnections();
    
    /**
     * @brief 初始化Artnet接收器
     */
    void setupArtnetReceiver();
    
    /**
     * @brief 初始化视频录制器
     */
    void setupVideoRecorder();

    /**
     * @brief 格式化时间显示
     * @param frameCounts 帧计数
     * @return 格式化的时间字符串
     */
    QString formatTime(qint64 frameCounts);

    /**
     * @brief 更新按钮状态
     */
    void updateButtonStates();              // 新增：更新按钮状态方法

    /**
     * @brief 按实际收到的 Universe 重建左侧域列表（保持当前选择）
     */
    void rebuildUniverseList();

    /**
     * @brief 更新信号状态标签（正常/无信号）的文本与样式
     * @param hasSignal 最近1.5秒内是否收到过Art-Net包
     */
    void updateStatusLabel(bool hasSignal);
    
signals:
    void cleanUp();

private:

    QGridLayout *m_mainLayout;          // 主布局
    DmxGridView *m_previewWidget;       // DMX 通道网格预览控件
    QLabel *m_startUniverseLabel;       // 开始Universe标签
    QLabel *m_endUniverseLabel;         // 结束Universe标签
    IntDragValueWidget *m_startUniverseBox;   // 开始Universe拖拽数值框
    IntDragValueWidget *m_endUniverseBox;     // 结束Universe拖拽数值框
    QListWidget *m_universeList;        // 域列表（显示实际收到的 Universe，选中即预览）
    QLabel *m_timeLabel;                // 时间显示标签
    QPushButton *m_recordButton;         // 开始按钮
    QPushButton *m_newButton;         // 新建按钮
    // 录制相关
    VideoRecorder *m_videoRecorder;     // 视频录制器
    QTimer *m_captureTimer;             // 帧捕获定时器
    bool m_isRecording;                 // 录制状态
    ArtnetReceiver *m_artnetReceiver;   // Artnet接收器
    QString m_currentFileName;          // 当前录制文件名
    bool m_hasValidFile;                // 是否有有效的录制文件

    /**
     * @brief 非阻塞等待非零DMX的定时器
     * 用于开始录制前的轮询检测（50ms 间隔）。
     */
    QTimer *m_waitTimer;                // 等待非零DMX定时器
    bool m_waitingForSignal;            // 当前是否处于等待模式

    // ---- 左侧面板：信号状态 ----
    QLabel *m_statusLabel;              // 信号状态（正常/无信号）
    void updateStatusText();            // 按闪烁相位刷新状态文本（圆点颜色）
    QTimer *m_statsTimer;               // 统计刷新定时器（状态灯刷新 + 过期记录清理）
    QTimer *m_blinkTimer;               // 状态灯闪烁定时器（500ms 节拍）
    bool m_signalActive;                // 状态灯当前是否有信号
    bool m_blinkPhase;                  // 状态灯闪烁相位（亮点/暗点）
    int m_blinkTick;                    // 闪烁节拍计数
    QElapsedTimer m_statsClock;         // 统计时钟（程序启动基准）
    qint64 m_lastPacketMs;              // 最近收到 Art-Net 包的时刻（ms，m_statsClock 基准）
    qint64 m_windowPackets;             // 当前统计窗口内的包计数
    qint64 m_droppedPackets;            // 累计丢包数
    QHash<QString, qint64> m_hostLastSeenMs;   // 源主机 -> 最后活跃时刻(ms)
    QHash<int, qint64> m_universeLastSeenMs;   // Universe -> 最后活跃时刻(ms)
    QHash<QString, quint8> m_lastSequence;     // "host:universe" -> 最后 sequence（丢包检测）
};

#endif // MAINWINDOW_H



