/**
 * @file FaustParamUI.hpp
 * @brief Faust UI 接口实现：遍历 dsp->buildUserInterface() 的回调，收集参数描述，
 *        并生成节点内嵌参数面板（照 ISFNode/ISFInterface 的动态控件模式）。
 *
 * ---------------------------------------------------------------------------
 * 设计（Faust UI.h 抽象层，宿主决定画什么）
 * ---------------------------------------------------------------------------
 * - addHorizontalSlider/addVerticalSlider/addNumEntry → FloatDragValueWidget
 * - addButton/addCheckButton                          → QCheckBox
 * - addHorizontalBargraph/addVerticalBargraph          → QLabel（只读电平，第一版）
 * - openHorizontalBox/openVerticalBox/openTabBox       → QGroupBox（记录层级）
 * - declare(zone,key,val)                              → 元数据（unit / style 等）
 *
 * 关键：本类既是 faust::UI（遍历参数），也是 QWidget（画控件）。
 * zone 指针是 DSP 实例内部变量的活指针，控件改值直接写 *zone；
 * bargraph 反向：DSP 写 zone，第一版用 QTimer 轮询显示（保留接口）。
 */

#pragma once

#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>

#include <cmath>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>

#include "faust/gui/UI.h"

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace Nodes
{

/** 一个 Faust 参数（UI.h 回调收集，含 zone 活指针） */
struct FaustParamDesc {
    enum class Kind { Slider, Button, Check, NumEntry, Bargraph } kind = Kind::Slider;
    QString label;
    FAUSTFLOAT* zone = nullptr;   // DSP 内部变量活指针（非拥有）
    FAUSTFLOAT init = 0.0f;
    FAUSTFLOAT min = 0.0f;
    FAUSTFLOAT max = 1.0f;
    FAUSTFLOAT step = 0.01f;
    int decimals = 3;
    QString unit;
    QString path;                  // box 层级路径（"Group/Sub"），供分组显示
    bool isPassive = false;        // bargraph：DSP 写，UI 只读
};

/**
 * FaustParamUI : QWidget + faust::UI
 *
 * 用法：
 *   FaustParamUI* ui = new FaustParamUI(parent);
 *   dsp->buildUserInterface(ui);          // 回调填充 params_，直接建控件
 *   ui->rebuild();                        // 若需在回调后统一刷新显示
 */
class FaustParamUI : public QWidget, public UI
{
    Q_OBJECT
public:
    explicit FaustParamUI(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        root_ = new QVBoxLayout(this);
        root_->setContentsMargins(4, 2, 4, 4);
        root_->setSpacing(4);

        formHost_ = new QWidget(this);
        form_ = new QFormLayout(formHost_);
        form_->setContentsMargins(0, 2, 0, 0);
        form_->setSpacing(4);
        form_->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        root_->addWidget(formHost_);
        root_->addStretch(1);

        setMinimumWidth(220);

        // bargraph 轮询：DSP 写 zone，UI 定时刷新读数（第一版低刷 10Hz 足够）
        pollTimer_ = new QTimer(this);
        pollTimer_->setInterval(100);
        connect(pollTimer_, &QTimer::timeout, this, [this]() { refreshBargraphs(); });
    }

    ~FaustParamUI() override
    {
        clearAll();
    }

    // ---- faust::UI 纯虚实现：布局（第一版只记录路径前缀，用扁平表单） ----
    void openTabBox(const char* label) override { openBox(label); }
    void openHorizontalBox(const char* label) override { openBox(label); }
    void openVerticalBox(const char* label) override { openBox(label); }
    void closeBox() override
    {
        if (!boxStack_.isEmpty()) boxStack_.removeLast();
    }

    // ---- 主动控件 ----
    void addButton(const char* label, FAUSTFLOAT* zone) override
    {
        addControl(label, zone, FaustParamDesc::Kind::Button, 0.f, 0.f, 1.f, 1.f);
    }
    void addCheckButton(const char* label, FAUSTFLOAT* zone) override
    {
        addControl(label, zone, FaustParamDesc::Kind::Check, 0.f, 0.f, 1.f, 1.f);
    }
    void addHorizontalSlider(const char* label, FAUSTFLOAT* zone,
                             FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max,
                             FAUSTFLOAT step) override
    {
        addControl(label, zone, FaustParamDesc::Kind::Slider, init, min, max, step);
    }
    void addVerticalSlider(const char* label, FAUSTFLOAT* zone,
                           FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max,
                           FAUSTFLOAT step) override
    {
        addControl(label, zone, FaustParamDesc::Kind::Slider, init, min, max, step);
    }
    void addNumEntry(const char* label, FAUSTFLOAT* zone,
                     FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max,
                     FAUSTFLOAT step) override
    {
        addControl(label, zone, FaustParamDesc::Kind::NumEntry, init, min, max, step);
    }

    // ---- 被动控件（仪表，DSP 写 zone） ----
    void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone,
                               FAUSTFLOAT min, FAUSTFLOAT max) override
    {
        addControl(label, zone, FaustParamDesc::Kind::Bargraph, 0.f, min, max, 0.f);
    }
    void addVerticalBargraph(const char* label, FAUSTFLOAT* zone,
                             FAUSTFLOAT min, FAUSTFLOAT max) override
    {
        addControl(label, zone, FaustParamDesc::Kind::Bargraph, 0.f, min, max, 0.f);
    }

    // ---- 元数据（[unit:dB] / [style:knob] 等） ----
    void declare(FAUSTFLOAT* zone, const char* key, const char* val) override
    {
        const QString k = QString::fromUtf8(key ? key : "");
        const QString v = QString::fromUtf8(val ? val : "");
        for (FaustParamDesc& p : params_) {
            if (p.zone != zone) continue;
            if (k == QStringLiteral("unit")) p.unit = v;
            // style:knob 等第一版忽略（始终 FloatDragValueWidget）
        }
        if (!zone && k == QStringLiteral("name")) {
            moduleName_ = v;
        }
    }

    void addSoundfile(const char*, const char*, Soundfile**) override {}

    // ---- 控件管理 ----
    const QVector<FaustParamDesc>& params() const { return params_; }
    int paramCount() const { return params_.size(); }

    /** 清空参数描述与控件（重新加载 .dsp 前调用） */
    void clearAll()
    {
        if (pollTimer_) pollTimer_->stop();
        while (form_ && form_->rowCount() > 0) form_->removeRow(0);
        params_.clear();
        editors_.clear();
        boxStack_.clear();
    }

    /** 按已收集的描述重建控件（buildUserInterface 回调后调用） */
    void rebuild()
    {
        while (form_ && form_->rowCount() > 0) form_->removeRow(0);
        editors_.clear();

        for (int i = 0; i < params_.size(); ++i) {
            const FaustParamDesc& p = params_[i];
            QWidget* editor = nullptr;

            if (p.kind == FaustParamDesc::Kind::Button ||
                p.kind == FaustParamDesc::Kind::Check) {
                auto* cb = new QCheckBox(formHost_);
                cb->setChecked(p.init > 0.5f);
                cb->setObjectName(QStringLiteral("faustParam%1").arg(i));
                connect(cb, &QCheckBox::toggled, this, [this, i](bool checked) {
                    Q_EMIT parameterChanged(i, checked ? 1.0 : 0.0);
                });
                editor = cb;
            } else if (p.kind == FaustParamDesc::Kind::Bargraph) {
                auto* lbl = new QLabel(formHost_);
                lbl->setObjectName(QStringLiteral("faustParam%1").arg(i));
                lbl->setMinimumWidth(60);
                editor = lbl;
            } else {
                auto* w = new FloatDragValueWidget(formHost_);
                w->setObjectName(QStringLiteral("faustParam%1").arg(i));
                w->setRange(p.min, p.max);
                w->setSingleStep(p.step > 0.f ? p.step : 0.01f);
                w->setDecimals(p.decimals);
                if (!p.unit.isEmpty()) w->setSuffix(QStringLiteral(" ") + p.unit);
                w->setValue(p.init);
                connect(w, &FloatDragValueWidget::valueChanged, this,
                        [this, i](double v) { Q_EMIT parameterChanged(i, v); });
                editor = w;
            }

            form_->addRow(displayLabel(p), editor);
            editors_.insert(i, editor);
        }

        form_->activate();
        root_->activate();
        updateGeometry();
        const QSize hint = sizeHint().expandedTo(minimumSizeHint());
        resize(qMax(hint.width(), minimumWidth()), hint.height());

        if (!params_.isEmpty()) pollTimer_->start();
    }

    /** 外部灌值（存盘恢复 / OSC），不触发 parameterChanged */
    void setParameterValue(int index, double value)
    {
        if (index < 0 || index >= params_.size()) return;
        QWidget* editor = editors_.value(index, nullptr);
        if (!editor) return;
        QSignalBlocker blocker(editor);
        const FaustParamDesc& p = params_[index];
        if (p.kind == FaustParamDesc::Kind::Button ||
            p.kind == FaustParamDesc::Kind::Check) {
            if (auto* cb = qobject_cast<QCheckBox*>(editor))
                cb->setChecked(value > 0.5);
        } else if (p.kind == FaustParamDesc::Kind::Bargraph) {
            // 被动控件由轮询刷新，不灌值
        } else if (auto* w = qobject_cast<FloatDragValueWidget*>(editor)) {
            w->setValue(value);
        }
    }

    /** 刷新 bargraph 读数（10Hz 轮询） */
    void refreshBargraphs()
    {
        for (int i = 0; i < params_.size(); ++i) {
            const FaustParamDesc& p = params_[i];
            if (p.kind != FaustParamDesc::Kind::Bargraph || !p.zone) continue;
            if (auto* lbl = qobject_cast<QLabel*>(editors_.value(i, nullptr))) {
                lbl->setText(QString::number(static_cast<double>(*p.zone), 'f', 2));
            }
        }
    }

    QString moduleName() const { return moduleName_; }

signals:
    /** 用户改参：DataModel 收到后写 *zone = value */
    void parameterChanged(int index, double value);

private:
    void openBox(const char* label)
    {
        const QString seg = QString::fromUtf8(label ? label : "");
        if (seg.isEmpty()) return;
        if (boxStack_.isEmpty()) boxStack_ << seg;
        else boxStack_.last() += QStringLiteral("/") + seg;
    }

    void addControl(const char* label, FAUSTFLOAT* zone,
                    FaustParamDesc::Kind kind, FAUSTFLOAT init,
                    FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
    {
        FaustParamDesc p;
        p.kind = kind;
        p.label = QString::fromUtf8(label ? label : "param");
        p.zone = zone;
        p.init = init;
        p.min = min;
        p.max = max;
        p.step = step;
        p.path = boxStack_.isEmpty() ? QString() : boxStack_.last();
        p.isPassive = (kind == FaustParamDesc::Kind::Bargraph);
        // step 推小数位（0.01→2，0.001→3，1→0）
        if (step > 0.f && step < 1.f) {
            p.decimals = qBound(0, static_cast<int>(-std::log10(step)) + 1, 6);
        } else {
            p.decimals = 0;
        }
        params_.push_back(p);
    }

    QString displayLabel(const FaustParamDesc& p) const
    {
        // Faust 生成的 buildUserInterface() 总是把 DSP 名字（JIT 时即传给
        // createDSPFactoryFromString 的 name，这里是 "FaustNode"）当作最外层
        // box，显示时剥掉这一层根前缀，避免所有控件都带 "FaustNode/"。
        // 不依赖 moduleName_（UI::declare 的 "name" 回调未必触发），直接按
        // 路径第一段剥；用户在 .dsp 里自己写的 open*Box 分组照常显示。
        QString path = p.path;
        const int slash = path.indexOf(QLatin1Char('/'));
        path = (slash < 0) ? QString() : path.mid(slash + 1);
        return path.isEmpty() ? p.label : path + QStringLiteral("/") + p.label;
    }

    QVBoxLayout* root_ = nullptr;
    QWidget* formHost_ = nullptr;
    QFormLayout* form_ = nullptr;
    QTimer* pollTimer_ = nullptr;
    QVector<FaustParamDesc> params_;
    QHash<int, QWidget*> editors_;
    QStringList boxStack_;
    QString moduleName_;
};

} // namespace Nodes
