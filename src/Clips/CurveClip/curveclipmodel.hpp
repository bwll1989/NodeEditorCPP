#ifndef CURVECLIPMODEL_H
#define CURVECLIPMODEL_H

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "AbstractClipModel.hpp"
#include "Common/BaseClass/AbstractClipDelegateModel.h"
#include "CurveViewport.hpp"
#include "FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "OSCMessage.h"
#include "OscListWidget/OSCMessageListWidget.hpp"
#include "OscListWidget/OSCMessageItemWidget.hpp"

namespace Clips
{
class CurveClipModel : public AbstractClipDelegateModel
{
    Q_OBJECT
public:
    explicit CurveClipModel(int start)
        : AbstractClipDelegateModel(start, QStringLiteral("Curve"))
        , m_path(new CurvePath(this))
        , m_listWidget(new OSCMessageListWidget(true, nullptr))
    {
        m_end = start + 100;
        ClipColor = QColor("#3a86ff");
        initPropertyWidget();
        connect(m_path, &CurvePath::curveChanged, this, [this] { republish(); });
        connect(m_path, &CurvePath::selectionChanged, this, [this] { syncEditors(); });
    }

    ~CurveClipModel() override
    {
        GlobalEventBus::instance()->unsubscribe(this);
        for (const auto& pair : _OscMapping) {
            StatusContainer::instance()->registerWidget(nullptr, makeFullOscAddress(pair.first));
        }
        if (m_viewport) {
            m_viewport->setPath(nullptr);
            m_viewport->releaseResources();
            m_viewport = nullptr;
        }
        m_listWidget = nullptr;
        m_editor = nullptr;
        m_closedBox = nullptr;
        m_pointLabel = nullptr;
        m_readout = nullptr;
        m_x = nullptr;
        m_y = nullptr;
        m_z = nullptr;
        m_clipPropertyWidget = nullptr;
        if (m_standardPropertyWidget) {
            QWidget* panel = m_standardPropertyWidget;
            m_standardPropertyWidget = nullptr;
            panel->hide();
            panel->deleteLater();
        }
    }

    QJsonObject save() const override
    {
        QJsonObject json = AbstractClipModel::save();
        json["points"] = m_path->toJson();
        json["closed"] = m_path->isClosed();
        json["messages"] = m_listWidget->save().value(QStringLiteral("messages")).toArray();
        return json;
    }

    void load(const QJsonObject& json) override
    {
        AbstractClipModel::load(json);
        if (json.contains(QStringLiteral("messages"))) {
            m_listWidget->load(json);
        } else if (json.contains(QStringLiteral("address"))) {
            OSCMessage message;
            message.address = json.value(QStringLiteral("address")).toString();
            message.type = QStringLiteral("List");
            if (!message.address.isEmpty()) {
                m_listWidget->addOSCMessage(message);
            }
        }
        if (m_closedBox) {
            const QSignalBlocker blocker(m_closedBox);
            m_closedBox->setChecked(json.value(QStringLiteral("closed")).toBool(false));
        }
        m_path->fromJson(json.value(QStringLiteral("points")).toArray(), json.value(QStringLiteral("closed")).toBool(false));
        if (m_viewport) {
            m_viewport->frameAll(true);
        }
    }

    QVariant data(int role) const override
    {
        if (role == TimelineRoles::ClipModelRole) {
            return QVariant::fromValue(static_cast<AbstractClipModel*>(const_cast<CurveClipModel*>(this)));
        }
        return AbstractClipModel::data(role);
    }

    QVariantMap currentData(int currentFrame) const override
    {
        if (currentFrame >= m_start && currentFrame <= m_end && m_currentFrame != currentFrame) {
            const int span = std::max(1, m_end - m_start);
            const double fraction = double(currentFrame - m_start) / double(span);
            m_currentFrame = currentFrame;
            m_fraction = fraction;
            const_cast<CurveClipModel*>(this)->publishAt(fraction);
        }
        return {};
    }

    QWidget* clipPropertyWidget() override
    {
        m_editor = new QWidget();
        auto* mainLayout = new QVBoxLayout(m_editor);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(4);

        auto* group = new QGroupBox(QStringLiteral("三维曲线"), m_editor);
        auto* layout = new QVBoxLayout(group);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);

