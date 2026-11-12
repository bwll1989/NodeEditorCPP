#ifndef DMXGRIDVIEW_H
#define DMXGRIDVIEW_H

#include <QWidget>
#include <QImage>
#include <QVariantMap>

class GridCanvas;
class UniverseCaption;

/**
 * @brief DMX 通道网格预览控件（LXMonitor 风格 · Flow 品牌视觉）
 *
 * 每次显示一个 Universe 的 512 个通道（32 列 × 16 行），网格按控件尺寸铺满：
 *  - 画布底色为墨绿黑 #141A0D；每个通道一个圆角单元格，颜色随 DMX 值变化
 *    （Flow 品牌手册色板）：暗绿黑(0) → 信息蓝 #3F6E8E → ArtNet 青 #4FB8C9(255)
 *  - 单元格中央显示通道值，格子足够大时左上角显示通道号
 *  - 顶部 Universe 标题条（等宽字体 + ArtNet 领域色圆点）显示当前域与录制状态
 *  - 当前显示的 Universe 由外部通过 setCurrentUniverse() 控制
 *   （选择 UI 位于主窗口左侧面板，本控件只负责绘制）
 *
 * 本控件只负责预览显示；录制使用的线性灰度图像由 buildRecordingImage()
 * 惰性生成（512 × Universe数，行=Universe，列=通道），与显示渲染完全解耦，
 * 录制数据不受任何显示样式影响。
 */
class DmxGridView : public QWidget
{
    Q_OBJECT

public:
    explicit DmxGridView(QWidget *parent = nullptr);

    /**
     * @brief 设置起始 Universe
     */
    void setFirstUniverse(int firstUniverse);

    /**
     * @brief 设置结束 Universe
     */
    void setEndUniverse(int endUniverse);

    /**
     * @brief 设置当前预览显示的 Universe（超出范围时自动夹取）
     */
    void setCurrentUniverse(int universe);

    /**
     * @brief 当前预览显示的 Universe
     */
    int currentUniverse() const { return m_firstUniverse + qBound(0, m_page, pageCount() - 1); }

    /**
     * @brief 设置录制指示（标题条右侧显示 REC）
     */
    void setRecordingActive(bool recording);

    /**
     * @brief 实际收到过数据的 Universe 列表（升序，仅当前录制范围内）
     */
    QList<int> activeUniverses() const
    {
        QList<int> result;
        for (auto it = m_universeData.constBegin(); it != m_universeData.constEnd(); ++it) {
            if (it.key() >= m_firstUniverse && it.key() < m_endUniverse) {
                result.append(it.key());
            }
        }
        return result;
    }

    /**
     * @brief 录制图像尺寸（与原 OpenGL 控件接口保持一致）
     * @return 宽度固定 512（列 = 通道）
     */
    int getTextureWidth() const { return 512; }

    /**
     * @brief 录制图像高度（行数 = Universe 数）
     */
    int getTextureHeight() const { return qMax(0, m_endUniverse - m_firstUniverse); }

    /**
     * @brief 获取录制用线性灰度图像（数据变化时惰性重建）
     */
    QImage getTextureImage();

public slots:
    /**
     * @brief 更新 Art-Net 数据
     * @param data 包含 universe / default(DMX数据) 的映射
     */
    void updateArtnetData(const QVariantMap &data);

    /**
     * @brief 清除所有数据
     */
    void clearData();

    /**
     * @brief 当前显示范围内是否存在非零 DMX 值
     */
    bool hasNonZeroDMX() const;

signals:
    /**
     * @brief 发现新 Universe（首次收到某个 Universe 的数据）时发出
     */
    void activeUniversesChanged();

private:
    friend class GridCanvas;
    int pageCount() const { return qMax(1, m_endUniverse - m_firstUniverse); }
    void buildRecordingImage() const;

    QMap<int, QByteArray> m_universeData;   // Universe -> 512 字节 DMX 数据
    int m_firstUniverse;                    // 起始 Universe
    int m_endUniverse;                      // 结束 Universe（不含）
    int m_page;                             // 当前预览页（相对起始 Universe 的偏移）
    mutable QImage m_recordImage;           // 录制用线性灰度缓存
    mutable bool m_dirty;                   // 录制缓存是否需要重建

    GridCanvas *m_canvas = nullptr;         // 网格绘制区
    UniverseCaption *m_caption = nullptr;   // 顶部 Universe 标题条
};

#endif // DMXGRIDVIEW_H
