#pragma once

#include <QCheckBox>
#include <QGridLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QWidget>

#include <algorithm>
#include <cmath>

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace Nodes
{
class TrackPickView final : public QWidget
{
    Q_OBJECT
public:
    explicit TrackPickView(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(320, 180);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMouseTracking(true);
        setCursor(Qt::CrossCursor);
    }

    void setPickingEnabled(bool enabled)
    {
        m_picking = enabled;
        setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
        update();
    }

    const QImage& image() const { return m_image; }
    bool isFrozen() const { return m_frozen; }
    void setFrozen(bool frozen) { m_frozen = frozen; }

    void setImage(const QImage& image)
    {
        if (m_frozen || image.isNull()) {
            return;
        }
        m_image = image;
        m_overlay = {};
        update();
    }

    void setOverlay(const QImage& overlay)
    {
        m_overlay = overlay;
        update();
    }

signals:
    void pointClicked(QPoint point);

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(20, 20, 20));
        const QRectF dest = imageDest();
        if (m_image.isNull() || dest.isEmpty()) {
            painter.setPen(QColor(180, 180, 180));
            const QString hint = m_picking ? QStringLiteral("等待画面，点击选择物体")
                                           : QStringLiteral("检测中");
            painter.drawText(rect(), Qt::AlignCenter, hint);
            return;
        }
        painter.drawImage(dest, m_image);
        if (!m_overlay.isNull()) {
            painter.drawImage(dest, m_overlay);
        }
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (!m_picking || m_image.isNull() || event->button() != Qt::LeftButton) {
            return;
        }
        const QPoint imagePoint = widgetPointToImage(event->pos());
        if (imagePoint.x() >= 0) {
            emit pointClicked(imagePoint);
        }
    }

private:
    QRectF imageDest() const
    {
        if (m_image.isNull() || width() <= 0 || height() <= 0) {
            return {};
        }
        const QSize fitted = m_image.size().scaled(size(), Qt::KeepAspectRatio);
        return QRectF((width() - fitted.width()) / 2.0,
                      (height() - fitted.height()) / 2.0,
                      fitted.width(),
                      fitted.height());
    }

    QPoint widgetPointToImage(const QPoint& widgetPoint) const
    {
        const QRectF dest = imageDest();
        if (m_image.isNull() || dest.width() < 1.0 || dest.height() < 1.0 || !dest.contains(widgetPoint)) {
            return QPoint(-1, -1);
        }
        const double scaleX = m_image.width() / dest.width();
        const double scaleY = m_image.height() / dest.height();
        const int x = std::clamp(static_cast<int>(std::floor((widgetPoint.x() - dest.left()) * scaleX)), 0, m_image.width() - 1);
        const int y = std::clamp(static_cast<int>(std::floor((widgetPoint.y() - dest.top()) * scaleY)), 0, m_image.height() - 1);
        return QPoint(x, y);
    }

    QImage m_image;
    QImage m_overlay;
    bool m_frozen = false;
    bool m_picking = true;
};

class ObjectTrackInterface final : public QWidget
{
    Q_OBJECT
public:
    explicit ObjectTrackInterface(QWidget* parent = nullptr)
    {
        auto* layout = new QGridLayout(this);
        layout->setContentsMargins(4, 2, 4, 4);
        layout->setSpacing(6);

        regionView = new TrackPickView(this);
        layout->addWidget(regionView, 0, 0, 1, 2);
        layout->addWidget(new QLabel(QStringLiteral("先点击选择对象。选择完成后再开始检测，检测过程中不能再选。"), this), 1, 0, 1, 2);

        idEdit = new QLineEdit(this);
        idEdit->setPlaceholderText(QStringLiteral("输入 ID，空则自动编号"));
        layout->addWidget(new QLabel(QStringLiteral("目标 ID:"), this), 2, 0);
        layout->addWidget(idEdit, 2, 1);

        removeBtn = new QPushButton(QStringLiteral("删除选中"), this);
        layout->addWidget(removeBtn, 3, 0, 1, 2);

        targetList = new QListWidget(this);
        targetList->setMinimumHeight(72);
        targetList->setIconSize(QSize(72, 40));
        layout->addWidget(targetList, 4, 0, 1, 2);

        confidence = new FloatDragValueWidget(this);
        confidence->setRange(0, 1);
        confidence->setSingleStep(0.01);
        confidence->setDecimals(2);
        confidence->setValue(0.25);
        layout->addWidget(new QLabel(QStringLiteral("相似度阈值:"), this), 5, 0);
        layout->addWidget(confidence, 5, 1);

        maxFps = new FloatDragValueWidget(this);
        maxFps->setRange(1, 30);
        maxFps->setSingleStep(1);
        maxFps->setDecimals(0);
        maxFps->setValue(15);
        layout->addWidget(new QLabel(QStringLiteral("最大帧率 (FPS):"), this), 6, 0);
        layout->addWidget(maxFps, 6, 1);

        holdFrames = new FloatDragValueWidget(this);
        holdFrames->setRange(0, 60);
        holdFrames->setSingleStep(1);
        holdFrames->setDecimals(0);
        holdFrames->setValue(8);
        layout->addWidget(new QLabel(QStringLiteral("丢失保持 (帧):"), this), 7, 0);
        layout->addWidget(holdFrames, 7, 1);

        drawOverlay = new QCheckBox(QStringLiteral("绘制跟踪框"), this);
        drawOverlay->setChecked(true);
        layout->addWidget(drawOverlay, 8, 0, 1, 2);

        layout->addWidget(new QLabel(QStringLiteral("开始检测:"), this), 9, 0);
        enableBtn = new QPushButton(QStringLiteral("开始检测"), this);
        enableBtn->setCheckable(true);
        layout->addWidget(enableBtn, 9, 1);

        statusLabel = new QLabel(QStringLiteral("点击画面选择对象"), this);
        statusLabel->setWordWrap(true);
        layout->addWidget(statusLabel, 10, 0, 1, 2);

        layout->setRowStretch(0, 1);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 2);
    }

    TrackPickView* regionView = nullptr;
    QLineEdit* idEdit = nullptr;
    QPushButton* removeBtn = nullptr;
    QListWidget* targetList = nullptr;
    FloatDragValueWidget* confidence = nullptr;
    FloatDragValueWidget* maxFps = nullptr;
    FloatDragValueWidget* holdFrames = nullptr;
    QCheckBox* drawOverlay = nullptr;
    QPushButton* enableBtn = nullptr;
    QLabel* statusLabel = nullptr;
};
} // namespace Nodes
