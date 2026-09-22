//
// Created by TRAE on 2026/09/15.
// 音频峰值后台解码器（独立 FFmpeg 扫描，不打开音频输出设备）
// - 移动到 AudioDecoderNode 插件内部：只有这里需要直接链接 FFmpeg，
//   避免通用 GUIElements 库被 FFmpeg 依赖"绑架"
// - 用于生成 AudioWaveformPeaks 数据；UI 侧通过信号接收 peaks 或进度/错误提示
//

#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include "Elements/AudioWaveformWidget/AudioWaveformPeaks.hpp"

class AudioPeakDecoder : public QObject
{
    Q_OBJECT
public:
    explicit AudioPeakDecoder(QObject* parent = nullptr);
    ~AudioPeakDecoder() override;

    /**
     * @brief 在当前线程同步解码生成峰值数据（一般不要在 UI 线程调用，用 QtConcurrent + watcher 更合适）
     * @param absoluteFilePath 音频文件绝对路径（mp3/wav/flac/aac/…）
     * @param outErrorMsg       [out] 失败时填充错误信息
     * @return 成功返回有效 peaks；失败返回 nullptr
     */
    std::shared_ptr<AudioWaveformPeaks> decodeSync(const QString& absoluteFilePath, QString* outErrorMsg = nullptr);

    /**
     * @brief 返回缓存目录（Documents/Flow/Peaks），不存在会自动创建
     */
    static QString cacheDirectory();

    /**
     * @brief 根据音频文件元信息计算缓存键（大小 + 修改时间 + 绝对路径 → SHA1）
     */
    static QString cacheKeyForFile(const QString& absoluteFilePath);

    /**
     * @brief 尝试读取该文件对应的缓存峰值；命中返回 true 并填充 peaks
     */
    static bool loadFromCache(const QString& absoluteFilePath, std::shared_ptr<AudioWaveformPeaks>& outPeaks);

    /**
     * @brief 将已生成的 peaks 写入该文件对应的缓存
     */
    static bool saveToCache(const QString& absoluteFilePath, const AudioWaveformPeaks& peaks);

signals:
    /**
     * @brief 解码进度（0.0~1.0），可连接到 UI 显示占位百分比
     */
    void progress(double p);

    /**
     * @brief 解码完成
     * @param absoluteFilePath 原输入文件路径（便于 UI 侧做路径匹配，避免"旧请求回调覆盖新请求"的 ABA 问题）
     * @param peaks            解码结果（失败时为 nullptr）
     * @param errorMsg         失败时错误描述
     */
    void finished(QString absoluteFilePath, std::shared_ptr<AudioWaveformPeaks> peaks, QString errorMsg);

public slots:
    /**
     * @brief 异步解码入口（moveToThread 场景调用；也可直接 QtConcurrent::run(&decodeSync,...)）
     *        流程：先查缓存，命中直接 finished；否则 decodeSync + 存缓存 + finished
     */
    void decodeAsync(const QString& absoluteFilePath);

private:
    /**
     * @brief 二进制格式序列化（小端），写入 peaks
     */
    static bool serialize(const AudioWaveformPeaks& peaks, QDataStream& out);
    /**
     * @brief 反序列化，校验 magic/version 后读入 peaks
     */
    static bool deserialize(QDataStream& in, std::shared_ptr<AudioWaveformPeaks>& outPeaks);

    static constexpr quint32 kMagic = 0x46574150;   // 'FWAP'（FlowWaveformAudioPeaks）
    static constexpr quint16 kVersion = 1;
};
