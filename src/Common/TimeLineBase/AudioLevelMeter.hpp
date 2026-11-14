// ============================================================================
//  AudioLevelMeter —— 总输出电平表（总 audio 输出的 RMS 电平显示）
//
//  归属（2026-11-14）：TimeLineBase 库内置——DefaultTimeLineToolBar 右缘
//  自挂（弹簧+addWidget），宿主经 setAudioLevel() 灌值。出处：MediaServer
//  收编（原 src/editor/AudioLevelMeter，恢复合并前「时间轴工具栏电平表」
//  ——合并时此控件未随迁）。设计依据：《audio-output-implementation-
//  plan.md》P5（2026-11-10 验证阶段先行）：
//    * 数据源 = AudioEngine::level()（Gist rootMeanSquare，最近混合窗口
//      RMS，原子发布）——UI 拍轮询灌值（33ms tick），控件无内部线程；
//    * 刻度 = dBFS：20·log10(rms)，映射 [-60dB, 0dB] → [0,1] 条长；
//    * 峰值保持：峰值 1.5s 衰减期（视觉锚——瞬时 RMS 抖动大，峰值线帮助
//      读数）；条体三段平色（绿/黄/红——经典电平表配色，平涂无渐变）；
//    * 纯显示控件：不写回、无信号——音频链路的只读观测面。
// ============================================================================

#pragma once

#include <QWidget>

// 全局命名空间（对齐 TimeLineBase 库内风格；原 MediaServer editor 代码
// 包 ms:: 命名空间，2026-11-14 迁入时剥除——DefaultTimeLineToolBar 等库内
// 消费者均为全局命名空间类，裸名直用）。
class AudioLevelMeter final : public QWidget {
    Q_OBJECT
public:
    explicit AudioLevelMeter(QWidget* parent = nullptr);

    /// 灌入最近窗口 RMS（线性 [0,~1+]；内部转 dBFS 映射条长 + 峰值保持）
    void setLevel(float rms);

    QSize sizeHint() const override { return {120, 16}; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    float  norm_     = 0.0f; ///< 归一化条长 [0,1]（dBFS 映射后）
    float  peak_     = 0.0f; ///< 峰值保持（归一化域）
    qint64 peak_ms_  = 0;    ///< 峰值最后命中时刻（steady_clock ms；衰减期用）
};
