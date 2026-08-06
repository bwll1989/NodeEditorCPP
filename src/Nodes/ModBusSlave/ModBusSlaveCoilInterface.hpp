#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QVector>
#include <QSignalBlocker>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * @brief 通用 Modbus TCP 从机（线圈）界面 —— 纯被控
 *
 * - 监听地址 / 端口 / 从站 ID
 * - 可配置线圈数量、地址基址（0/1 基）与起始地址
 * - 线圈勾选框仅只读显示主站写入后的状态（不可本地改写）
 */
class ModBusSlaveCoilInterface : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kMinCoilCount = 1;
    static constexpr int kMaxCoilCount = 64;

    explicit ModBusSlaveCoilInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(6);

        auto connectionGroup = new QGroupBox("从机监听", this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        connectionLayout->setContentsMargins(6, 8, 6, 6);
        connectionLayout->setSpacing(4);

        connectionLayout->addWidget(new QLabel("监听地址:", this), 0, 0);
        _hostEdit = new QLineEdit("0.0.0.0", this);
        connectionLayout->addWidget(_hostEdit, 0, 1);

        connectionLayout->addWidget(new QLabel("端口:", this), 1, 0);
        _portEdit = new IntDragValueWidget(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(50001);
        connectionLayout->addWidget(_portEdit, 1, 1);

        connectionLayout->addWidget(new QLabel("从站ID:", this), 2, 0);
        _serverId = new IntDragValueWidget(this);
        _serverId->setRange(0, 255);
        _serverId->setValue(1);
        connectionLayout->addWidget(_serverId, 2, 1);

        _statusLabel = new QPushButton("状态: 未监听", this);
        _statusLabel->setEnabled(false);
        _statusLabel->setCheckable(true);
        _statusLabel->setFlat(true);
        _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        connectionLayout->addWidget(_statusLabel, 3, 0, 1, 2);

        mainLayout->addWidget(connectionGroup);

        auto coilConfigGroup = new QGroupBox("线圈配置", this);
        auto coilConfigLayout = new QGridLayout(coilConfigGroup);
        coilConfigLayout->setContentsMargins(6, 8, 6, 6);
        coilConfigLayout->setSpacing(4);

        coilConfigLayout->addWidget(new QLabel("数量:", this), 0, 0);
        _coilCountEdit = new IntDragValueWidget(this);
        _coilCountEdit->setRange(kMinCoilCount, kMaxCoilCount);
        _coilCountEdit->setValue(16);
        coilConfigLayout->addWidget(_coilCountEdit, 0, 1);

        coilConfigLayout->addWidget(new QLabel("地址基址:", this), 1, 0);
        _addressBaseCombo = new QComboBox(this);
        _addressBaseCombo->addItem("0 基 (PDU)", 0);
        _addressBaseCombo->addItem("1 基", 1);
        _addressBaseCombo->setCurrentIndex(0);
        coilConfigLayout->addWidget(_addressBaseCombo, 1, 1);

        _startAddressLabel = new QLabel("起始地址(PDU):", this);
        coilConfigLayout->addWidget(_startAddressLabel, 2, 0);
        _coilStartEdit = new IntDragValueWidget(this);
        _coilStartEdit->setRange(0, 65535);
        _coilStartEdit->setValue(1);
        coilConfigLayout->addWidget(_coilStartEdit, 2, 1);

        mainLayout->addWidget(coilConfigGroup);

        _coilGroup = new QGroupBox("线圈", this);
        _coilScroll = new QScrollArea(this);
        _coilScroll->setWidgetResizable(true);
        _coilScroll->setFrameShape(QFrame::NoFrame);
        _coilScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _coilScroll->setMinimumHeight(80);
        _coilScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto coilInner = new QWidget(_coilScroll);
        _coilLayout = new QGridLayout(coilInner);
        _coilLayout->setContentsMargins(6, 8, 6, 6);
        _coilLayout->setSpacing(4);
        _coilScroll->setWidget(coilInner);

        auto coilGroupLayout = new QVBoxLayout(_coilGroup);
        coilGroupLayout->setContentsMargins(4, 8, 4, 4);
        coilGroupLayout->addWidget(_coilScroll);
        _coilGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        mainLayout->addWidget(_coilGroup, /*stretch*/ 1);

        connect(_hostEdit, &QLineEdit::editingFinished, this, [this]() {
            emit hostChanged(_hostEdit->text());
        });
        connect(_portEdit, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit portChanged(value);
        });
        connect(_serverId, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit serverIdChanged(value);
        });
        connect(_coilCountEdit, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit coilCountChanged(value);
        });
        connect(_addressBaseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            emit addressBaseChanged(_addressBaseCombo->currentData().toInt());
        });
        connect(_coilStartEdit, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit coilStartAddressChanged(value);
        });

        connectionGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        coilConfigGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(260, 280);

        rebuildCoilWidgets(16, 1, 0);
    }

    void updateStartAddressLabel(int addressBase)
    {
        if (_startAddressLabel) {
            _startAddressLabel->setText(addressBase == 0
                                            ? QStringLiteral("起始地址(PDU):")
                                            : QStringLiteral("起始地址(1基):"));
        }
        if (_coilStartEdit) {
            _coilStartEdit->setRange(addressBase, 65535);
        }
    }

    void rebuildCoilWidgets(int count, int pduStart, int addressBase)
    {
        count = qBound(kMinCoilCount, count, kMaxCoilCount);
        _startPdu = pduStart;
        _addressBase = addressBase;
        updateStartAddressLabel(addressBase);

        for (QCheckBox *box : _coilCheckBoxes) {
            if (box) {
                _coilLayout->removeWidget(box);
                box->deleteLater();
            }
        }
        _coilCheckBoxes.clear();

        const int cols = 2;
        for (int i = 0; i < count; ++i) {
            const int absAddr = pduStart + i + addressBase;
            auto *box = new QCheckBox(
                QString("C%1 [%2]").arg(i).arg(absAddr, 4, 10, QChar('0')), this);
            // 纯被控：仅显示主站写入结果，禁止本地点击改状态
            box->setEnabled(false);
            _coilLayout->addWidget(box, i / cols, i % cols);
            _coilCheckBoxes.append(box);
        }

        const int displayStart = pduStart + addressBase;
        _coilGroup->setTitle(
            QString("线圈状态(只读) C0~C%1, 地址 %2~%3")
                .arg(count - 1)
                .arg(displayStart, 4, 10, QChar('0'))
                .arg(displayStart + count - 1, 4, 10, QChar('0')));
    }

    void setCoilState(int index, bool state)
    {
        if (index < 0 || index >= _coilCheckBoxes.size()) {
            return;
        }
        QSignalBlocker blocker(_coilCheckBoxes[index]);
        _coilCheckBoxes[index]->setChecked(state);
    }

    bool getCoilState(int index) const
    {
        if (index < 0 || index >= _coilCheckBoxes.size()) {
            return false;
        }
        return _coilCheckBoxes[index]->isChecked();
    }

    void setListeningStatus(bool listening)
    {
        if (listening) {
            _statusLabel->setChecked(true);
            _statusLabel->setText("状态: 监听中");
            _statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            _statusLabel->setChecked(false);
            _statusLabel->setText("状态: 未监听");
            _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }

    QVector<QCheckBox *> coilCheckBoxes() const { return _coilCheckBoxes; }

signals:
    void hostChanged(const QString &host);
    void portChanged(int port);
    void serverIdChanged(int serverId);
    void coilCountChanged(int count);
    void addressBaseChanged(int base);
    void coilStartAddressChanged(int address);

public:
    QLineEdit *_hostEdit = nullptr;
    IntDragValueWidget *_portEdit = nullptr;
    IntDragValueWidget *_serverId = nullptr;
    IntDragValueWidget *_coilCountEdit = nullptr;
    QComboBox *_addressBaseCombo = nullptr;
    QLabel *_startAddressLabel = nullptr;
    IntDragValueWidget *_coilStartEdit = nullptr;
    QPushButton *_statusLabel = nullptr;
    QGroupBox *_coilGroup = nullptr;
    QScrollArea *_coilScroll = nullptr;
    QGridLayout *_coilLayout = nullptr;
    QVector<QCheckBox *> _coilCheckBoxes;
    int _startPdu = 1;
    int _addressBase = 0;
};
