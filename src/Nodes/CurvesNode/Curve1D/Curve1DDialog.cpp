#include "Curve1DDialog.hpp"

#include "Curve1DDataModel.hpp"
#include "CurvePath.hpp"
#include "Curve1DCore.hpp"
#include "Curve1DEditor.hpp"
#include "FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "IntDragValueWidget/IntDragValueWidget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace Nodes
{
    Curve1DDialog::Curve1DDialog(Curve1DDataModel* model)
        : QDialog(nullptr)
        , m_model(model)
    {
        // 独立顶层窗口（与 Curve 节点的编辑器一致：parent=nullptr + Qt::Window）。
        // 若以嵌入面板为父，QDialog 会贴着节点弹出并随主窗口联动，视觉上像内嵌在节点图里。
        setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
        setWindowTitle(QStringLiteral("一维曲线编辑器 - Curve1D"));
        setAttribute(Qt::WA_DeleteOnClose, false);
        setAttribute(Qt::WA_QuitOnClose, false);
        resize(760, 500);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(4);

        auto* tools = new QHBoxLayout();
        m_dialogPlayButton = new QPushButton(QStringLiteral("启动"), this);
        m_dialogPlayButton->setCheckable(true);
        m_dialogPlayButton->setChecked(m_model->play());
        if (m_model->play()) {
            m_dialogPlayButton->setText(QStringLiteral("停止"));
        }
        auto* addCurveButton = new QPushButton(QStringLiteral("+曲线"), this);
        addCurveButton->setToolTip(QStringLiteral("新增一条曲线（输出端口 +1）"));
        auto* removeCurveButton = new QPushButton(QStringLiteral("-曲线"), this);
        removeCurveButton->setToolTip(QStringLiteral("删除当前选中的曲线（至少保留一条）"));
        auto* addButton = new QPushButton(QStringLiteral("添加点"), this);
        auto* removeButton = new QPushButton(QStringLiteral("删除点"), this);
        auto* frameButton = new QPushButton(QStringLiteral("适应视图"), this);
        auto* periodLabel = new QLabel(QStringLiteral("周期"), this);
        m_periodSpin = new FloatDragValueWidget(this);
        m_periodSpin->setRange(0.1, 3600.0);
        m_periodSpin->setDecimals(2);
        m_periodSpin->setSingleStep(0.1);
        m_periodSpin->setSuffix(QStringLiteral(" s"));
        m_periodSpin->setValue(m_model->period());
        auto* rateLabel = new QLabel(QStringLiteral("采样率"), this);
        m_rateSpin = new IntDragValueWidget(this);
        m_rateSpin->setRange(1, 100);
        m_rateSpin->setSuffix(QStringLiteral(" Hz"));
        m_rateSpin->setValue(m_model->sampleRate());
        tools->addWidget(m_dialogPlayButton);
        tools->addWidget(addCurveButton);
        tools->addWidget(removeCurveButton);
        tools->addWidget(addButton);
        tools->addWidget(removeButton);
        tools->addWidget(frameButton);
        tools->addStretch();
        tools->addWidget(periodLabel);
        tools->addWidget(m_periodSpin);
        tools->addWidget(rateLabel);
        tools->addWidget(m_rateSpin);
        layout->addLayout(tools);

        // 进度条：横跨弹窗，可拖拽定位播放头
        m_dialogSlider = new QSlider(Qt::Horizontal, this);
        m_dialogSlider->setRange(0, 10000);
        connect(m_dialogSlider, &QSlider::valueChanged, this, [this](int v) {
            m_model->setProgress(v / 10000.0);   // slider int 0~10000 归一化为 0.0~1.0
        });
        layout->addWidget(m_dialogSlider);

        // 点编辑行（紧凑单行，置于视口上方）：当前选中点的时间 / 数值
        auto* pointRow = new QHBoxLayout();
        pointRow->setSpacing(4);
        m_pointLabel = new QLabel(this);
        m_pointLabel->setFixedHeight(22);
        auto makeValueEditor = [this](const QString& suffix, double minValue, double maxValue) {
            auto* editor = new FloatDragValueWidget(this);
            editor->setRange(minValue, maxValue);
            editor->setDecimals(3);
            editor->setSingleStep(0.01);
            editor->setSuffix(suffix);
            return editor;
        };
        m_x = makeValueEditor(QStringLiteral("时间"), 0.0, m_model->period());
        m_y = makeValueEditor(QStringLiteral("数值"), -100000.0, 100000.0);
        m_x->setFixedHeight(22);
        m_y->setFixedHeight(22);
        pointRow->addWidget(m_pointLabel);
        pointRow->addWidget(m_x, 1);
        pointRow->addWidget(m_y, 1);
        layout->addLayout(pointRow);

        // 函数图编辑视口（X=完整周期时间刻度，Y=输出值；左上角图例点击切换编辑目标）
        m_editViewport = new Curve1DEditor(this);
        m_editViewport->setPeriod(m_model->period());
        m_editViewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_editViewport->setMinimumHeight(260);
        layout->addWidget(m_editViewport, 1);

        // 编辑器内点击曲线/图例切换选中 → 同步编辑目标
        connect(m_editViewport, &Curve1DEditor::activeIndexChanged,
                m_model, &Curve1DDataModel::setActiveCurve);
        connect(addCurveButton, &QPushButton::clicked, m_model, &Curve1DDataModel::addCurve);
        connect(removeCurveButton, &QPushButton::clicked, m_model, [this] {
            m_model->removeCurve(m_model->activeCurve());
        });

        connect(addButton, &QPushButton::clicked, m_model, [this] {
            CurvePath* path = m_model->activePath();
            if (!path) {
                return;
            }
            const QList<QVector3D> pts = path->points();
            if (pts.isEmpty()) {
                path->addPoint(QVector3D(0.0f, 0.5f, 0.0f));
                return;
            }
            if (pts.size() == 1) {
                // 仅一点：在对侧补第二点构成全跨度
                const float y = pts.first().y();
                path->addPoint(pts.first().x() < 0.5f ? QVector3D(1.0f, y, 0.0f)
                                                      : QVector3D(0.0f, y, 0.0f));
                return;
            }
            // 在最大 X 间隙的中点插入关键点（值取曲线该处的当前输出）
            int gapIndex = 1;
            float gap = -1.0f;
            for (int i = 1; i < pts.size(); ++i) {
                const float width = pts.at(i).x() - pts.at(i - 1).x();
                if (width > gap) {
                    gap = width;
                    gapIndex = i;
                }
            }
            const float midX = (pts.at(gapIndex - 1).x() + pts.at(gapIndex).x()) * 0.5f;
            path->insertPoint(gapIndex,
                              QVector3D(midX, float(curve1DValueAt(path, midX)), 0.0f));
        });
        connect(removeButton, &QPushButton::clicked, m_model, [this] {
            CurvePath* path = m_model->activePath();
            if (path) {
                path->removePoint(path->selectedIndex());
            }
        });
        connect(frameButton, &QPushButton::clicked, this, &Curve1DDialog::fitView);
        connect(m_periodSpin, &FloatDragValueWidget::valueChanged,
                m_model, &Curve1DDataModel::setPeriod);
        connect(m_dialogPlayButton, &QPushButton::toggled, m_model, &Curve1DDataModel::setPlay);
        connect(m_rateSpin, &IntDragValueWidget::valueChanged,
                m_model, &Curve1DDataModel::setSampleRate);

        // 点编辑：数据层 X=进度 0~1（时间编辑值 / 周期 反算），Y=输出值
        const auto editAxis = [this] {
            CurvePath* path = m_model->activePath();
            const int index = path ? path->selectedIndex() : -1;
            if (index < 0 || !m_x || !m_y) {
                return;
            }
            const QList<QVector3D>& pts = path->points();
            float x = float(qBound(0.0, m_x->value() / std::max(0.1, m_model->period()), 1.0));
            const float eps = 0.001f;
            if (index > 0) {
                x = std::max(x, pts.at(index - 1).x() + eps);
            }
            if (index < pts.size() - 1) {
                x = std::min(x, pts.at(index + 1).x() - eps);
            }
            x = qBound(0.0f, x, 1.0f);
            path->setPoint(index, QVector3D(x, float(m_y->value()), 0.0f));
        };
        connect(m_x, &FloatDragValueWidget::valueChanged, m_model, editAxis);
        connect(m_y, &FloatDragValueWidget::valueChanged, m_model, editAxis);

        // 模型播放态变化 → 工具行启动按钮（面板按钮的镜像，OSC/连线/同步源均经 playChanged）
        connect(m_model, &Curve1DDataModel::playChanged, this, [this](bool playing) {
            const QSignalBlocker blocker(m_dialogPlayButton);
            m_dialogPlayButton->setChecked(playing);
            m_dialogPlayButton->setText(playing ? QStringLiteral("停止") : QStringLiteral("启动"));
        });

        syncCurves();
        fitView();
        syncPointEditors();
    }

    void Curve1DDialog::syncCurves()
    {
        if (!m_editViewport) {
            return;
        }
        m_editViewport->setPaths(m_model->paths());
        m_editViewport->setActiveIndex(m_model->activeCurve());
    }

    void Curve1DDialog::syncPointEditors()
    {
        if (!m_x || !m_y) {
            return;
        }
        CurvePath* path = m_model->activePath();
        const int index = path ? path->selectedIndex() : -1;
        const bool enabled = index >= 0 && path && index < path->points().size();
        m_x->setEnabled(enabled);
        m_y->setEnabled(enabled);
        if (m_pointLabel) {
            m_pointLabel->setText(enabled
                ? QStringLiteral("点 %1").arg(index + 1)
                : QStringLiteral("未选择"));
        }
        if (!enabled) {
            return;
        }
        const QVector3D point = path->points().at(index);
        const QSignalBlocker blockX(m_x);
        const QSignalBlocker blockY(m_y);
        m_x->setValue(double(point.x()) * m_model->period());   // 显示为周期内时间（秒）
        m_y->setValue(point.y());
    }

    void Curve1DDialog::setProgressView(double progress)
    {
        if (!m_dialogSlider) {
            return;
        }
        const QSignalBlocker blocker(m_dialogSlider);
        m_dialogSlider->setValue(int(std::round(progress * 10000.0)));
    }

    void Curve1DDialog::setPeriodView(double period)
    {
        // 编辑器 X 轴时间刻度与点编辑行的时间范围随周期刷新
        if (m_editViewport) {
            m_editViewport->setPeriod(period);
        }
        if (m_x) {
            m_x->setRange(0.0, period);
        }
        if (m_periodSpin) {
            const QSignalBlocker blocker(m_periodSpin);
            m_periodSpin->setValue(period);
        }
    }

    void Curve1DDialog::setRateView(int rate)
    {
        if (!m_rateSpin) {
            return;
        }
        const QSignalBlocker blocker(m_rateSpin);
        m_rateSpin->setValue(rate);
    }

    void Curve1DDialog::setPlaybackFraction(double fraction)
    {
        if (m_editViewport) {
            m_editViewport->setPlaybackFraction(fraction);
        }
    }

    void Curve1DDialog::fitView()
    {
        if (m_editViewport) {
            m_editViewport->fitY();
        }
    }
}
