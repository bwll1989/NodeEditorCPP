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
    setToolTip(tr("总输出电平（RMS，Gist 检测）：条色随电平渐变 绿→黄→红"
                  "（-12dB 转黄、-3dB 转红）；白线 = 峰值保持。"
                  "timeline 混音（TimelineAudioBusHost）电平可视化"));
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

    // 电平条渐变（声压级越高越暖：绿→黄→橙红。渐变坐标锚定整槽宽度——
    // 颜色随位置固定、条长揭示刻度，对齐专业电平表观感；单矩形替代旧
    // 三段裁剪绘制，一次 drawRoundedRect 完成）
    if (norm_ > 0.0f) {
        QLinearGradient grad(rc.left(), 0.0, rc.right(), 0.0);
        grad.setColorAt(0.0,                 QColor(0x43, 0xA0, 0x47)); // 绿（低电平）
        grad.setColorAt(0.55,                QColor(0x5C, 0xAD, 0x44)); // 绿保持段（避免过早泛黄）
        grad.setColorAt(dbToNorm(kDbYellow), QColor(0xF3, 0xB0, 0x2C)); // 黄（-12dB 分界）
        grad.setColorAt(dbToNorm(kDbRed),    QColor(0xE0, 0x5A, 0x2E)); // 橙红（-3dB 分界）
        grad.setColorAt(1.0,                 QColor(0xD8, 0x33, 0x2A)); // 红（0dB 满）
        const QRectF bar(rc.left(), rc.top() + 2,
                         rc.width() * norm_, rc.height() - 4);
        p.setBrush(grad);
        p.drawRoundedRect(bar, 2, 2);
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
