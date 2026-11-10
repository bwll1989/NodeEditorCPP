#include "Curve2DDialog.hpp"

#include "Curve2DDataModel.hpp"
#include "CurvePath.hpp"
#include "Curve2DEditor.hpp"
#include "FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "IntDragValueWidget/IntDragValueWidget.hpp"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QVBoxLayout>

namespace Nodes
{
    Curve2DDialog::Curve2DDialog(Curve2DDataModel* model)
        : QDialog(nullptr)
        , m_model(model)
    {
        // 独立顶层窗口（与 Curve 节点的编辑器一致：parent=nullptr + Qt::Window）。
        // 若以嵌入面板为父，QDialog 会贴着节点弹出并随主窗口联动，视觉上像内嵌在节点图里。
        setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
        setWindowTitle(QStringLiteral("二维曲线编辑器 - Curve2D"));
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
        m_closedBox = new QCheckBox(QStringLiteral("闭合"), this);
        m_closedBox->setToolTip(QStringLiteral("闭合/开放当前选中的曲线"));
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
        tools->addWidget(m_closedBox);
        tools->addWidget(addButton);
        tools->addWidget(removeButton);
        tools->addWidget(frameButton);
        tools->addStretch();
        tools->addWidget(periodLabel);
        tools->addWidget(m_periodSpin);
        tools->addWidget(rateLabel);
        tools->addWidget(m_rateSpin);
        layout->addLayout(tools);

        // 进度条：可拖拽定位播放头，随播放推进
        m_dialogSlider = new QSlider(Qt::Horizontal, this);
        m_dialogSlider->setRange(0, 10000);
        connect(m_dialogSlider, &QSlider::valueChanged, this, [this](int v) {
            m_model->setProgress(v / 10000.0);   // slider int 0~10000 归一化为 0.0~1.0
        });
        layout->addWidget(m_dialogSlider);

        // 点编辑行（紧凑单行，置于视口上方）：当前选中点的 X / Y
        auto* pointRow = new QHBoxLayout();
        pointRow->setSpacing(4);
        m_pointLabel = new QLabel(this);
        m_pointLabel->setFixedHeight(22);
        auto makeAxisEditor = [this](const QString& suffix) {
            auto* editor = new FloatDragValueWidget(this);
            editor->setRange(-10000.0, 10000.0);
            editor->setDecimals(3);
            editor->setSingleStep(0.01);
            editor->setSuffix(suffix);
            return editor;
        };
        m_x = makeAxisEditor(QStringLiteral("X"));
        m_y = makeAxisEditor(QStringLiteral("Y"));
        m_x->setFixedHeight(22);
        m_y->setFixedHeight(22);
        pointRow->addWidget(m_pointLabel);
        pointRow->addWidget(m_x, 1);
        pointRow->addWidget(m_y, 1);
        layout->addLayout(pointRow);

        // 平面编辑视口占满主体（曲线列表为视口内嵌图例，点击行/曲线切换编辑目标）
        m_editViewport = new Curve2DEditor(this);
        m_editViewport->setPaths(m_model->paths());
        m_editViewport->setActiveIndex(m_model->activeCurve());
        m_editViewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_editViewport->setMinimumHeight(260);
        layout->addWidget(m_editViewport, 1);

        // 编辑器内点击图例/曲线切换选中 → 同步编辑目标（同 Curve1D 信号闭环）
        connect(m_editViewport, &Curve2DEditor::activeIndexChanged,
                m_model, &Curve2DDataModel::setActiveCurve);
        connect(addCurveButton, &QPushButton::clicked, m_model, &Curve2DDataModel::addCurve);
        connect(removeCurveButton, &QPushButton::clicked, m_model, [this] {
            m_model->removeCurve(m_model->activeCurve());
        });

        connect(addButton, &QPushButton::clicked, m_model, [this] {
            CurvePath* path = m_model->activePath();
            if (!path) {
                return;
            }
            const QVector3D last = path->points().isEmpty() ? QVector3D() : path->points().last();
            path->addPoint(last + QVector3D(0.5f, 0.5f, 0.0f));
        });
        connect(removeButton, &QPushButton::clicked, m_model, [this] {
            CurvePath* path = m_model->activePath();
            if (path) {
                path->removePoint(path->selectedIndex());
            }
        });
        connect(frameButton, &QPushButton::clicked, this, &Curve2DDialog::fitView);
        connect(m_closedBox, &QCheckBox::toggled, m_model, &Curve2DDataModel::setClosed);
        connect(m_periodSpin, &FloatDragValueWidget::valueChanged,
                m_model, &Curve2DDataModel::setPeriod);
        connect(m_dialogPlayButton, &QPushButton::toggled, m_model, &Curve2DDataModel::setPlay);
        connect(m_rateSpin, &IntDragValueWidget::valueChanged,
                m_model, &Curve2DDataModel::setSampleRate);

        // 平面编辑：只改 x/y，z 保留原值（不参与输出）
        const auto editAxis = [this] {
            CurvePath* path = m_model->activePath();
            const int index = path ? path->selectedIndex() : -1;
            if (index < 0 || !m_x || !m_y) {
                return;
            }
            const float z = path->points().at(index).z();
            path->setPoint(index, QVector3D(float(m_x->value()), float(m_y->value()), z));
        };
        connect(m_x, &FloatDragValueWidget::valueChanged, m_model, editAxis);
        connect(m_y, &FloatDragValueWidget::valueChanged, m_model, editAxis);

        // 模型播放态变化 → 工具行启动按钮（面板按钮的镜像，OSC/连线/同步源均经 playChanged）
        connect(m_model, &Curve2DDataModel::playChanged, this, [this](bool playing) {
            const QSignalBlocker blocker(m_dialogPlayButton);
            m_dialogPlayButton->setChecked(playing);
            m_dialogPlayButton->setText(playing ? QStringLiteral("停止") : QStringLiteral("启动"));
        });

        syncCurves();
        syncClosedBox();
        fitView();
        syncPointEditors();
    }

