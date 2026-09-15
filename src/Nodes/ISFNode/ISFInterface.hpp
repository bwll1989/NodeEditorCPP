/**
 * @file ISFInterface.hpp
 * @brief ISF 节点内嵌 UI：媒体库选 .fs + 按 INPUTS 动态生成参数区
 *
 * 控件映射：
 * - float   → FloatDragValueWidget
 * - long    → QComboBox（有 LABELS/VALUES）或 IntDragValueWidget
 * - bool    → QCheckBox
 * - event   → QCheckBox（渲染后由 DataModel 清回 false，模拟单帧脉冲）
 * - color   → 色块按钮 + 独立顶层 ColorEditorWidget（勿把对话框 parent 到节点上）
 * - point2D → 并排两个 FloatDrag（X/Y），各自使用独立 min/max
 * - image   → 仅显示 "(port)"，需接线
 * - audio   → "(audio → wave)"，接 AudioData，节点内打波形纹理
 * - audioFFT→ "(audio → FFT)"，接 AudioData，节点内做 FFT 后打频谱纹理
 *
 * 面板参数始终可调；上游 VariableData 到达时通过 setParameterValue 同步显示。
 */

#pragma once

#include "ISFRenderer.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantMap>
#include <QWidget>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"

namespace Nodes
{

class ISFInterface : public QWidget
{
    Q_OBJECT
public:
    explicit ISFInterface(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        m_root = new QVBoxLayout(this);
        m_root->setContentsMargins(4, 2, 4, 4);
        m_root->setSpacing(6);

        fileSelectComboBox->setPlaceholderText(QStringLiteral("Select .fs from Media Library"));
        m_root->addWidget(fileSelectComboBox);

        reloadButton->setText(QStringLiteral("Reload"));
        m_root->addWidget(reloadButton);

        m_paramsHost = new QWidget(this);
        m_paramsForm = new QFormLayout(m_paramsHost);
        m_paramsForm->setContentsMargins(0, 4, 0, 0);
        m_paramsForm->setSpacing(4);
        m_paramsForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_root->addWidget(m_paramsHost);

        m_root->addStretch(1);
        setMinimumWidth(220);

        // 颜色编辑器必须是独立顶层窗口：
        // 若 parent 到节点嵌入控件，在节点编辑器里常出现空白 / 无法交互。
        m_colorEditor = new ColorEditorWidget();
        m_colorEditor->setParent(nullptr);
        m_colorEditor->setAlphaEnabled(true);
        m_colorEditor->setAttribute(Qt::WA_DeleteOnClose, false);
        m_colorEditor->setAttribute(Qt::WA_QuitOnClose, false);
        connect(m_colorEditor, &ColorEditorWidget::colorChanged, this, [this](const QColor& c) {
            if (m_editingColorName.isEmpty()) {
                return;
            }
            if (QPushButton* btn = qobject_cast<QPushButton*>(m_editors.value(m_editingColorName))) {
                applyColorButton(btn, c);
            }
            Q_EMIT parameterChanged(
                m_editingColorName,
                QVariantList{c.redF(), c.greenF(), c.blueF(), c.alphaF()});
        });
    }

    ~ISFInterface() override
    {
        if (m_colorEditor) {
            m_colorEditor->setParent(nullptr);
            m_colorEditor->close();
            m_colorEditor->deleteLater();
            m_colorEditor = nullptr;
        }
    }

    /**
     * @brief 按当前 INPUTS 重建参数区
     * @param ports  端口描述（含 image，image 只出提示行）
     * @param values ATTR name → 当前值（缺省用 port.defaultValue）
     */
    void rebuildParameters(const QVector<ISFPortDesc>& ports, const QVariantMap& values)
    {
        clearParameters();

        for (const ISFPortDesc& port : ports) {
            if (!port.isVariable()) {
                if (port.isImage()) {
                    auto* hint = new QLabel(QStringLiteral("(port)"), m_paramsHost);
                    hint->setStyleSheet(QStringLiteral("color: gray;"));
                    m_paramsForm->addRow(port.caption, hint);
                } else if (port.isAudioWave()) {
                    auto* hint = new QLabel(QStringLiteral("(audio → wave)"), m_paramsHost);
                    hint->setStyleSheet(QStringLiteral("color: gray;"));
                    m_paramsForm->addRow(port.caption, hint);
                } else if (port.isAudioFft()) {
                    auto* hint = new QLabel(QStringLiteral("(audio → FFT)"), m_paramsHost);
                    hint->setStyleSheet(QStringLiteral("color: gray;"));
                    m_paramsForm->addRow(port.caption, hint);
                }
                continue;
            }

            QWidget* editor = createEditor(port);
            if (!editor) {
                continue;
            }
            m_editors.insert(port.name, editor);
            m_kinds.insert(port.name, port.kind);
            m_paramsForm->addRow(port.caption, editor);

            const QVariant v = values.value(port.name, port.defaultValue);
            setParameterValue(port.name, v);
        }

        // 按内容收紧尺寸，避免 Resizable 残留的旧 height + stretch 留白/撑破外框
        if (m_paramsForm) {
            m_paramsForm->activate();
        }
        if (m_root) {
            m_root->activate();
        }
        // 清掉拖拽放大时留下的过高 minimum，否则 adjustSize 缩不回去
        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        updateGeometry();
        const QSize hint = sizeHint().expandedTo(minimumSizeHint());
        resize(qMax(hint.width(), minimumWidth()), hint.height());
    }