        auto* tools = new QHBoxLayout();
        auto* addButton = new QPushButton(QStringLiteral("添加点"), group);
        auto* removeButton = new QPushButton(QStringLiteral("删除点"), group);
        auto* frameButton = new QPushButton(QStringLiteral("适应视图"), group);
        m_closedBox = new QCheckBox(QStringLiteral("闭合"), group);
        tools->addWidget(addButton);
        tools->addWidget(removeButton);
        tools->addWidget(frameButton);
        tools->addWidget(m_closedBox);
        tools->addStretch();
        layout->addLayout(tools);

        m_viewport = new CurveViewport(group);
        m_viewport->setPath(m_path);
        m_viewport->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(m_viewport, 1);

        auto* pointRow = new QHBoxLayout();
        m_pointLabel = new QLabel(group);
        m_x = makeAxisEditor(QStringLiteral("X"), group);
        m_y = makeAxisEditor(QStringLiteral("Y"), group);
        m_z = makeAxisEditor(QStringLiteral("Z"), group);
        pointRow->addWidget(m_pointLabel);
        pointRow->addWidget(m_x, 1);
        pointRow->addWidget(m_y, 1);
        pointRow->addWidget(m_z, 1);
        layout->addLayout(pointRow);

        layout->addWidget(m_listWidget);

        m_readout = new QLabel(group);
        m_readout->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(m_readout);

        group->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mainLayout->addWidget(group);

        connect(addButton, &QPushButton::clicked, this, [this] {
            const QVector3D last = m_path->points().isEmpty() ? QVector3D() : m_path->points().last();
            m_path->addPoint(last + QVector3D(0.5f, 0.5f, 0.0f));
        });
        connect(removeButton, &QPushButton::clicked, this, [this] {
            m_path->removePoint(m_path->selectedIndex());
        });
        connect(frameButton, &QPushButton::clicked, this, [this] {
            if (m_viewport) {
                m_viewport->frameAll(false);
            }
        });
        connect(m_closedBox, &QCheckBox::toggled, m_path, &CurvePath::setClosed);
        const auto editAxis = [this] {
            const int index = m_path->selectedIndex();
            if (index < 0 || !m_x || !m_y || !m_z) {
                return;
            }
            m_path->setPoint(index, QVector3D(float(m_x->value()), float(m_y->value()), float(m_z->value())));
        };
        connect(m_x, &FloatDragValueWidget::valueChanged, this, editAxis);
        connect(m_y, &FloatDragValueWidget::valueChanged, this, editAxis);
        connect(m_z, &FloatDragValueWidget::valueChanged, this, editAxis);

        syncEditors();
        const QVector3D startPosition = m_path->positionAtFraction(0.0);
        if (m_readout) {
            m_readout->setText(QStringLiteral("输出 [%1, %2, %3]")
                .arg(double(startPosition.x()), 0, 'f', 3)
                .arg(double(startPosition.y()), 0, 'f', 3)
                .arg(double(startPosition.z()), 0, 'f', 3));
        }
        return m_editor;
    }

public slots:
    void onGlobalEvent(const GlobalEvent& ev)
    {
        if (ev.kind != GlobalEventKind::Command || !timeGroupBox) {
            return;
        }
        const QString addrStartFrame = makeFullOscAddress("/start_frame");
        const QString addrEndFrame = makeFullOscAddress("/end_frame");
        const QString addrPositionFrame = makeFullOscAddress("/position_frame");
        if (ev.address == addrStartFrame && timeGroupBox->m_startFrameSpinBox) {
            bool ok = false;
            const int v = ev.payload.toInt(&ok);
            if (!ok) return;
            QSignalBlocker blocker(timeGroupBox->m_startFrameSpinBox);
            setStart(v);
            timeGroupBox->m_startFrameSpinBox->setValue(v);
        } else if (ev.address == addrEndFrame && timeGroupBox->m_endFrameSpinBox) {
            bool ok = false;
            const int v = ev.payload.toInt(&ok);
            if (!ok) return;
            QSignalBlocker blocker(timeGroupBox->m_endFrameSpinBox);
            setEnd(v);
            timeGroupBox->m_endFrameSpinBox->setValue(v);
        } else if (ev.address == addrPositionFrame && timeGroupBox->m_endFrameSpinBox && timeGroupBox->m_startFrameSpinBox) {
            bool ok = false;
            const int v = ev.payload.toInt(&ok);
            if (!ok) return;
            const int len = length();
            QSignalBlocker blocker(timeGroupBox->m_startFrameSpinBox);
            QSignalBlocker blocker2(timeGroupBox->m_endFrameSpinBox);
            setStart(v);
            setEnd(v + len);
            timeGroupBox->m_startFrameSpinBox->setValue(v);
            timeGroupBox->m_endFrameSpinBox->setValue(v + len);
        }
    }

protected:
    void afterModelReady() override
    {
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/start_frame"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/end_frame"), this, SLOT(onGlobalEvent(GlobalEvent)));
        GlobalEventBus::instance()->subscribe(makeFullOscAddress("/position_frame"), this, SLOT(onGlobalEvent(GlobalEvent)));
    }

