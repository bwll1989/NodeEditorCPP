// ============================================================================
//  AudioLevelMeter.cpp —— 总输出电平表实现
//  （MediaServer 收编恢复，2026-11-14；原 src/editor/AudioLevelMeter.cpp）
// ============================================================================

#include "AudioLevelMeter.hpp"

#include <chrono>
#include <cmath>

#include <QPainter>
#include <QPainterPath>

namespace {

// dBFS 刻度端点（电平表动态度：-60dB 起、0dB 满）
constexpr float kDbMin = -60.0f;
constexpr float kDbMax = 0.0f;
// 三段分界（经典电平表配色：<-12dB 绿 / -12..-3dB 黄 / >-3dB 红）
constexpr float kDbYellow = -12.0f;
constexpr float kDbRed    = -3.0f;
// 峰值保持衰减期（ms）：超时后峰值随条长回落
constexpr qint64 kPeakHoldMs = 1500;

/// 线性 RMS → 归一化 [0,1]（dBFS 映射；0 rms → 0）
float rmsToNorm(float rms) {
    if (rms <= 1e-6f) return 0.0f; // -120dB 以下按零（表底噪死区）
    const float db = 20.0f * std::log10(rms);
    return std::clamp((db - kDbMin) / (kDbMax - kDbMin), 0.0f, 1.0f);
}

/// dB 阈值 → 归一化位置
float dbToNorm(float db) {
    return std::clamp((db - kDbMin) / (kDbMax - kDbMin), 0.0f, 1.0f);
}

} // namespace

AudioLevelMeter::AudioLevelMeter(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("audioLevelMeter"));
    setToolTip(QStringLiteral("总输出电平（RMS，Gist 检测）：绿 -60~-12dB / 黄 -12~-3dB / 红 -3~0dB；"
                              "白线 = 峰值保持。timeline 混音（TimelineAudioBusHost）电平可视化"));
    setFixedHeight(16);
    setMinimumWidth(90);
}

void AudioLevelMeter::setLevel(float rms) {
    const float new_norm = rmsToNorm(rms);
    // 峰值保持：新峰立即命中；超衰减期后随当前条回落（缓释视觉）
    const auto now_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
    float new_peak = peak_;
    if (new_norm >= peak_) {
        new_peak  = new_norm;
        peak_ms_ = now_ms;
    } else if (now_ms - peak_ms_ > kPeakHoldMs) {
        new_peak  = new_norm;
        peak_ms_ = now_ms;
    }
    // 值未变不重绘（33ms 高频灌值；播放暂停后恒静音窗口尤甚）
    if (new_norm == norm_ && new_peak == peak_) return;
    norm_ = new_norm;
    peak_ = new_peak;
    update();
}

void AudioLevelMeter::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF rc = rect().adjusted(0.5, 0.5, -0.5, -0.5);

    // 底槽（暗灰平涂）
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0x2A, 0x2A, 0x2A));
    p.drawRoundedRect(rc, 3, 3);

    // 电平条三段平色（裁剪绘制：整条按 norm_ 宽度，三段各自颜色区域）
    if (norm_ > 0.0f) {
        const float y0 = dbToNorm(kDbYellow);
        const float yr = dbToNorm(kDbRed);
        auto seg = [&](float from, float to, QColor col) {
            const float lo = std::min(from, norm_);
            const float hi = std::min(to, norm_);
            if (hi <= lo) return;
            QRectF r(rc.left() + rc.width() * lo, rc.top() + 2,
                     rc.width() * (hi - lo), rc.height() - 4);
            p.setBrush(col);
            p.drawRoundedRect(r, 2, 2);
        };
        seg(0.0f, y0, QColor(0x43, 0xA0, 0x47)); // 绿
        seg(y0, yr, QColor(0xF3, 0xB0, 0x2C));   // 黄
        seg(yr, 1.0f, QColor(0xD8, 0x43, 0x3A)); // 红
    }

    // 峰值保持线（白色细线）
    if (peak_ > 0.0f) {
        const qreal x = rc.left() + rc.width() * peak_;
        p.setPen(QPen(QColor(0xF2, 0xF2, 0xF2), 1.0));
        p.drawLine(QPointF(x, rc.top() + 1), QPointF(x, rc.bottom() - 1));
    }

    // 边框
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0, 0, 0, 60), 0.5));
    p.drawRoundedRect(rc, 3, 3);
}
