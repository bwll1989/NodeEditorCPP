#pragma once

#include <QCheckBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"

namespace Nodes
{

/**
 * @brief Libretro 模拟器节点嵌入面板（GBA 专用精简版）
 *
 * Core 固定为随主程序分发的 mgba_libretro.dll（不可更换）；
 * ROM 从媒体库 Game 分类选择；按键按 GBA 实机布局（D-pad + A/B + L/R + Start/Select）。
 * 每个按键同时映射到独立输入端口（电平语义，按住为 true）。
 */
class LibretroInterface : public QGroupBox
{
    Q_OBJECT

public:
    explicit LibretroInterface(QWidget* parent = nullptr)
        : QGroupBox(QStringLiteral("Libretro (GBA)"), parent)
    {
        m_activeCheck = new QCheckBox(QStringLiteral("Active"), this);

        m_romSelector = new SelectorComboBox(MediaLibrary::Category::Game, this);
        m_romSelector->setPlaceholderText(QStringLiteral("从媒体库选择 .gba"));
        m_romSelector->setMinimumWidth(180);

        m_volumeSlider = new QSlider(Qt::Horizontal, this);
        m_volumeSlider->setRange(0, 100);
        m_volumeSlider->setValue(80);

        // ---- 手柄按键（GBA 实机布局，按住生效）----
        auto makePadButton = [this](const QString& text, int w = 40) {
            QPushButton* btn = new QPushButton(text, this);
            btn->setFixedSize(w, 26);
            btn->setFocusPolicy(Qt::NoFocus);
            return btn;
        };
        m_lBtn = makePadButton(QStringLiteral("L"), 56);
        m_rBtn = makePadButton(QStringLiteral("R"), 56);
        m_upBtn = makePadButton(QStringLiteral("↑"));
        m_downBtn = makePadButton(QStringLiteral("↓"));
        m_leftBtn = makePadButton(QStringLiteral("←"));
        m_rightBtn = makePadButton(QStringLiteral("→"));
        m_aBtn = makePadButton(QStringLiteral("A"));
        m_bBtn = makePadButton(QStringLiteral("B"));
        m_startBtn = makePadButton(QStringLiteral("Start"), 56);
        m_selectBtn = makePadButton(QStringLiteral("Select"), 56);

        // D-pad 十字（左）
        auto* dpad = new QGridLayout();
        dpad->setSpacing(3);
        dpad->addWidget(m_upBtn, 0, 1);
        dpad->addWidget(m_leftBtn, 1, 0);
        dpad->addWidget(m_downBtn, 1, 1);
        dpad->addWidget(m_rightBtn, 1, 2);

        // A/B 错位（右，A 上 B 左下，同实机）
        auto* abBtns = new QGridLayout();
        abBtns->setSpacing(3);
        abBtns->addWidget(m_aBtn, 0, 1);
        abBtns->addWidget(m_bBtn, 1, 0);

        auto* padRow = new QHBoxLayout();
        padRow->setSpacing(12);
        padRow->addLayout(dpad);
        padRow->addStretch(1);
        padRow->addLayout(abBtns);

        auto* shoulderRow = new QHBoxLayout();
        shoulderRow->addWidget(m_lBtn);
        shoulderRow->addStretch(1);
        shoulderRow->addWidget(m_rBtn);

        auto* sysRow = new QHBoxLayout();
        sysRow->addWidget(m_selectBtn);
        sysRow->addStretch(1);
        sysRow->addWidget(m_startBtn);

        // ---- 动作与状态 ----
        m_resetBtn = new QPushButton(QStringLiteral("Reset"), this);
        m_saveBtn = new QPushButton(QStringLiteral("Save State"), this);
        m_loadBtn = new QPushButton(QStringLiteral("Load State"), this);

        auto* actionRow = new QHBoxLayout();
        actionRow->addWidget(m_resetBtn);
        actionRow->addWidget(m_saveBtn);
        actionRow->addWidget(m_loadBtn);

        m_stateLabel = new QLabel(QStringLiteral("状态: 未启动"), this);
        m_stateLabel->setWordWrap(true);
        m_stateLabel->setStyleSheet(QStringLiteral("color: #c44; font-weight: bold;"));

        auto* mainForm = new QFormLayout();
        mainForm->setContentsMargins(0, 0, 0, 0);
        mainForm->setSpacing(4);
        mainForm->addRow(QStringLiteral("ROM"), m_romSelector);
        mainForm->addRow(QStringLiteral("Volume"), m_volumeSlider);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(6, 8, 6, 6);
        layout->setSpacing(6);
        layout->addWidget(m_activeCheck);
        layout->addLayout(mainForm);
        layout->addLayout(shoulderRow);
        layout->addLayout(padRow);
        layout->addLayout(sysRow);
        layout->addLayout(actionRow);
        layout->addWidget(m_stateLabel);

        setMinimumSize(300, 380);
        setMaximumWidth(420);

        connect(m_activeCheck, &QCheckBox::toggled, this, &LibretroInterface::activeChanged);
        connect(m_romSelector, &SelectorComboBox::selectionChanged, this,
                &LibretroInterface::romFileChanged);
        // int 编码量必须 lambda 归一化，严禁 valueChanged(int) 直连 double setter
        connect(m_volumeSlider, &QSlider::valueChanged, this,
                [this](int v) { emit volumeChanged(v / 100.0); });
        connect(m_resetBtn, &QPushButton::clicked, this, &LibretroInterface::resetRequested);
        connect(m_saveBtn, &QPushButton::clicked, this, &LibretroInterface::saveStateRequested);
        connect(m_loadBtn, &QPushButton::clicked, this, &LibretroInterface::loadStateRequested);

        // 手柄按键 → 按位掩码（bit 定义 = RETRO_DEVICE_ID_JOYPAD_*）
        auto hold = [this](QPushButton* btn, uint16_t bit) {
            connect(btn, &QPushButton::pressed, this, [this, bit]() {
                m_localMask |= bit;
                emit joypadMaskChanged(m_localMask);
            });
            connect(btn, &QPushButton::released, this, [this, bit]() {
                m_localMask &= uint16_t(~bit);
                emit joypadMaskChanged(m_localMask);
            });
        };
        hold(m_upBtn, 1u << 4);       // UP
        hold(m_downBtn, 1u << 5);     // DOWN
        hold(m_leftBtn, 1u << 6);     // LEFT
        hold(m_rightBtn, 1u << 7);    // RIGHT
        hold(m_aBtn, 1u << 8);        // A
        hold(m_bBtn, 1u << 0);        // B
        hold(m_lBtn, 1u << 10);       // L
        hold(m_rBtn, 1u << 11);       // R
        hold(m_startBtn, 1u << 3);    // START
        hold(m_selectBtn, 1u << 2);   // SELECT
    }

public slots:
    void setActiveChecked(bool active)
    {
        QSignalBlocker b(m_activeCheck);
        m_activeCheck->setChecked(active);
    }

