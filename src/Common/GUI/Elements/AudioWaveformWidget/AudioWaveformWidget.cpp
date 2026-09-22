#include "AudioWaveformWidget.hpp"
#include <QPainter>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>
#include <random>

AudioWaveformWidget::AudioWaveformWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(80);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, false);
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(false);
}

AudioWaveformWidget::~AudioWaveformWidget() = default;

void AudioWaveformWidget::setColors(const QColor& bg, const QColor& wave, const QColor& head,
                                     const QColor& playedWave)
{
    m_bgColor = bg;
    m_waveColor = wave;
    m_playheadColor = head;
    if (playedWave.isValid()) {
        m_playedWaveColor = playedWave;
    } else {
        int h = 0, s = 0, v = 0, a = 255;
        wave.getHsv(&h, &s, &v, &a);
        v = qBound(0, v + 70, 255);
        s = qBound(0, s + 30, 255);
        m_playedWaveColor = QColor::fromHsv(h, s, v, qBound(80, a + 15, 255));
    }
    update();
}

void AudioWaveformWidget::setPlaceholderText(const QString& text)
{
    if (text == m_placeholderText) return;
    m_placeholderText = text;
    update();
}

void AudioWaveformWidget::setSeekEnabled(bool enabled)
{
    m_seekEnabled = enabled;
    setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void AudioWaveformWidget::setDurationSec(double sec)
{
    m_durationSec = sec > 0.0 ? sec : 0.0;
}

double AudioWaveformWidget::durationSec() const
{
    if (m_peaks && m_peaks->isValid() && m_peaks->totalSeconds() > 0.0) {
        return m_peaks->totalSeconds();
    }
    return m_durationSec;
}

int AudioWaveformWidget::playheadToX(double sec) const
{
    const double total = durationSec();
    if (total <= 0.0) return -1;
    if (sec < 0.0) sec = 0.0;
    if (sec > total) sec = total;
    const double ratio = sec / total;
    const int leftPad = 2;
    const int rightPad = 2;
    const int innerW = std::max(1, width() - leftPad - rightPad);
    return leftPad + static_cast<int>(ratio * static_cast<double>(innerW) + 0.5);
}

double AudioWaveformWidget::xToSec(int x) const
{
    const double total = durationSec();
    if (total <= 0.0) return 0.0;
    const int leftPad = 2;
    const int rightPad = 2;
    const int innerW = std::max(1, width() - leftPad - rightPad);
    const double ratio = qBound(0.0, static_cast<double>(x - leftPad) / static_cast<double>(innerW), 1.0);
    return ratio * total;
}

void AudioWaveformWidget::emitSeekFromPos(const QPoint& pos, bool dragging)
{
    const double total = durationSec();
    if (total <= 0.0) {
        return;
    }
    const double sec = xToSec(pos.x());
    setPlayheadSec(sec);
    emit seekRequested(sec, dragging);
}

void AudioWaveformWidget::updatePlayheadRect(double oldSec, double newSec)
{
    const int xOld = playheadToX(oldSec);
    const int xNew = playheadToX(newSec);
    const int margin = 3;
    const int h = height();
    const int textRowH = 16;
    const int textRowY = std::max(0, h - textRowH);
    auto makeRow = [&](int x) -> QRect {
        return QRect(std::max(0, x - margin), 0, margin * 2 + 1, h);
    };
    QRegion reg(QRect(0, textRowY, width(), h - textRowY));
    if (xOld >= 0) reg = reg.united(makeRow(xOld));
    if (xNew >= 0 && xNew != xOld) reg = reg.united(makeRow(xNew));
    // 拖拽时已播放/未播放分色区域会大片变化，整幅刷新更稳
    if (m_dragging && xOld >= 0 && xNew >= 0) {
        const int xMin = std::min(xOld, xNew) - margin;
        const int xMax = std::max(xOld, xNew) + margin;
        reg = reg.united(QRect(std::max(0, xMin), 0, std::max(1, xMax - xMin), h));
    }
    if (xOld < 0 && xNew < 0) { update(); return; }
    const auto rects = reg.rects();
    for (const auto& r : rects) update(r);
}

void AudioWaveformWidget::setPlayheadSec(double sec)
{
    double s = sec;
    const double total = durationSec();
    if (total > 0.0) {
        if (s < 0.0) s = 0.0;
        else if (s > total) s = total;
    }
    const double oldCanonical = (m_playheadSec < 0.0) ? -1.0 : m_playheadSec;
    const double newCanonical = (sec < 0.0) ? -1.0 : s;
    const auto fmt = [](double secVal, double totalVal) -> QString {
        const double cur = secVal < 0.0 ? 0.0 : secVal;
        const double tot = totalVal > 0.0 ? totalVal : 0.0;
        const auto f = [](double v) -> QString {
            int s2 = static_cast<int>(v);
            const int m = s2 / 60;
            s2 %= 60;
            const int cs = static_cast<int>((v - static_cast<double>(static_cast<int>(v))) * 100.0);
            return QString("%1:%2.%3")
                .arg(m, 2, 10, QChar('0'))
                .arg(s2, 2, 10, QChar('0'))
                .arg(cs, 2, 10, QChar('0'));
        };
        return QString("%1 / %2").arg(f(cur), f(tot));
    };
    const QString newText = fmt(newCanonical, total);
    const bool timeChanged = (newText != m_cachedTimeText);
    const bool posChanged = (std::fabs(newCanonical - oldCanonical) >= 1e-3);
    if (!posChanged && !timeChanged) return;

    m_cachedTimeText = newText;
    const double oldSec = m_playheadSec;
    m_playheadSec = s;
    updatePlayheadRect(oldSec, s);
}

void AudioWaveformWidget::setDemoData(int bins, int sampleRate)
{
    auto demo = std::make_shared<AudioWaveformPeaks>();
    if (bins <= 0) bins = 4096;
    demo->beginBuild(1, sampleRate);
    const int samplesPerBin = AudioWaveformPeaks::kSamplesPerBin;
    std::mt19937_64 rng(1234);
    std::uniform_real_distribution<float> n(-0.3f, 0.3f);
    float phase = 0.0f;
    for (int b = 0; b < bins; ++b) {
        const float env = 0.35f + 0.55f * (0.5f + 0.5f * std::sin(6.2831853f * static_cast<float>(b) / 128.0f));
        phase += 0.11f;
        for (int i = 0; i < samplesPerBin; ++i) {
            const float v = env * (std::sin(phase + 0.02f * static_cast<float>(i)) * 0.85f + n(rng));
            demo->feedInterleaved(&v, 1, 1);
        }
    }
    demo->endBuild();
    setPeaks(demo);
}

void AudioWaveformWidget::setPeaks(std::shared_ptr<AudioWaveformPeaks> peaks)
{
    if (m_peaks == peaks) return;
    m_peaks = std::move(peaks);
    m_playheadSec = -1.0;
    m_cachedTimeText.clear();
    if (m_peaks && m_peaks->isValid()) {
        m_durationSec = m_peaks->totalSeconds();
    }
    update();
}

void AudioWaveformWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    update();
}

