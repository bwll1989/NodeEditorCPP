#pragma once

#include "PortAlignedColumn/PortAlignedColumn.hpp"
#include "HotKeyItem.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMap>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QWidget>

namespace Nodes
{
    /**
     * Keyboard In：一行对应一个输出端口；左侧为添加与全局选项。
     */
    class HotKeyInterface : public QWidget
    {
        Q_OBJECT
    public:
        enum ModifierMode {
            Ignore = 0,
            None,
            Control,
            Alt,
            ControlAndAlt,
            Shift,
            ShiftAndAlt,
            ShiftAndControl,
            ShiftControlAndAlt
        };

        static constexpr int kEmbeddedWidth = 320;

        explicit HotKeyInterface(QWidget *parent = nullptr)
            : QWidget(parent)
            , m_column(new PortAlignedColumn(this))
            , m_rowHeight(PortAlignedColumn::rowPitch())
        {
            setObjectName(QStringLiteral("HotKeyInterface"));
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumWidth(kEmbeddedWidth);

            m_addButton = new QPushButton(this);
            m_addButton->setIcon(QIcon(QStringLiteral(":/icons/icons/add.png")));
            m_addButton->setIconSize(QSize(14, 14));
            m_addButton->setFlat(true);
            m_addButton->setToolTip(QStringLiteral("添加输出按键行"));
            connect(m_addButton, &QPushButton::clicked, this, &HotKeyInterface::onAddClicked);

            m_activeCheck = new QCheckBox(QStringLiteral("On"), this);
            m_activeCheck->setChecked(true);
            m_activeCheck->setToolTip(QStringLiteral("使能监视键盘（与 ENABLE 输入同一状态）"));
            connect(m_activeCheck, &QCheckBox::toggled, this, &HotKeyInterface::onActiveToggled);

            auto *modLabel = new QLabel(QStringLiteral("Mod"), this);

            m_modifierCombo = new QComboBox(this);
            m_modifierCombo->addItem(QStringLiteral("Ignore"), Ignore);
            m_modifierCombo->addItem(QStringLiteral("None"), None);
            m_modifierCombo->addItem(QStringLiteral("Ctrl"), Control);
            m_modifierCombo->addItem(QStringLiteral("Alt"), Alt);
            m_modifierCombo->addItem(QStringLiteral("Ctrl+Alt"), ControlAndAlt);
            m_modifierCombo->addItem(QStringLiteral("Shift"), Shift);
            m_modifierCombo->addItem(QStringLiteral("Shift+Alt"), ShiftAndAlt);
            m_modifierCombo->addItem(QStringLiteral("Shift+Ctrl"), ShiftAndControl);
            m_modifierCombo->addItem(QStringLiteral("Shift+Ctrl+Alt"), ShiftControlAndAlt);
            m_modifierCombo->setToolTip(QStringLiteral("修饰键过滤"));
            m_modifierCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

            auto *left = new QVBoxLayout();
            left->setContentsMargins(4, 2, 6, 2);
            left->setSpacing(6);
            left->addWidget(m_addButton, 0, Qt::AlignLeft);
            left->addWidget(m_activeCheck, 0, Qt::AlignLeft);
            left->addWidget(modLabel, 0, Qt::AlignLeft);
            left->addWidget(m_modifierCombo, 0);
            left->addStretch(1);

            auto *leftWrap = new QWidget(this);
            leftWrap->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            leftWrap->setLayout(left);

            auto *root = new QHBoxLayout(this);
            root->setContentsMargins(2, 2, 2, 2);
            root->setSpacing(6);
            root->addWidget(leftWrap, 0);
            root->addWidget(m_column, 1);
            setLayout(root);

            setMinimumRows(1);
            setRowCount(4);

            if (qApp) {
                qApp->installEventFilter(this);
            }
        }

        ~HotKeyInterface() override
        {
            if (qApp) {
                qApp->removeEventFilter(this);
            }
        }

        int rowCount() const { return m_column->rowCount(); }

