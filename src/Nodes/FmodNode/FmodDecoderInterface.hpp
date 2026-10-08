/**
 * @file FmodDecoderInterface.hpp
 * @brief Fmod 节点内嵌 UI：Bank 路径 + 与输入口对齐的动态控件行
 *
 * 布局约定（QFormLayout，与 DataModel::inPorts_ 顺序一致）：
 * - 每个事件一行：事件短名 + Play 按钮（对应触发口）
 * - 每个用户参数一行：参数 caption + FloatDrag / IntDrag / Combo（对应参数口）
 */

#pragma once

#include "FmodDecoderWorker.h"

#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHash>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QWidget>
#include <cmath>

namespace Nodes
{

class FmodDecoderInterface : public QWidget
{
    Q_OBJECT
public:
    explicit FmodDecoderInterface(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(4, 2, 4, 4);
        root->setSpacing(4);

        auto* pathRow = new QHBoxLayout();
        pathRow->setContentsMargins(0, 0, 0, 0);
        pathRow->setSpacing(4);
        fileSelectComboBox->setPlaceholderText(QStringLiteral("FMOD Bank folder"));
        pathRow->addWidget(fileSelectComboBox, 1);
        selectButton->setText(QStringLiteral("Select"));
        pathRow->addWidget(selectButton);
        root->addLayout(pathRow);

        scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        paramsHost = new QWidget(scrollArea);
        paramsForm = new QFormLayout(paramsHost);
        paramsForm->setContentsMargins(0, 4, 0, 0);
        paramsForm->setSpacing(4);
        paramsForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        scrollArea->setWidget(paramsHost);
        root->addWidget(scrollArea, 1);

        setMinimumSize(QSize(260, 180));
    }

    /**
     * @brief 按事件/参数目录重建面板
     */
    void rebuildCatalog(const QStringList& events,
                        const QVector<FmodParamDesc>& params,
                        const QVariantMap& values)
    {
        clearEditors();

        QHash<QString, QVector<FmodParamDesc>> paramsByEvent;
        for (const FmodParamDesc& p : params) {
            paramsByEvent[p.eventPath].append(p);
        }

        for (const QString& eventPath : events) {
            auto* playBtn = new QPushButton(QStringLiteral("Play"), paramsHost);
            playBtn->setToolTip(eventPath);
            connect(playBtn, &QPushButton::clicked, this, [this, eventPath]() {
                Q_EMIT eventTriggered(eventPath);
            });
            paramsForm->addRow(eventDisplayName(eventPath), playBtn);

            for (const FmodParamDesc& p : paramsByEvent.value(eventPath)) {
                QWidget* editor = createEditor(p);
                if (!editor) {
                    continue;
                }
                const QString key = makeKey(p.eventPath, p.paramName);
                editors_.insert(key, editor);
                kinds_.insert(key, kindOf(p));
                paramsForm->addRow(p.caption, editor);
                setParameterValue(p.eventPath, p.paramName, values.value(key, p.defaultValue));
            }
        }

        paramsForm->activate();
        updateGeometry();
    }

    /** @brief 外部（参数口）写回控件显示，不触发 parameterChanged */
    void setParameterValue(const QString& eventPath, const QString& paramName, const QVariant& value)
    {
        const QString key = makeKey(eventPath, paramName);
        QWidget* editor = editors_.value(key, nullptr);
        if (!editor) {
            return;
        }
        QSignalBlocker blocker(editor);
        switch (kinds_.value(key)) {
        case Kind::Float:
            if (auto* w = qobject_cast<FloatDragValueWidget*>(editor)) {
                w->setValue(value.toDouble());
            }
            break;
        case Kind::Int:
            if (auto* w = qobject_cast<IntDragValueWidget*>(editor)) {
                w->setValue(value.toInt());
            }
            break;
        case Kind::Labeled:
            if (auto* combo = qobject_cast<QComboBox*>(editor)) {
                const int idx = combo->findData(value.toInt());
                combo->setCurrentIndex(idx >= 0 ? idx : 0);
            }
            break;
        }
    }

    static QString makeKey(const QString& eventPath, const QString& paramName)
    {
        return eventPath + QChar(0x1f) + paramName;
    }

    static QString eventDisplayName(const QString& eventPath)
    {
        if (eventPath.startsWith(QStringLiteral("event:/"))) {
            return eventPath.mid(7);
        }
        return eventPath;
    }

signals:
    void parameterChanged(const QString& eventPath, const QString& paramName, float value);
    void eventTriggered(const QString& eventPath);

public:
    QLineEdit* fileSelectComboBox = new QLineEdit(this);
    QPushButton* selectButton = new QPushButton(this);
    QScrollArea* scrollArea = nullptr;
    QWidget* paramsHost = nullptr;
    QFormLayout* paramsForm = nullptr;

private:
    enum class Kind { Float, Int, Labeled };

    static Kind kindOf(const FmodParamDesc& p)
    {
        if (p.labeled) {
            return Kind::Labeled;
        }
        if (p.discrete) {
            return Kind::Int;
        }
        return Kind::Float;
    }

    void clearEditors()
    {
        while (paramsForm && paramsForm->rowCount() > 0) {
            paramsForm->removeRow(0);
        }
        editors_.clear();
        kinds_.clear();
    }

    QWidget* createEditor(const FmodParamDesc& p)
    {
        const QString eventPath = p.eventPath;
        const QString paramName = p.paramName;

        if (p.labeled && !p.labels.isEmpty()) {
            auto* combo = new QComboBox(paramsHost);
            const int first = static_cast<int>(std::lround(p.minimum));
            for (int i = 0; i < p.labels.size(); ++i) {
                combo->addItem(p.labels.at(i), first + i);
            }
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, combo, eventPath, paramName](int) {
                        Q_EMIT parameterChanged(
                            eventPath, paramName, static_cast<float>(combo->currentData().toInt()));
                    });
            return combo;
        }

        if (p.discrete) {
            auto* w = new IntDragValueWidget(paramsHost);
            w->setRange(static_cast<int>(std::lround(p.minimum)),
                        static_cast<int>(std::lround(p.maximum)));
            w->setSingleStep(1);
            connect(w, &IntDragValueWidget::valueChanged, this,
                    [this, eventPath, paramName](int v) {
                        Q_EMIT parameterChanged(eventPath, paramName, static_cast<float>(v));
                    });
            return w;
        }

        auto* w = new FloatDragValueWidget(paramsHost);
        w->setRange(p.minimum, p.maximum);
        w->setDecimals(3);
        const double span = static_cast<double>(p.maximum) - static_cast<double>(p.minimum);
        w->setSingleStep(span > 0.0 ? span / 100.0 : 0.01);
        connect(w, &FloatDragValueWidget::valueChanged, this,
                [this, eventPath, paramName](double v) {
                    Q_EMIT parameterChanged(eventPath, paramName, static_cast<float>(v));
                });
        return w;
    }

    QHash<QString, QWidget*> editors_;
    QHash<QString, Kind> kinds_;
};

} // namespace Nodes