void AudioWaveformWidget::mousePressEvent(QMouseEvent* e)
{
    if (!m_seekEnabled || e->button() != Qt::LeftButton || durationSec() <= 0.0) {
        QWidget::mousePressEvent(e);
        return;
    }
    // 不使用 grabMouse()：节点里嵌在 QGraphicsProxyWidget / 不可见顶层窗口时会触发
    // QWindowsWindow::setMouseGrabEnabled: Not setting mouse grab for invisible window
    m_dragging = true;
    emitSeekFromPos(e->pos(), false);
}

void AudioWaveformWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_dragging || !(e->buttons() & Qt::LeftButton)) {
        QWidget::mouseMoveEvent(e);
        return;
    }
    emitSeekFromPos(e->pos(), true);
}

void AudioWaveformWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (!m_dragging || e->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(e);
        return;
    }
    m_dragging = false;
    emitSeekFromPos(e->pos(), false);
}

void AudioWaveformWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    const QRect r = rect();

    p.fillRect(r, m_bgColor);

    if (!m_peaks || !m_peaks->isValid()) {
        p.setPen(QColor(255, 255, 255, 150));
        QFont f = font();
        f.setPointSize(std::max(8, f.pointSize()));
        p.setFont(f);
        p.drawText(r, Qt::AlignCenter, m_placeholderText);
        // 无 peaks 时若有 duration + playhead，仍画播放头与时间
        if (m_playheadSec >= 0.0 && m_durationSec > 0.0) {
            const int xh = playheadToX(m_playheadSec);
            if (xh >= 2 && xh <= width() - 2) {
                p.setPen(QPen(m_playheadColor, 1));
                p.drawLine(xh, 2, xh, height() - 2);
            }
            if (!m_cachedTimeText.isEmpty()) {
                p.setPen(QColor(255, 255, 255, 210));
                QFont tf = font();
                tf.setPointSize(std::max(7, tf.pointSize() - 1));
                p.setFont(tf);
                p.drawText(r.adjusted(0, 0, -4, -2), Qt::AlignRight | Qt::AlignBottom, m_cachedTimeText);
            }
        }
        return;
    }

    const double total = m_peaks->totalSeconds();
    if (total <= 0.0) return;

    const int leftPad = 2;
    const int rightPad = 2;
    const int topPad = 2;
    const int bottomPad = 2;
    const int W = std::max(1, width() - leftPad - rightPad);
    const int H = std::max(2, height() - topPad - bottomPad);
    const int midY = topPad + H / 2;

    int level = 0, startBin = 0, binCount = 0;
    if (!m_peaks->viewForRange(0.0, total, W, &level, &startBin, &binCount) || binCount <= 0) {
        return;
    }
    const auto& lv = m_peaks->levels()[level];

    bool allSilent = false;
    {
        float maxAbs = 0.0f;
        const int checkBins = std::min(binCount, 2048);
        for (int i = 0; i < checkBins; ++i) {
            const int idx = startBin + (binCount <= 1 ? 0 : (static_cast<long long>(i) * (binCount - 1)) / (checkBins - 1));
            if (idx < 0 || idx >= static_cast<int>(lv.bins)) continue;
            maxAbs = std::max(maxAbs, std::fabs(lv.mins[static_cast<std::size_t>(idx)]));
            maxAbs = std::max(maxAbs, std::fabs(lv.maxs[static_cast<std::size_t>(idx)]));
            if (maxAbs > 0.005f) break;
        }
        allSilent = (maxAbs < 0.005f && W > 2);
    }

    std::vector<QLineF> playedLines;
    std::vector<QLineF> unplayedLines;
    playedLines.reserve(static_cast<std::size_t>(W));
    unplayedLines.reserve(static_cast<std::size_t>(W));
    const int phX = m_playheadSec >= 0.0 ? playheadToX(m_playheadSec) : -1;

    float phasePlace = 0.0f;
    const int segW = std::max(8, W / 12);

    for (int x = 0; x < W; ++x) {
        const int idx = binCount <= 1 ? 0 : (startBin + static_cast<int>((static_cast<long long>(x) * static_cast<long long>(binCount - 1)) / (W - 1)));
        float mn = 0.0f, mx = 0.0f;
        if (allSilent) {
            phasePlace += 0.17f;
            const float env = 0.30f + 0.48f * (0.5f + 0.5f * std::sin(
                6.2831853f * static_cast<float>(x) / static_cast<float>(std::max(1, segW))));
            const float noise = std::fmod(std::sin(static_cast<float>(x) * 12.9898f) * 43758.5453f, 1.0f);
            const float v = env * (std::sin(phasePlace) * 0.80f + noise * 0.25f);
            mn = -std::fabs(v);
            mx = std::fabs(v);
        } else {
            if (idx >= 0 && idx < static_cast<int>(lv.bins)) {
                mn = lv.mins[static_cast<std::size_t>(idx)];
                mx = lv.maxs[static_cast<std::size_t>(idx)];
            }
        }
        auto sampleToY = [&](float v) -> int {
            const float clamped = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
            const float fHalf = static_cast<float>(H) * 0.5f;
            return midY - static_cast<int>(clamped * fHalf + 0.5f);
        };
        const int yMax = sampleToY(mx);
        const int yMin = sampleToY(mn);
        const int xPx = leftPad + x;
        const int top = yMax < yMin ? yMax : yMin;
        const int bot = yMax >= yMin ? yMax : yMin;
        const QLineF line(static_cast<qreal>(xPx) + 0.5f, static_cast<qreal>(top),
                          static_cast<qreal>(xPx) + 0.5f, static_cast<qreal>(bot));
        if (phX >= 0 && xPx <= phX) {
            playedLines.push_back(line);
        } else {
            unplayedLines.push_back(line);
        }
    }
    if (!unplayedLines.empty()) {
        QColor c = m_waveColor;
        if (allSilent) c.setAlpha(std::min(180, std::max(60, c.alpha() * 7 / 10)));
        p.setPen(c);
        p.drawLines(unplayedLines.data(), static_cast<int>(unplayedLines.size()));
    }
    if (!playedLines.empty()) {
        QColor c = m_playedWaveColor;
        if (allSilent) c.setAlpha(std::min(220, std::max(80, c.alpha() * 8 / 10)));
        p.setPen(c);
        p.drawLines(playedLines.data(), static_cast<int>(playedLines.size()));
    }

    if (m_playheadSec >= 0.0) {
        const int xh = playheadToX(m_playheadSec);
        if (xh >= leftPad && xh <= leftPad + W) {
            p.setPen(QPen(m_playheadColor, 1));
            p.drawLine(xh, topPad, xh, topPad + H);
        }
    }

    {
        p.setPen(QColor(255, 255, 255, 210));
        const QString& txt = m_cachedTimeText;
        if (!txt.isEmpty()) {
            QFont f = font();
            f.setPointSize(std::max(7, f.pointSize() - 1));
            p.setFont(f);
            const QRect tr = r.adjusted(0, 0, -4, -2);
            p.drawText(tr, Qt::AlignRight | Qt::AlignBottom, txt);
        }
    }
}
