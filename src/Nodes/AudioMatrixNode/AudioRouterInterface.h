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
#include <QVector>

namespace Nodes
{
    /**
     * @brief 竖排文字标签（输出端口名，逆时针旋转 90°）
     */
    class RouterVerticalTextLabel : public QWidget
    {
    public:
        explicit RouterVerticalTextLabel(const QString &text, QWidget *parent = nullptr)
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
     * @brief 二进制路由矩阵：行 = In，列 = Out；每列最多选一路输入
     */
    class RouterMatrixWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit RouterMatrixWidget(int rows, int cols, QWidget *parent = nullptr);

        void resizeMatrix(int rows, int cols);
        int getRows() const { return m_rows; }
        int getCols() const { return m_cols; }

        /** 每个输出对应的输入索引，未连接为 -1 */
        QVector<int> routingMap() const;
        void setRoutingMap(const QVector<int> &map);

    public slots:
        void setIdentity();
        void clearAll();

    signals:
        void routingChanged(QVector<int> map);

    private:
        void rebuildButtons();
        void emitRouting();
        QToolButton *buttonAt(int row, int col) const;

        int m_rows = 0;
        int m_cols = 0;
        QGridLayout *m_layout = nullptr;
        QVector<QToolButton *> m_buttons;
    };

    /**
     * @brief Audio Router 界面（与 Audio Matrix 同布局，交叉点为通/断）
     */
    class AudioRouterInterface : public QWidget
    {
        Q_OBJECT

    public:
        static constexpr int kMinChannels = 1;
        static constexpr int kDefaultChannels = 8;
        static constexpr int kAxisWidth = 40;
        static constexpr int kOutHeaderHeight = 48;
        static constexpr int kGridSpacing = 2;

        explicit AudioRouterInterface(int inputCount = kDefaultChannels,
                                      int outputCount = kDefaultChannels,
                                      QWidget *parent = nullptr);
        ~AudioRouterInterface() override = default;

        RouterMatrixWidget *routerWidget() const { return mRouterWidget; }
        int inputCount() const;
        int outputCount() const;
        void setChannelCounts(int inputCount, int outputCount);

    private:
        void rebuildAxisLabels();
        void updateMinimumSizeForChannels();

        RouterMatrixWidget *mRouterWidget = nullptr;
        QToolButton *mMenuButton = nullptr;
        QWidget *mCornerPanel = nullptr;
        QWidget *mOutHeader = nullptr;
        QWidget *mInHeader = nullptr;
        QHBoxLayout *mOutHeaderLayout = nullptr;
        QVBoxLayout *mInHeaderLayout = nullptr;
    };
}
