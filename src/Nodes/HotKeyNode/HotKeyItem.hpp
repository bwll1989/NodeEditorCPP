#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QKeySequenceEdit>
#include <QJsonObject>
#include <QPixmap>
#include <QSizePolicy>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * 单行按键配置：Key + Index；默认适配 240 宽，可随节点拉伸。
 */
class HotKeyItem : public QWidget
{
    Q_OBJECT
public:
    explicit HotKeyItem(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(2, 0, 2, 0);
        layout->setSpacing(4);
        layout->setAlignment(Qt::AlignVCenter);

        portIndex->setMinimum(0);
        portIndex->setFixedWidth(32);
        portIndex->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        portIndex->setToolTip(QStringLiteral("输出端口索引"));

        keyEdit->setMaximumSequenceLength(1);
        keyEdit->setMinimumWidth(96);
        keyEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        keyEdit->setToolTip(QStringLiteral("录制要监视的按键"));

        stateIndicator->setFixedSize(10, 10);
        stateIndicator->setToolTip(QStringLiteral("按下状态"));
        updateStyle();

        auto *keyLabel = new QLabel(QStringLiteral("Key"), this);
        auto *idxLabel = new QLabel(QStringLiteral("Idx"), this);

        auto *moveLabel = new QLabel(this);
        moveLabel->setFixedSize(12, 12);
        moveLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        moveLabel->setPixmap(QPixmap(QStringLiteral(":/icons/icons/move.png"))
                                 .scaled(moveLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        moveLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(keyLabel, 0, Qt::AlignVCenter);
        layout->addWidget(keyEdit, 1, Qt::AlignVCenter);
        layout->addWidget(idxLabel, 0, Qt::AlignVCenter);
        layout->addWidget(portIndex, 0, Qt::AlignVCenter);
        layout->addWidget(stateIndicator, 0, Qt::AlignVCenter);
        layout->addWidget(moveLabel, 0, Qt::AlignVCenter);
        layout->setStretch(1, 1);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumWidth(220);
        setFixedHeight(28);

        connect(portIndex, &IntDragValueWidget::valueChanged,
                this, &HotKeyItem::configChanged);
        connect(keyEdit, &QKeySequenceEdit::keySequenceChanged,
                this, [this](const QKeySequence &) { Q_EMIT configChanged(); });
    }

    QSize sizeHint() const override
    {
        return QSize(240, 26);
    }

    QSize minimumSizeHint() const override
    {
        return QSize(240, 26);
    }

    int outPort() const { return portIndex->value(); }

    void setOutPort(int port) { portIndex->setValue(port); }

    QKeySequence keySequence() const { return keyEdit->keySequence(); }

    void setKeySequence(const QKeySequence &seq) { keyEdit->setKeySequence(seq); }

    bool isPressed() const { return m_pressed; }

    bool setPressed(bool pressed)
    {
        if (m_pressed == pressed) {
            return false;
        }
        m_pressed = pressed;
        updateStyle();
        return true;
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj[QStringLiteral("port")] = portIndex->value();
        obj[QStringLiteral("value")] = keyEdit->keySequence().toString();
        return obj;
    }

    void fromJson(const QJsonObject &obj)
    {
        portIndex->setValue(obj[QStringLiteral("port")].toInt(0));
        keyEdit->setKeySequence(QKeySequence(obj[QStringLiteral("value")].toString()));
    }

signals:
    void configChanged();

private:
    void updateStyle()
    {
        stateIndicator->setStyleSheet(
            m_pressed
                ? QStringLiteral("QFrame{background-color:#00C853;border:none;border-radius:5px;}")
                : QStringLiteral("QFrame{background-color:#E53935;border:none;border-radius:5px;}"));
    }

    IntDragValueWidget *portIndex = new IntDragValueWidget(this);
    QKeySequenceEdit *keyEdit = new QKeySequenceEdit(this);
    QFrame *stateIndicator = new QFrame(this);
    bool m_pressed = false;
};
