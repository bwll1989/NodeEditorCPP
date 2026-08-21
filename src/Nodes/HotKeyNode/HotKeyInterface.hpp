#pragma once

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QSet>
#include <QWidget>

#include "HotKeyItem.hpp"

namespace Nodes
{
    /**
     * Keyboard In 界面：参考 DelayInterface，列表与输出端口数量相互独立。
     */
    class HotKeyInterface : public QFrame
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

        explicit HotKeyInterface(QWidget *parent = nullptr)
            : QFrame(parent)
        {
            keyList->setDragEnabled(true);
            keyList->setAcceptDrops(true);
            keyList->setDragDropMode(QAbstractItemView::InternalMove);
            keyList->setSelectionMode(QAbstractItemView::SingleSelection);
            keyList->setContextMenuPolicy(Qt::CustomContextMenu);
            keyList->setMinimumHeight(64);
            keyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            keyList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            keyList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            activeCheck->setChecked(true);
            activeCheck->setText(QStringLiteral("On"));
            activeCheck->setToolTip(QStringLiteral("开启后监视键盘；关闭时不再更新输出"));

            modifierCombo->addItem(QStringLiteral("Ignore"), Ignore);
            modifierCombo->addItem(QStringLiteral("None"), None);
            modifierCombo->addItem(QStringLiteral("Ctrl"), Control);
            modifierCombo->addItem(QStringLiteral("Alt"), Alt);
            modifierCombo->addItem(QStringLiteral("Ctrl+Alt"), ControlAndAlt);
            modifierCombo->addItem(QStringLiteral("Shift"), Shift);
            modifierCombo->addItem(QStringLiteral("Shift+Alt"), ShiftAndAlt);
            modifierCombo->addItem(QStringLiteral("Shift+Ctrl"), ShiftAndControl);
            modifierCombo->addItem(QStringLiteral("Shift+Ctrl+Alt"), ShiftControlAndAlt);
            modifierCombo->setToolTip(QStringLiteral("修饰键过滤"));
            modifierCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            auto *modLabel = new QLabel(QStringLiteral("Mod"), this);

            main_layout->addWidget(keyList, 0, 0, 1, 3);
            main_layout->addWidget(activeCheck, 1, 0);
            main_layout->addWidget(modLabel, 1, 1);
            main_layout->addWidget(modifierCombo, 1, 2);
            main_layout->setContentsMargins(4, 2, 4, 4);
            main_layout->setHorizontalSpacing(4);
            main_layout->setVerticalSpacing(2);
            main_layout->setColumnStretch(2, 1);
            setLayout(main_layout);
            // 默认 240，可拉伸；内容最小 240，更窄时列表出横向滚动条
            resize(240, 120);
            setMinimumWidth(120);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            connect(keyList, &QListWidget::customContextMenuRequested,
                    this, &HotKeyInterface::showContextMenu);
            connect(activeCheck, &QCheckBox::toggled, this, &HotKeyInterface::onActiveToggled);

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

        QSize sizeHint() const override
        {
            return QSize(240, 120);
        }

        QSize minimumSizeHint() const override
        {
            return QSize(120, 80);
        }

        void addKey(int port = 0, const QKeySequence &key = {})
        {
            auto *listItem = new QListWidgetItem(keyList);
            auto *row = new HotKeyItem(keyList);
            row->setOutPort(port);
            if (!key.isEmpty()) {
                row->setKeySequence(key);
            }
            keyList->addItem(listItem);
            keyList->setItemWidget(listItem, row);
            listItem->setSizeHint(QSize(240, 32));

            connect(row, &HotKeyItem::configChanged, this, &HotKeyInterface::listChanged);
            if (!signalsBlocked()) {
                Q_EMIT listChanged();
            }
        }

        QVector<HotKeyItem *> items() const
        {
            QVector<HotKeyItem *> result;
            for (int i = 0; i < keyList->count(); ++i) {
                if (auto *row = itemWidgetAt(i)) {
                    result.append(row);
                }
            }
            return result;
        }

        bool isPortPressed(int port) const
        {
            for (HotKeyItem *item : items()) {
                if (item && item->outPort() == port && item->isPressed()) {
                    return true;
                }
            }
            return false;
        }

        QJsonArray exportKeys() const
        {
            QJsonArray arr;
            for (HotKeyItem *item : items()) {
                if (item) {
                    arr.append(item->toJson());
                }
            }
            return arr;
        }

        void importKeys(const QJsonArray &arr)
        {
            const bool blocked = blockSignals(true);
            keyList->clear();
            for (const QJsonValue &v : arr) {
                if (!v.isObject()) {
                    continue;
                }
                const QJsonObject obj = v.toObject();
                addKey(obj.value(QStringLiteral("port")).toInt(0),
                       QKeySequence(obj.value(QStringLiteral("value")).toString()));
            }
            blockSignals(blocked);
            Q_EMIT listChanged();
        }

        bool isActive() const { return activeCheck->isChecked(); }

        ModifierMode modifierMode() const
        {
            return static_cast<ModifierMode>(modifierCombo->currentData().toInt());
        }

        void setModifierMode(int mode)
        {
            const int idx = modifierCombo->findData(mode);
            if (idx >= 0) {
                modifierCombo->setCurrentIndex(idx);
            }
        }

    signals:
        void keyStateChanged(int port);
        void listChanged();

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override
        {
            Q_UNUSED(watched)

            if (!activeCheck->isChecked()) {
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
            for (HotKeyItem *item : items()) {
                if (!item || item->keySequence().isEmpty()) {
                    continue;
                }
                if (!keyMatches(item->keySequence(), keyEvent->key())) {
                    continue;
                }
                if (item->setPressed(pressed)) {
                    Q_EMIT keyStateChanged(item->outPort());
                }
            }

            return false;
        }

    private slots:
        void showContextMenu(const QPoint &pos)
        {
            QMenu menu(this);
            QAction *addAction = menu.addAction(QStringLiteral("Add Key"));
            QAction *deleteAction = menu.addAction(QStringLiteral("Delete Key"));
            QAction *clearAction = menu.addAction(QStringLiteral("Clear All Keys"));

            connect(addAction, &QAction::triggered, this, [this]() { addKey(); });
            connect(deleteAction, &QAction::triggered, this, &HotKeyInterface::deleteSelected);
            connect(clearAction, &QAction::triggered, this, [this]() {
                keyList->clear();
                Q_EMIT listChanged();
            });

            menu.exec(keyList->mapToGlobal(pos));
        }

        void deleteSelected()
        {
            QListWidgetItem *item = keyList->currentItem();
            if (!item) {
                return;
            }
            HotKeyItem *row = qobject_cast<HotKeyItem *>(keyList->itemWidget(item));
            const int port = row ? row->outPort() : -1;
            const bool wasPressed = row && row->isPressed();
            delete item;
            keyList->clearSelection();
            Q_EMIT listChanged();
            if (wasPressed && port >= 0) {
                Q_EMIT keyStateChanged(port);
            }
        }

        void onActiveToggled(bool active)
        {
            if (!active) {
                releaseAllPressed();
            }
        }

    private:
        HotKeyItem *itemWidgetAt(int index) const
        {
            QListWidgetItem *item = keyList->item(index);
            if (!item) {
                return nullptr;
            }
            return qobject_cast<HotKeyItem *>(keyList->itemWidget(item));
        }

        void releaseAllPressed()
        {
            QSet<int> ports;
            for (HotKeyItem *item : items()) {
                if (item && item->isPressed() && item->setPressed(false)) {
                    ports.insert(item->outPort());
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

    public:
        QGridLayout *main_layout = new QGridLayout(this);
        QListWidget *keyList = new QListWidget(this);
        QCheckBox *activeCheck = new QCheckBox(this);
        QComboBox *modifierCombo = new QComboBox(this);
    };
}