    void setRomFile(const QString& name)
    {
        // 模型→UI 推送：阻断 selectionChanged 回环
        QSignalBlocker b(m_romSelector);
        m_romSelector->setCurrentValue(name);
    }

    void setVolumeScaled(int percent)
    {
        QSignalBlocker b(m_volumeSlider);
        m_volumeSlider->setValue(qBound(0, percent, 100));
    }

    void setStatus(const QString& text, bool ok)
    {
        m_stateLabel->setText(text);
        m_stateLabel->setStyleSheet(ok
            ? QStringLiteral("color: #2a7; font-weight: bold;")
            : QStringLiteral("color: #c44; font-weight: bold;"));
    }

signals:
    void activeChanged(bool active);
    void romFileChanged(const QString& fileName);
    void volumeChanged(double volume);
    void joypadMaskChanged(quint16 mask);
    void resetRequested();
    void saveStateRequested();
    void loadStateRequested();

public:
    /// OSC 写入的按键位合并进面板本地掩码（面板按键与 OSC 并存）
    void applyExternalButton(int bit, bool pressed)
    {
        if (bit < 0 || bit > 15) {
            return;
        }
        if (pressed) {
            m_oscMask |= uint16_t(1u << bit);
        } else {
            m_oscMask &= uint16_t(~(1u << bit));
        }
        emit joypadMaskChanged(uint16_t(m_localMask | m_oscMask));
    }

    /// 停止模拟时清空面板/OSC 按键掩码，防止重启后旧按键状态"卡住"
    void clearPadState()
    {
        m_localMask = 0;
        m_oscMask = 0;
        emit joypadMaskChanged(uint16_t(0));
    }

    QCheckBox* m_activeCheck = nullptr;
    SelectorComboBox* m_romSelector = nullptr;
    QSlider* m_volumeSlider = nullptr;
    QPushButton* m_upBtn = nullptr;
    QPushButton* m_downBtn = nullptr;
    QPushButton* m_leftBtn = nullptr;
    QPushButton* m_rightBtn = nullptr;
    QPushButton* m_aBtn = nullptr;
    QPushButton* m_bBtn = nullptr;
    QPushButton* m_lBtn = nullptr;
    QPushButton* m_rBtn = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_selectBtn = nullptr;
    QPushButton* m_resetBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_loadBtn = nullptr;
    QLabel* m_stateLabel = nullptr;

    uint16_t m_localMask = 0;   // 面板按键
    uint16_t m_oscMask = 0;     // OSC 外部控制按键
};

} // namespace Nodes