private:
    FloatDragValueWidget* makeAxisEditor(const QString& suffix, QWidget* parent)
    {
        auto* editor = new FloatDragValueWidget(parent);
        editor->setRange(-10000.0, 10000.0);
        editor->setDecimals(3);
        editor->setSingleStep(0.01);
        editor->setSuffix(suffix);
        return editor;
    }

    void syncEditors()
    {
        if (!m_x || !m_y || !m_z) {
            return;
        }
        const int index = m_path->selectedIndex();
        const bool enabled = index >= 0 && index < m_path->points().size();
        m_x->setEnabled(enabled);
        m_y->setEnabled(enabled);
        m_z->setEnabled(enabled);
        if (m_pointLabel) {
            m_pointLabel->setText(enabled
                ? QStringLiteral("点 %1").arg(index + 1)
                : QStringLiteral("未选择"));
        }
        if (!enabled) {
            return;
        }
        const QVector3D point = m_path->points().at(index);
        const QSignalBlocker blockX(m_x);
        const QSignalBlocker blockY(m_y);
        const QSignalBlocker blockZ(m_z);
        m_x->setValue(point.x());
        m_y->setValue(point.y());
        m_z->setValue(point.z());
    }

    void republish()
    {
        if (m_currentFrame >= m_start && m_currentFrame <= m_end) {
            publishAt(m_fraction);
        }
    }

    void publishAt(double fraction)
    {
        const QVector3D position = m_path->positionAtFraction(fraction);
        QVariantList value;
        value << double(position.x()) << double(position.y()) << double(position.z());
        const int localFrame = m_currentFrame >= m_start ? m_currentFrame - m_start : 0;

        if (m_listWidget) {
            for (int i = 0; i < m_listWidget->count(); ++i) {
                OSCMessageItemWidget* widget = m_listWidget->item(i);
                if (!widget || !widget->getJSEngine()) {
                    continue;
                }
                auto* engine = widget->getJSEngine();
                engine->globalObject().setProperty(QStringLiteral("$percent"), fraction);
                engine->globalObject().setProperty(QStringLiteral("$frame"), localFrame);
                engine->globalObject().setProperty(QStringLiteral("$x"), double(position.x()));
                engine->globalObject().setProperty(QStringLiteral("$y"), double(position.y()));
                engine->globalObject().setProperty(QStringLiteral("$z"), double(position.z()));
            }
            for (int i = 0; i < m_listWidget->count(); ++i) {
                OSCMessageItemWidget* widget = m_listWidget->item(i);
                if (!widget || widget->getAddress().trimmed().isEmpty()) {
                    continue;
                }
                OSCMessage message = widget->getMessage();
                if (widget->getExpression().trimmed().isEmpty()) {
                    message.type = QStringLiteral("List");
                    message.value = value;
                }
                StatusContainer::instance()->parseOSC(message);
            }
        }

        if (m_viewport) {
            m_viewport->setPlaybackFraction(fraction);
        }
        if (m_readout) {
            m_readout->setText(QStringLiteral("输出 [%1, %2, %3]")
                .arg(double(position.x()), 0, 'f', 3)
                .arg(double(position.y()), 0, 'f', 3)
                .arg(double(position.z()), 0, 'f', 3));
        }
    }

    CurvePath* m_path = nullptr;
    QWidget* m_editor = nullptr;
    CurveViewport* m_viewport = nullptr;
    QCheckBox* m_closedBox = nullptr;
    OSCMessageListWidget* m_listWidget = nullptr;
    QLabel* m_pointLabel = nullptr;
    QLabel* m_readout = nullptr;
    FloatDragValueWidget* m_x = nullptr;
    FloatDragValueWidget* m_y = nullptr;
    FloatDragValueWidget* m_z = nullptr;
    mutable int m_currentFrame = -1;
    mutable double m_fraction = 0.0;
};
}

#endif
