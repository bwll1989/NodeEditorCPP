//
// Created by WuBin on 2025/11/3.
// DMX 通道网格预览控件（Flow 品牌视觉 · 纯 QPainter 实现，无 OpenGL 依赖）
//
#include "DmxGridView.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QFont>

namespace {
/**
 * @brief DMX 值 → 单元格颜色（Flow 品牌手册色板）
 * 0 = 暗绿黑 #1C2415（贴近画布底 #141A0D）→ 128 = 信息蓝 #3F6E8E → 255 = ArtNet 青 #4FB8C9
 * （ArtNet 青取自品牌手册时间轴片段色板中的 ArtNet 专属色）
 */
QColor valueColor(int value)
{
    const double t = qBound(0, value, 255) / 255.0;
    if (t < 0.5) {
        const double k = t * 2.0;
        return QColor(int(28 + 35 * k), int(36 + 74 * k), int(21 + 121 * k));
    }
    const double k = (t - 0.5) * 2.0;
    return QColor(int(63 + 16 * k), int(110 + 74 * k), int(142 + 59 * k));
}
} // namespace

// ================= UniverseCaption：顶部 Universe 标题条 =================

/**
 * @brief 标题条：等宽字体显示当前 Universe，右侧为通道数 / 录制指示
 * 实心圆点仅用于状态标识（品牌图标规范：实心留给端口与状态）
 */
class UniverseCaption : public QWidget
{
public:
    explicit UniverseCaption(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFixedHeight(22);
    }

    void setUniverse(int universe)
    {
        if (m_universe != universe) {
            m_universe = universe;
            update();
        }
    }

    void setRecording(bool on)
    {
        if (m_recording != on) {
            m_recording = on;
            update();
        }
    }

protected:
    void paintEvent(QPaintEvent * /*event*/) override
    {
        QPainter p(this);
        p.fillRect(rect(), QColor(0x14, 0x1A, 0x0D));   // 墨绿黑 · 品牌深色底

        QFont f = p.font();
        f.setFamilies({QStringLiteral("Cascadia Code"),
                       QStringLiteral("Consolas"),
                       QStringLiteral("Courier New")});
        f.setPixelSize(11);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        p.setFont(f);

        // 左：ArtNet 领域色圆点 + 当前 Universe（等宽标注）
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x4F, 0xB8, 0xC9));
        p.drawEllipse(QRectF(10.0, height() / 2.0 - 2.5, 5.0, 5.0));
        p.setPen(QColor(0x9B, 0xA2, 0x94));
        p.drawText(QRect(24, 0, width() / 2, height()), Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("UNIVERSE %1").arg(m_universe, 3, 10, QLatin1Char('0')));

        // 右：录制指示（语义红）或通道规格
        p.setPen(m_recording ? QColor(0xC4, 0x43, 0x2E) : QColor(0x9B, 0xA2, 0x94, 190));
        p.drawText(rect().adjusted(0, 0, -10, 0), Qt::AlignRight | Qt::AlignVCenter,
                   m_recording ? QStringLiteral("● REC") : QStringLiteral("512 CH"));
    }

private:
    int m_universe = 0;
    bool m_recording = false;
};

// ================= GridCanvas：网格绘制区 =================

/**
 * @brief 通道网格画布：把当前 Universe 的 512 通道铺满整个可视区域
 */
class GridCanvas : public QWidget
{
public:
    explicit GridCanvas(DmxGridView *owner)
        : QWidget(owner)
        , m_owner(owner)
    {
    }

protected:
    void paintEvent(QPaintEvent * /*event*/) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);

        p.fillRect(rect(), QColor(0x14, 0x1A, 0x0D));   // 墨绿黑 · 品牌深色底

        const int universe = m_owner->currentUniverse();
        const QByteArray dmx = m_owner->m_universeData.value(universe);

        constexpr int kCols = 32;
        constexpr int kRows = 16;
        const qreal cellW = width() / qreal(kCols);
        const qreal cellH = height() / qreal(kRows);
        if (cellW < 2.0 || cellH < 2.0) {
            return;
        }

        const qreal smallDim = qMin(cellW, cellH);
        const qreal gap = qBound(1.0, smallDim * 0.10, 4.0);
        const qreal radius = qMin(3.5, (smallDim - gap * 2.0) * 0.25);

        QFont valueFont = p.font();
        valueFont.setBold(true);
        valueFont.setPixelSize(qBound(8, int(smallDim * 0.40), 15));

        for (int ch = 0; ch < 512; ++ch) {
            const int cx = ch % kCols;
            const int cy = ch / kCols;          // 行 = 通道 / 列数（注意是列数，不是行数）
            const QRectF cellRect(cx * cellW + gap, cy * cellH + gap,
                                  cellW - gap * 2.0, cellH - gap * 2.0);
            const int v = (ch < dmx.size()) ? static_cast<unsigned char>(dmx.at(ch)) : 0;

            p.setPen(Qt::NoPen);
            p.setBrush(valueColor(v));
            p.drawRoundedRect(cellRect, radius, radius);

            // 通道值（亮格用墨色字保证对比度，阈值取品牌色较亮区段），随格子尺寸缩放
            p.setFont(valueFont);
            p.setPen(v >= 192 ? QColor(0x14, 0x1A, 0x0D) : QColor(0xF2, 0xF4, 0xEA));
            p.drawText(cellRect, Qt::AlignCenter, QString::number(v));

            // 通道号（格子足够大时显示在左上角）
            if (smallDim >= 26.0) {
                QFont chanFont = p.font();
                chanFont.setBold(false);
                chanFont.setPixelSize(qBound(7, int(smallDim * 0.22), 10));
                p.setFont(chanFont);
                p.setPen(v >= 192 ? QColor(20, 26, 13, 160) : QColor(242, 244, 234, 150));
                p.drawText(cellRect.adjusted(3, 1, -1, -1),
                           Qt::AlignLeft | Qt::AlignTop, QString::number(ch + 1));
            }
        }
    }