    /**
     * @brief 外部（接线 / 存盘 / event 清脉冲）写回控件，不触发 parameterChanged
     */
    void setParameterValue(const QString& name, const QVariant& value)
    {
        QWidget* editor = m_editors.value(name, nullptr);
        if (!editor) {
            return;
        }
        const ISFPortDesc::Kind kind = m_kinds.value(name, ISFPortDesc::Kind::Unsupported);
        // 对直接控件挡住信号；Point2D 子控件另有 blocker
        QSignalBlocker blocker(editor);

        switch (kind) {
        case ISFPortDesc::Kind::Float: {
            if (auto* w = qobject_cast<FloatDragValueWidget*>(editor)) {
                w->setValue(value.toDouble());
            }
            break;
        }
        case ISFPortDesc::Kind::Long: {
            if (auto* combo = qobject_cast<QComboBox*>(editor)) {
                const int idx = combo->findData(value.toInt());
                combo->setCurrentIndex(idx >= 0 ? idx : 0);
            } else if (auto* w = qobject_cast<IntDragValueWidget*>(editor)) {
                w->setValue(value.toInt());
            }
            break;
        }
        case ISFPortDesc::Kind::Bool:
        case ISFPortDesc::Kind::Event: {
            if (auto* box = qobject_cast<QCheckBox*>(editor)) {
                box->setChecked(value.toBool());
            }
            break;
        }
        case ISFPortDesc::Kind::Point2D: {
            // 用 objectName 定位，避免依赖 children() 顺序
            auto* x = editor->findChild<FloatDragValueWidget*>(QStringLiteral("isfPointX"));
            auto* y = editor->findChild<FloatDragValueWidget*>(QStringLiteral("isfPointY"));
            if (!x || !y) {
                break;
            }
            const QVariantList list = value.toList();
            QSignalBlocker bx(x);
            QSignalBlocker by(y);
            x->setValue(list.size() > 0 ? list[0].toDouble() : 0.0);
            y->setValue(list.size() > 1 ? list[1].toDouble() : 0.0);
            break;
        }
        case ISFPortDesc::Kind::Color: {
            if (auto* btn = qobject_cast<QPushButton*>(editor)) {
                applyColorButton(btn, colorFromVariantLocal(value));
            }
            break;
        }
        default:
            break;
        }
    }

    /// 供 DataModel ExternalBinding / 信号连接使用（公开成员与工程内其它节点一致）
    SelectorComboBox* fileSelectComboBox =
        new SelectorComboBox(MediaLibrary::Category::ISF, this);
    QPushButton* reloadButton = new QPushButton(this);

signals:
    /** 用户改参；DataModel 写入 m_scalarValues 并置 dirty */
    void parameterChanged(const QString& name, const QVariant& value);

private:
    void clearParameters()
    {
        while (m_paramsForm->rowCount() > 0) {
            m_paramsForm->removeRow(0);
        }
        m_editors.clear();
        m_kinds.clear();
    }

    /** QColor 或 [r,g,b,a]（0–1 / 0–255）→ QColor */
    static QColor colorFromVariantLocal(const QVariant& value)
    {
        if (value.typeId() == QMetaType::QColor) {
            return value.value<QColor>();
        }
        const QVariantList list = value.toList();
        auto at = [&](int i, double fb) {
            return i < list.size() ? list.at(i).toDouble() : fb;
        };
        double r = at(0, 1.0);
        double g = at(1, 1.0);
        double b = at(2, 1.0);
        double a = at(3, 1.0);
        if (r > 1.0 || g > 1.0 || b > 1.0 || a > 1.0) {
            return QColor(qBound(0, int(r), 255),
                          qBound(0, int(g), 255),
                          qBound(0, int(b), 255),
                          qBound(0, int(a > 1.0 ? a : a * 255.0), 255));
        }
        QColor c;
        c.setRgbF(qBound(0.0, r, 1.0), qBound(0.0, g, 1.0), qBound(0.0, b, 1.0), qBound(0.0, a, 1.0));
        return c;
    }

