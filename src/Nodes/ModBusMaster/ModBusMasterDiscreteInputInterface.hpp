#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QVector>
#include <QSignalBlocker>

#include <QComboBox>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"

/**
 * @brief 通用 Modbus 离散输入只读界面
 *
 * - 连接参数：IP / 端口 / Server ID
 * - DI 参数：数量、起始 PDU 地址
 * - 动态只读勾选框：显示各路离散输入状态
 */
class ModBusMasterDiscreteInputInterface : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kMinInputCount = 1;
    static constexpr int kMaxInputCount = 64;

    explicit ModBusMasterDiscreteInputInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(6);

        auto connectionGroup = new QGroupBox("连接设置", this);
        auto connectionLayout = new QGridLayout(connectionGroup);
        connectionLayout->setContentsMargins(6, 8, 6, 6);
        connectionLayout->setSpacing(4);

        connectionLayout->addWidget(new QLabel("IP地址:", this), 0, 0);
        _hostEdit = new QLineEdit("127.0.0.1", this);
        connectionLayout->addWidget(_hostEdit, 0, 1);

        connectionLayout->addWidget(new QLabel("端口:", this), 1, 0);
        _portEdit = new IntDragValueWidget(this);
        _portEdit->setRange(1, 65535);
        _portEdit->setValue(502);
        connectionLayout->addWidget(_portEdit, 1, 1);

        connectionLayout->addWidget(new QLabel("Server ID:", this), 2, 0);
        _serverId = new IntDragValueWidget(this);
        _serverId->setRange(0, 255);
        _serverId->setValue(1);
        connectionLayout->addWidget(_serverId, 2, 1);

        _statusLabel = new QPushButton("状态: 未连接", this);
        _statusLabel->setEnabled(false);
        _statusLabel->setCheckable(true);
        _statusLabel->setFlat(true);
        _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        connectionLayout->addWidget(_statusLabel, 3, 0, 1, 2);

        mainLayout->addWidget(connectionGroup);

        auto configGroup = new QGroupBox("离散输入配置", this);
        auto configLayout = new QGridLayout(configGroup);
        configLayout->setContentsMargins(6, 8, 6, 6);
        configLayout->setSpacing(4);

        configLayout->addWidget(new QLabel("数量:", this), 0, 0);
        _inputCountEdit = new IntDragValueWidget(this);
        _inputCountEdit->setRange(kMinInputCount, kMaxInputCount);
        _inputCountEdit->setValue(8);
        configLayout->addWidget(_inputCountEdit, 0, 1);

        configLayout->addWidget(new QLabel("地址基址:", this), 1, 0);
        _addressBaseCombo = new QComboBox(this);
        _addressBaseCombo->addItem("0 基 (PDU)", 0);
        _addressBaseCombo->addItem("1 基", 1);
        _addressBaseCombo->setCurrentIndex(0);
        configLayout->addWidget(_addressBaseCombo, 1, 1);

        _startAddressLabel = new QLabel("起始地址(PDU):", this);
        configLayout->addWidget(_startAddressLabel, 2, 0);
        _inputStartEdit = new IntDragValueWidget(this);
        _inputStartEdit->setRange(0, 65535);
        _inputStartEdit->setValue(0);
        configLayout->addWidget(_inputStartEdit, 2, 1);

        mainLayout->addWidget(configGroup);

        _inputGroup = new QGroupBox("离散输入 (DI0~)", this);
        _inputScroll = new QScrollArea(this);
        _inputScroll->setWidgetResizable(true);
        _inputScroll->setFrameShape(QFrame::NoFrame);
        _inputScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _inputScroll->setMinimumHeight(80);
        _inputScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto inputInner = new QWidget(_inputScroll);
        _inputLayout = new QGridLayout(inputInner);
        _inputLayout->setContentsMargins(6, 8, 6, 6);
        _inputLayout->setSpacing(4);
        _inputScroll->setWidget(inputInner);

        auto inputGroupLayout = new QVBoxLayout(_inputGroup);
        inputGroupLayout->setContentsMargins(4, 8, 4, 4);
        inputGroupLayout->addWidget(_inputScroll);
        _inputGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        mainLayout->addWidget(_inputGroup, /*stretch*/ 1);

        connect(_hostEdit, &QLineEdit::editingFinished, this, [this]() {
            emit hostChanged(_hostEdit->text());
        });
        connect(_portEdit, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit portChanged(value);
        });
        connect(_serverId, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit serverIdChanged(value);
        });
        connect(_inputCountEdit, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit inputCountChanged(value);
        });
        connect(_addressBaseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
            emit addressBaseChanged(_addressBaseCombo->currentData().toInt());
        });
        connect(_inputStartEdit, &IntDragValueWidget::valueChanged, this, [this](int value) {
            emit inputStartAddressChanged(value);
        });

        connectionGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        configGroup->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(240, 280);

        rebuildInputWidgets(8, 0, 0);
    }

    void updateStartAddressLabel(int addressBase)
    {
        if (_startAddressLabel) {
            _startAddressLabel->setText(addressBase == 0
                                            ? QStringLiteral("起始地址(PDU):")
                                            : QStringLiteral("起始地址(1基):"));
        }
        if (_inputStartEdit) {
            _inputStartEdit->setRange(addressBase, 65535);
        }
    }

    void rebuildInputWidgets(int count, int pduStart, int addressBase)
    {
        count = qBound(kMinInputCount, count, kMaxInputCount);
        _startPdu = pduStart;
        _addressBase = addressBase;
        updateStartAddressLabel(addressBase);

        for (QCheckBox *box : _inputCheckBoxes) {
            if (box) {
                _inputLayout->removeWidget(box);
                box->deleteLater();
            }
        }
        _inputCheckBoxes.clear();

        const int cols = 2;
        for (int i = 0; i < count; ++i) {
            const int absAddr = pduStart + i + addressBase;
            auto *box = new QCheckBox(
                QString("DI%1 [%2]").arg(i).arg(absAddr, 4, 10, QChar('0')), this);
            box->setEnabled(false);
            box->setStyleSheet("color: gray; font-weight: bold;");
            _inputLayout->addWidget(box, i / cols, i % cols);
            _inputCheckBoxes.append(box);
        }

        const int displayStart = pduStart + addressBase;
        _inputGroup->setTitle(
            QString("离散输入 (DI0~DI%1, 地址 %2~%3)")
                .arg(count - 1)
                .arg(displayStart, 4, 10, QChar('0'))
                .arg(displayStart + count - 1, 4, 10, QChar('0')));
    }

    void setInputState(int index, bool state)
    {
        if (index < 0 || index >= _inputCheckBoxes.size()) {
            return;
        }
        QCheckBox *box = _inputCheckBoxes[index];
        QSignalBlocker blocker(box);
        box->setChecked(state);
        box->setText(QString("DI%1 [%2]")
                         .arg(index)
                         .arg(_startPdu + index + _addressBase, 4, 10, QChar('0')));
        box->setStyleSheet(state
                               ? QStringLiteral("color: green; font-weight: bold;")
                               : QStringLiteral("color: gray; font-weight: bold;"));
    }

    void setConnectionStatus(bool connected)
    {
        _connected = connected;
        if (connected) {
            _statusLabel->setChecked(true);
            _statusLabel->setText("状态: 已连接");
            _statusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else {
            _statusLabel->setChecked(false);
            _statusLabel->setText("状态: 未连接");
            _statusLabel->setStyleSheet("color: red; font-weight: bold;");
        }
    }

    QVector<QCheckBox *> inputCheckBoxes() const { return _inputCheckBoxes; }

signals:
    void hostChanged(const QString &host);
    void portChanged(int port);
    void serverIdChanged(int serverId);
    void inputCountChanged(int count);
    void addressBaseChanged(int base);
    void inputStartAddressChanged(int address);

public:
    QLineEdit *_hostEdit = nullptr;
    IntDragValueWidget *_portEdit = nullptr;
    IntDragValueWidget *_serverId = nullptr;
    IntDragValueWidget *_inputCountEdit = nullptr;
    QComboBox *_addressBaseCombo = nullptr;
    QLabel *_startAddressLabel = nullptr;
    IntDragValueWidget *_inputStartEdit = nullptr;
    QPushButton *_statusLabel = nullptr;
    QGroupBox *_inputGroup = nullptr;
    QScrollArea *_inputScroll = nullptr;
    QGridLayout *_inputLayout = nullptr;
    QVector<QCheckBox *> _inputCheckBoxes;
    bool _connected = false;
    int _startPdu = 0;
    int _addressBase = 0;
};