private:
    DmxGridView *m_owner = nullptr;
};

// ================= DmxGridView =================

DmxGridView::DmxGridView(QWidget *parent)
    : QWidget(parent)
    , m_firstUniverse(0)
    , m_endUniverse(64)
    , m_page(0)
    , m_dirty(true)
{
    setStyleSheet(QStringLiteral("DmxGridView { background-color: #141A0D; }"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 顶部 Universe 标题条 + 网格画布铺满剩余空间
    m_caption = new UniverseCaption(this);
    layout->addWidget(m_caption);

    m_canvas = new GridCanvas(this);
    layout->addWidget(m_canvas, 1);

    m_caption->setUniverse(currentUniverse());
}

void DmxGridView::setFirstUniverse(int firstUniverse)
{
    m_firstUniverse = firstUniverse;
    m_page = qBound(0, m_page, pageCount() - 1);
    m_dirty = true;
    if (m_canvas) {
        m_canvas->update();
    }
}

void DmxGridView::setEndUniverse(int endUniverse)
{
    m_endUniverse = endUniverse;
    m_page = qBound(0, m_page, pageCount() - 1);
    m_dirty = true;
    if (m_canvas) {
        m_canvas->update();
    }
}

void DmxGridView::setCurrentUniverse(int universe)
{
    const int page = qBound(0, universe - m_firstUniverse, pageCount() - 1);
    if (page != m_page) {
        m_page = page;
        if (m_canvas) {
            m_canvas->update();
        }
    }
    if (m_caption) {
        m_caption->setUniverse(currentUniverse());
    }
}

/**
 * @brief 设置录制指示（标题条右侧显示 REC）
 */
void DmxGridView::setRecordingActive(bool recording)
{
    if (m_caption) {
        m_caption->setRecording(recording);
    }
}

/**
 * @brief 更新 Art-Net 数据（仅缓存数据并触发重绘，录制缓存惰性重建）
 */
void DmxGridView::updateArtnetData(const QVariantMap &data)
{
    if (!data.contains("universe") || !data.contains("default")) {
        return;
    }
    const int universe = data["universe"].toInt();
    if (universe < m_firstUniverse || universe >= m_endUniverse) {
        return;
    }

    const bool isNewUniverse = !m_universeData.contains(universe);
    m_universeData[universe] = data["default"].toByteArray();
    m_dirty = true;
    if (m_canvas) {
        m_canvas->update();
    }
    if (isNewUniverse) {
        emit activeUniversesChanged();
    }
}

void DmxGridView::clearData()
{
    m_universeData.clear();
    m_dirty = true;
    if (m_canvas) {
        m_canvas->update();
    }
    emit activeUniversesChanged();
}

bool DmxGridView::hasNonZeroDMX() const
{
    for (int universe = m_firstUniverse; universe < m_endUniverse; ++universe) {
        auto it = m_universeData.find(universe);
        if (it != m_universeData.end()) {
            const QByteArray &dmx = it.value();
            const int size = qMin(dmx.size(), 512);
            for (int i = 0; i < size; ++i) {
                if (static_cast<unsigned char>(dmx.at(i)) != 0) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief 获取录制用线性灰度图像（惰性重建：仅在录制抓帧时生成一次）
 * @return 512 × Universe数 的灰度图；范围无效时返回空图像
 */
QImage DmxGridView::getTextureImage()
{
    if (m_dirty) {
        buildRecordingImage();
    }
    return m_recordImage;
}

/**
 * @brief 构建录制图像：行 = Universe，列 = 512 通道（线性原始值，与显示无关）
 */
void DmxGridView::buildRecordingImage() const
{
    const int rows = qMax(0, m_endUniverse - m_firstUniverse);
    if (rows <= 0) {
        m_recordImage = QImage();
        m_dirty = false;
        return;
    }

    QImage image(512, rows, QImage::Format_Grayscale8);
    for (int row = 0; row < rows; ++row) {
        uchar *dst = image.scanLine(row);
        auto it = m_universeData.find(m_firstUniverse + row);
        if (it != m_universeData.end()) {
            const QByteArray &dmx = it.value();
            const int n = qMin(512, dmx.size());
            memcpy(dst, dmx.constData(), size_t(n));
            if (n < 512) {
                memset(dst + n, 0, size_t(512 - n));
            }
        } else {
            memset(dst, 0, 512);
        }
    }

    m_recordImage = image;
    m_dirty = false;
}
