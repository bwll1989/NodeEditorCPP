#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QSignalBlocker>

#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include "AJTProtocol.hpp"

/**
 * @brief AJT Node 界面：设备 ID + 六路调光 + 全开/全关
 */
class AJTNodeInterface : public QWidget
{
    Q_OBJECT

public:
    static constexpr int kChannelCount = Nodes::AJTProtocol::kChannelCount;

    explicit AJTNodeInterface(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(5, 5, 5, 5);
        mainLayout->setSpacing(6);

        auto idGroup = new QGroupBox(QStringLiteral("设备"), this);
        auto idLayout = new QGridLayout(idGroup);
        idLayout->setContentsMargins(6, 8, 6, 6);
        idLayout->setSpacing(4);

        idLayout->addWidget(new QLabel(QStringLiteral("设备 ID:"), this), 0, 0);
        _idEdit = new IntDragValueWidget(this);
        _idEdit->setRange(0, 255);
        _idEdit->setValue(0x15);
        idLayout->addWidget(_idEdit, 0, 1);

        mainLayout->addWidget(idGroup);

        auto channelGroup = new QGroupBox(QStringLiteral("六路调光 (0～255)"), this);
        auto channelLayout = new QGridLayout(channelGroup);
        channelLayout->setContentsMargins(6, 8, 6, 6);
        channelLayout->setSpacing(4);

        for (int i = 0; i < kChannelCount; ++i) {
            channelLayout->addWidget(new QLabel(QString("CH%1:").arg(i + 1), this), i, 0);
            _channelEdits[i] = new IntDragValueWidget(this);
            _channelEdits[i]->setRange(0, 255);
            _channelEdits[i]->setValue(0);
            channelLayout->addWidget(_channelEdits[i], i, 1);
        }
        mainLayout->addWidget(channelGroup);

        _enableButton = new QPushButton(QStringLiteral("全开/全关"), this);
        _enableButton->setCheckable(true);
        _enableButton->setChecked(false);
        mainLayout->addWidget(_enableButton);

        mainLayout->addStretch();
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumSize(220, 280);
    }

    void setDeviceId(int id)
    {
        QSignalBlocker blocker(_idEdit);
        _idEdit->setValue(id);
    }

    void setChannelLevel(int index, int level)
    {
        if (index < 0 || index >= kChannelCount) {
            return;
        }
        QSignalBlocker blocker(_channelEdits[index]);
        _channelEdits[index]->setValue(level);
    }

    void setEnableChecked(bool enabled)
    {
        QSignalBlocker blocker(_enableButton);
        _enableButton->setChecked(enabled);
    }

    IntDragValueWidget *_idEdit = nullptr;
    IntDragValueWidget *_channelEdits[kChannelCount]{};
    QPushButton *_enableButton = nullptr;
};
