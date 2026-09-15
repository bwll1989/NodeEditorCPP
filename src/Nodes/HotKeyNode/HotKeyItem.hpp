#pragma once

#include <QHBoxLayout>
#include <QIcon>
#include <QJsonObject>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QWidget>

/**
 * HotKey 单行：左侧删除 + 按键录制。
 * 行索引即输出端口索引；行高由外部传入（与 port step 一致）。
 */
class HotKeyItem : public QWidget
{
    Q_OBJECT
public:
    explicit HotKeyItem(int rowHeight, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_rowHeight(qMax(1, rowHeight))
    {
        setObjectName(QStringLiteral("HotKeyItem"));
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFixedHeight(m_rowHeight);
        setMinimumHeight(m_rowHeight);
        setMaximumHeight(m_rowHeight);

        const int inner = qMax(1, m_rowHeight - 2);
        const int icon = qBound(10, 14, inner - 2);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(2, 0, 2, 0);
        layout->setSpacing(2);

        m_deleteBtn = new QPushButton(this);
        m_deleteBtn->setIcon(QIcon(QStringLiteral(":/icons/icons/remove.png")));
        m_deleteBtn->setIconSize(QSize(icon, icon));
        m_deleteBtn->setFixedSize(inner, inner);
        m_deleteBtn->setFlat(true);
        m_deleteBtn->setFocusPolicy(Qt::NoFocus);
        m_deleteBtn->setCursor(Qt::PointingHandCursor);
        m_deleteBtn->setToolTip(QStringLiteral("删除"));
        connect(m_deleteBtn, &QPushButton::clicked, this, &HotKeyItem::deleteRequested);

        m_keyEdit = new QKeySequenceEdit(this);
        m_keyEdit->setMaximumSequenceLength(1);
        m_keyEdit->setFixedHeight(inner);
        m_keyEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_keyEdit->setToolTip(QStringLiteral("录制要监视的按键"));
        connect(m_keyEdit, &QKeySequenceEdit::keySequenceChanged,
                this, [this](const QKeySequence &) { Q_EMIT configChanged(); });

        if (auto *lineEdit = m_keyEdit->findChild<QLineEdit *>()) {
            lineEdit->setFixedHeight(inner);
            lineEdit->setMinimumHeight(inner);
            lineEdit->setMaximumHeight(inner);
            lineEdit->setFrame(false);
            lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }

        layout->addWidget(m_deleteBtn, 0, Qt::AlignVCenter);
        layout->addWidget(m_keyEdit, 1, Qt::AlignVCenter);
    }

    QKeySequence keySequence() const
    {
        return m_keyEdit ? m_keyEdit->keySequence() : QKeySequence();
    }

    void setKeySequence(const QKeySequence &seq)
    {
        if (m_keyEdit) {
            m_keyEdit->setKeySequence(seq);
        }
    }

    bool isPressed() const { return m_pressed; }

    bool setPressed(bool pressed)
    {
        if (m_pressed == pressed) {
            return false;
        }
        m_pressed = pressed;
        return true;
    }

    QJsonObject toJson(int port) const
    {
        QJsonObject obj;
        obj.insert(QStringLiteral("port"), port);
        obj.insert(QStringLiteral("value"), keySequence().toString());
        return obj;
    }

signals:
    void configChanged();
    void deleteRequested();

private:
    QPushButton *m_deleteBtn = nullptr;
    QKeySequenceEdit *m_keyEdit = nullptr;
    int m_rowHeight = 23;
    bool m_pressed = false;
};