    void Curve2DDialog::syncCurves()
    {
        if (!m_editViewport) {
            return;
        }
        m_editViewport->setPaths(m_model->paths());
        m_editViewport->setActiveIndex(m_model->activeCurve());
    }

    void Curve2DDialog::syncClosedBox()
    {
        if (!m_closedBox) {
            return;
        }
        CurvePath* path = m_model->activePath();
        const QSignalBlocker blocker(m_closedBox);
        m_closedBox->setChecked(path ? path->isClosed() : false);
    }

    void Curve2DDialog::syncPointEditors()
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
        m_x->setValue(point.x());
        m_y->setValue(point.y());
    }

    void Curve2DDialog::setProgressView(double progress)
    {
        if (!m_dialogSlider) {
            return;
        }
        const QSignalBlocker blocker(m_dialogSlider);
        m_dialogSlider->setValue(int(std::round(progress * 10000.0)));
    }

    void Curve2DDialog::setPeriodView(double period)
    {
        if (m_periodSpin) {
            const QSignalBlocker blocker(m_periodSpin);
            m_periodSpin->setValue(period);
        }
    }

    void Curve2DDialog::setRateView(int rate)
    {
        if (!m_rateSpin) {
            return;
        }
        const QSignalBlocker blocker(m_rateSpin);
        m_rateSpin->setValue(rate);
    }

    void Curve2DDialog::setPlaybackFraction(double fraction)
    {
        if (m_editViewport) {
            m_editViewport->setPlaybackFraction(fraction);
        }
    }

    void Curve2DDialog::fitView()
    {
        if (m_editViewport) {
            m_editViewport->frameAll();
        }
    }

    void Curve2DDialog::detachViewport()
    {
        if (m_editViewport) {
            m_editViewport->setPath(nullptr);
        }
    }
}