    static void applyColorButton(QPushButton* btn, const QColor& c)
    {
        btn->setProperty("isfColor", c);
        btn->setText(c.name(QColor::HexArgb));
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: %1; color: %2; min-height: 22px; }")
                               .arg(c.name(QColor::HexArgb),
                                    c.lightness() > 140 ? QStringLiteral("black")
                                                        : QStringLiteral("white")));
    }

    QWidget* createEditor(const ISFPortDesc& port)
    {
        switch (port.kind) {
        case ISFPortDesc::Kind::Float: {
            auto* w = new FloatDragValueWidget(m_paramsHost);
            w->setDecimals(4);
            w->setSingleStep(0.01);
            w->setRange(port.hasMin ? port.minValue : -1e6,
                        port.hasMax ? port.maxValue : 1e6);
            connect(w, &FloatDragValueWidget::valueChanged, this,
                    [this, name = port.name](double v) {
                        Q_EMIT parameterChanged(name, v);
                    });
            return w;
        }
        case ISFPortDesc::Kind::Long: {
            if (!port.longLabels.isEmpty()
                && port.longLabels.size() == port.longValues.size()) {
                auto* combo = new QComboBox(m_paramsHost);
                for (int i = 0; i < port.longLabels.size(); ++i) {
                    combo->addItem(port.longLabels[i], port.longValues[i]);
                }
                connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
                        [this, name = port.name, combo](int) {
                            Q_EMIT parameterChanged(name, combo->currentData().toInt());
                        });
                return combo;
            }
            auto* w = new IntDragValueWidget(m_paramsHost);
            w->setRange(port.hasMin ? int(port.minValue) : -1000000,
                        port.hasMax ? int(port.maxValue) : 1000000);
            connect(w, &IntDragValueWidget::valueChanged, this,
                    [this, name = port.name](int v) {
                        Q_EMIT parameterChanged(name, v);
                    });
            return w;
        }
        case ISFPortDesc::Kind::Bool:
        case ISFPortDesc::Kind::Event: {
            auto* box = new QCheckBox(m_paramsHost);
            connect(box, &QCheckBox::toggled, this, [this, name = port.name](bool checked) {
                Q_EMIT parameterChanged(name, checked);
            });
            return box;
        }
        case ISFPortDesc::Kind::Point2D: {
            // 不用 VectorDragValueWidget：ISF MIN/MAX 是二维，两轴范围可不同
            auto* row = new QWidget(m_paramsHost);
            auto* lay = new QHBoxLayout(row);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(4);

            auto* x = new FloatDragValueWidget(row);
            auto* y = new FloatDragValueWidget(row);
            x->setObjectName(QStringLiteral("isfPointX"));
            y->setObjectName(QStringLiteral("isfPointY"));

            // MAX>2 多半是像素坐标，步长 1；否则按归一化点用 0.01
            const bool likelyPixel =
                (port.hasPointMax && (port.maxX > 2.0 || port.maxY > 2.0));
            const double step = likelyPixel ? 1.0 : 0.01;
            for (FloatDragValueWidget* s : {x, y}) {
                s->setDecimals(3);
                s->setSingleStep(step);
            }
            x->setRange(port.hasPointMin ? port.minX : -1e6,
                        port.hasPointMax ? port.maxX : 1e6);
            y->setRange(port.hasPointMin ? port.minY : -1e6,
                        port.hasPointMax ? port.maxY : 1e6);

            lay->addWidget(x, 1);
            lay->addWidget(y, 1);

            auto emitPoint = [this, name = port.name, x, y]() {
                Q_EMIT parameterChanged(name, QVariantList{x->value(), y->value()});
            };
            connect(x, &FloatDragValueWidget::valueChanged, this, emitPoint);
            connect(y, &FloatDragValueWidget::valueChanged, this, emitPoint);
            return row;
        }
        case ISFPortDesc::Kind::Color: {
            auto* btn = new QPushButton(m_paramsHost);
            applyColorButton(btn, Qt::white);
            connect(btn, &QPushButton::clicked, this, [this, name = port.name, btn]() {
                const QColor cur = btn->property("isfColor").value<QColor>();
                showIndependentColorEditor(name, cur.isValid() ? cur : Qt::white);
            });
            return btn;
        }
        default:
            return nullptr;
        }
    }

    void showIndependentColorEditor(const QString& paramName, const QColor& color)
    {
        if (!m_colorEditor) {
            return;
        }
        m_editingColorName = paramName;
        m_colorEditor->setParent(nullptr);
        m_colorEditor->setWindowTitle(QStringLiteral("ISF Color — %1").arg(paramName));
        m_colorEditor->setWindowFlags(
            Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
        m_colorEditor->setColor(color);
        m_colorEditor->resize(800, 400);
        m_colorEditor->show();
        m_colorEditor->raise();
        m_colorEditor->activateWindow();
    }

    QVBoxLayout* m_root = nullptr;
    QWidget* m_paramsHost = nullptr;
    QFormLayout* m_paramsForm = nullptr;
    QHash<QString, QWidget*> m_editors;           ///< ATTR name → 控件
    QHash<QString, ISFPortDesc::Kind> m_kinds;    ///< ATTR name → 类型（写回时用）
    ColorEditorWidget* m_colorEditor = nullptr;
    QString m_editingColorName;                   ///< 当前颜色编辑器对应的 ATTR
};

} // namespace Nodes
