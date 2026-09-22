//
// Created by TRAE on 2026/09/15.
// 音频波形展示控件
// - 绘制多声道 max 合并的对称像素波形
// - 支持播放头指示与局部刷新
// - 支持鼠标拖拽 / 点击定位（seek）
//

#pragma once

#include <QWidget>
#include <QColor>
#include <memory>
#include "AudioWaveformPeaks.hpp"

#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

class GUI_ELEMENTS_EXPORT AudioWaveformWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioWaveformWidget(QWidget* parent = nullptr);
    ~AudioWaveformWidget() override;

    void setPeaks(std::shared_ptr<AudioWaveformPeaks> peaks);
    std::shared_ptr<AudioWaveformPeaks> peaks() const { return m_peaks; }

    void setDemoData(int bins, int sampleRate = 48000);

    /**
     * @brief 设置播放头（秒）。-1 表示不显示播放头。
     */
    void setPlayheadSec(double sec);
    double playheadSec() const { return m_playheadSec; }

    /**
     * @brief 无峰值数据时仍可用的总时长（秒），供 seek 换算；有 peaks 时优先用 peaks
     */
    void setDurationSec(double sec);
    double durationSec() const;

    void setPlaceholderText(const QString& text);

    void setColors(const QColor& bg, const QColor& wave, const QColor& head = Qt::white,
                   const QColor& playedWave = QColor());

    /** 是否允许拖拽/点击 seek，默认 true */
    void setSeekEnabled(bool enabled);
    bool seekEnabled() const { return m_seekEnabled; }

signals:
    /**
     * @brief 用户点击或拖拽请求跳转（秒）
     * @param sec 目标时间，已钳制到 [0, duration]
     * @param dragging true 表示拖拽中的中间更新；false 表示按下或松开的确定位置
     */
    void seekRequested(double sec, bool dragging);

protected:
    void paintEvent(QPaintEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    int playheadToX(double sec) const;
    double xToSec(int x) const;
    void updatePlayheadRect(double oldSec, double newSec);
    void emitSeekFromPos(const QPoint& pos, bool dragging);

    QColor m_bgColor = QColor(0x1e, 0x1b, 0x4b);
    QColor m_waveColor = QColor(0x81, 0x8c, 0xf8);
    QColor m_playedWaveColor = QColor(0xff, 0xff, 0xff, 240);
    QColor m_playheadColor = Qt::white;

    std::shared_ptr<AudioWaveformPeaks> m_peaks;
    QString m_placeholderText = QStringLiteral("请先选择音频文件");
    double m_playheadSec = -1.0;
    double m_durationSec = 0.0;     // peaks 未就绪时的兜底总时长
    QString m_cachedTimeText;
    bool m_seekEnabled = true;
    bool m_dragging = false;
};
