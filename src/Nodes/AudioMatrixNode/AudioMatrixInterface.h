#pragma once

#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QPainter>
#include <QPaintEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "Elements/MartixWidget/MatrixWidget.h"

namespace Nodes
{
    /**
     * @brief 竖排文字标签（输出端口名，逆时针旋转 90°）
     */
    class VerticalTextLabel : public QWidget
    {
    public:
        explicit VerticalTextLabel(const QString &text, QWidget *parent = nullptr)
            : QWidget(parent)
            , m_text(text)
        {
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumSize(16, 40);
            setToolTip(text);
        }

        void setText(const QString &text)
        {
            m_text = text;
            setToolTip(text);
            update();
        }

        QSize sizeHint() const override { return QSize(20, 48); }
        QSize minimumSizeHint() const override { return QSize(16, 40); }

    protected:
        void paintEvent(QPaintEvent *) override
        {
            QPainter p(this);
            p.setRenderHint(QPainter::Antialiasing, true);
            const QPalette pal = palette();
            p.fillRect(rect(), pal.color(QPalette::Window));
            p.setPen(pal.color(QPalette::WindowText));
            QFont f = font();
            f.setBold(true);
            p.setFont(f);

            p.translate(width() * 0.5, height() * 0.5);
            p.rotate(-90.0);
            p.drawText(QRectF(-height() * 0.5, -width() * 0.5, height(), width()),
                       Qt::AlignCenter,
                       m_text);
        }

    private:
        QString m_text;
    };

    /**
     * @brief Audio Matrix 路由矩阵界面（经典 patchbay 布局）
     *
     * 行列数由节点端口数驱动；左上角为「⋯」菜单（1:1 / 全部静音）。
     */
    class AudioMatrixInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kMinChannels = 1;
        static constexpr int kDefaultChannels = 8;
        static constexpr int kAxisWidth = 40;
        static constexpr int kOutHeaderHeight = 48;
        static constexpr int kGridSpacing = 2;

        explicit AudioMatrixInterface(int inputCount = kDefaultChannels,
                                      int outputCount = kDefaultChannels,
                                      QWidget *parent = nullptr);
        ~AudioMatrixInterface() override = default;

        MatrixWidget *matrixWidget() const { return mMatrixWidget; }
        int inputCount() const;
        int outputCount() const;

        /** 按端口数同步矩阵尺寸，并刷新轴标签 */
        void setChannelCounts(int inputCount, int outputCount);

    private:
        void rebuildAxisLabels();
        void updateMinimumSizeForChannels();

        MatrixWidget *mMatrixWidget = nullptr;
        QToolButton *mMenuButton = nullptr;
        QWidget *mCornerPanel = nullptr;
        QWidget *mOutHeader = nullptr;
        QWidget *mInHeader = nullptr;
        QHBoxLayout *mOutHeaderLayout = nullptr;
        QVBoxLayout *mInHeaderLayout = nullptr;
    };
}