        bool isPortPressed(int port) const
        {
            if (!isActive()) {
                return false;
            }
            if (auto *row = rowAt(port)) {
                return row->isPressed();
            }
            return false;
        }

        void setRowCount(int count)
        {
            count = qMax(m_minimumRows, count);
            const QSignalBlocker blocker(this);
            while (m_column->rowCount() < count) {
                appendRowInternal(QKeySequence());
            }
            while (m_column->rowCount() > count) {
                m_column->removeRow(m_column->rowCount() - 1);
            }
            updateGeometry();
        }

        void setMinimumRows(int count)
        {
            m_minimumRows = qMax(1, count);
            if (m_column->rowCount() < m_minimumRows) {
                setRowCount(m_minimumRows);
            }
        }

        QJsonArray exportKeys() const
        {
            QJsonArray arr;
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (auto *row = rowAt(i)) {
                    arr.append(row->toJson(i));
                }
            }
            return arr;
        }

        void importKeys(const QJsonArray &arr)
        {
            const QSignalBlocker blocker(this);

            // 兼容旧数据：按 port 落位；无 port 时按数组顺序
            QMap<int, QKeySequence> byPort;
            int maxPort = -1;
            int fallback = 0;
            for (const QJsonValue &v : arr) {
                if (!v.isObject()) {
                    continue;
                }
                const QJsonObject obj = v.toObject();
                int port = obj.value(QStringLiteral("port")).toInt(-1);
                if (port < 0) {
                    port = fallback;
                }
                byPort.insert(port, QKeySequence(obj.value(QStringLiteral("value")).toString()));
                maxPort = qMax(maxPort, port);
                fallback = qMax(fallback, port + 1);
            }

            const int target = qMax(m_minimumRows, maxPort + 1);
            while (m_column->rowCount() < target) {
                appendRowInternal(QKeySequence());
            }
            while (m_column->rowCount() > target) {
                m_column->removeRow(m_column->rowCount() - 1);
            }
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (auto *row = rowAt(i)) {
                    row->setKeySequence(byPort.value(i));
                }
            }
            updateGeometry();
        }

        bool isActive() const { return m_activeCheck && m_activeCheck->isChecked(); }

        void setActive(bool active)
        {
            if (!m_activeCheck || m_activeCheck->isChecked() == active) {
                return;
            }
            const QSignalBlocker blocker(m_activeCheck);
            m_activeCheck->setChecked(active);
            if (!active) {
                releaseAllPressed();
            }
        }

        ModifierMode modifierMode() const
        {
            return static_cast<ModifierMode>(m_modifierCombo->currentData().toInt());
        }

        void setModifierMode(int mode)
        {
            const int idx = m_modifierCombo->findData(mode);
            if (idx >= 0) {
                m_modifierCombo->setCurrentIndex(idx);
            }
        }

        int modifierModeValue() const { return m_modifierCombo->currentData().toInt(); }

        QSize sizeHint() const override
        {
            const int rows = qMax(1, m_column->rowCount());
            return QSize(kEmbeddedWidth, qMax(rows * m_rowHeight, 80));
        }

        QSize minimumSizeHint() const override
        {
            const int rows = qMax(1, m_column->rowCount());
            return QSize(kEmbeddedWidth, qMax(rows * m_rowHeight, 80));
        }

    signals:
        void keyStateChanged(int port);
        void listChanged();
        void rowAppended();
        void rowRemoved(int index);

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override
        {
            Q_UNUSED(watched)

            if (!isActive()) {
                return false;
            }

            const auto type = event->type();
            if (type != QEvent::KeyPress && type != QEvent::KeyRelease) {
                return false;
            }

            auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->isAutoRepeat()) {
                return false;
            }

            if (qobject_cast<QKeySequenceEdit *>(QApplication::focusWidget())) {
                return false;
            }

            if (!modifiersMatch(keyEvent->modifiers())) {
                releaseAllPressed();
                return false;
            }

            const bool pressed = (type == QEvent::KeyPress);
            for (int i = 0; i < m_column->rowCount(); ++i) {
                HotKeyItem *item = rowAt(i);
                if (!item || item->keySequence().isEmpty()) {
                    continue;
                }
                if (!keyMatches(item->keySequence(), keyEvent->key())) {
                    continue;
                }
                if (item->setPressed(pressed)) {
                    Q_EMIT keyStateChanged(i);
                }
            }

            return false;
        }

    private slots:
        void onAddClicked()
        {
            appendRowInternal(QKeySequence());
            updateGeometry();
            Q_EMIT listChanged();
            Q_EMIT rowAppended();
        }

        void onRowDeleteRequested()
        {
            auto *row = qobject_cast<HotKeyItem *>(sender());
            if (!row) {
                return;
            }
            const int index = indexOfRow(row);
            if (index < 0 || m_column->rowCount() <= m_minimumRows) {
                return;
            }
            const bool wasPressed = row->isPressed();
            m_column->removeRow(index);
            updateGeometry();
            Q_EMIT listChanged();
            Q_EMIT rowRemoved(index);
            if (wasPressed) {
                Q_EMIT keyStateChanged(index);
            }
        }

        void onActiveToggled(bool active)
        {
            if (!active) {
                releaseAllPressed();
            }
            Q_EMIT listChanged();
        }

    private:
        HotKeyItem *rowAt(int index) const
        {
            return qobject_cast<HotKeyItem *>(m_column->rowContent(index));
        }

        int indexOfRow(HotKeyItem *row) const
        {
            for (int i = 0; i < m_column->rowCount(); ++i) {
                if (m_column->rowContent(i) == row) {
                    return i;
                }
            }
            return -1;
        }

        void appendRowInternal(const QKeySequence &key)
        {
            auto *row = new HotKeyItem(m_rowHeight, m_column);
            if (!key.isEmpty()) {
                row->setKeySequence(key);
            }
            connect(row, &HotKeyItem::configChanged, this, &HotKeyInterface::listChanged);
            connect(row, &HotKeyItem::deleteRequested, this, &HotKeyInterface::onRowDeleteRequested);
            m_column->appendRow(row);
        }

        void releaseAllPressed()
        {
            QSet<int> ports;
            for (int i = 0; i < m_column->rowCount(); ++i) {
                HotKeyItem *item = rowAt(i);
                if (item && item->isPressed() && item->setPressed(false)) {
                    ports.insert(i);
                }
            }
            for (int port : ports) {
                Q_EMIT keyStateChanged(port);
            }
        }

        bool modifiersMatch(Qt::KeyboardModifiers mods) const
        {
            const bool ctrl = mods.testFlag(Qt::ControlModifier);
            const bool alt = mods.testFlag(Qt::AltModifier);
            const bool shift = mods.testFlag(Qt::ShiftModifier);

            switch (modifierMode()) {
            case Ignore:
                return true;
            case None:
                return !ctrl && !alt && !shift;
            case Control:
                return ctrl && !alt && !shift;
            case Alt:
                return !ctrl && alt && !shift;
            case ControlAndAlt:
                return ctrl && alt && !shift;
            case Shift:
                return !ctrl && !alt && shift;
            case ShiftAndAlt:
                return !ctrl && alt && shift;
            case ShiftAndControl:
                return ctrl && !alt && shift;
            case ShiftControlAndAlt:
                return ctrl && alt && shift;
            }
            return true;
        }

        static bool keyMatches(const QKeySequence &sequence, int key)
        {
            if (sequence.isEmpty() || key == 0) {
                return false;
            }
            const QKeyCombination combo = sequence[0];
            return static_cast<int>(combo.key()) == key;
        }

        PortAlignedColumn *m_column = nullptr;
        QPushButton *m_addButton = nullptr;
        QCheckBox *m_activeCheck = nullptr;
        QComboBox *m_modifierCombo = nullptr;
        int m_rowHeight = 23;
        int m_minimumRows = 1;
    };
}
