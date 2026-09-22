#include "WebMicInterface.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

using namespace Nodes;

WebMicInterface::WebMicInterface(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    auto *group = new QGroupBox(QStringLiteral("Web Mic"), this);
    auto *layout = new QGridLayout(group);

    portSpin = new IntDragValueWidget();
    portSpin->setRange(1, 65535);
    portSpin->setSingleStep(1);
    portSpin->setValue(kDefaultPort);
    layout->addWidget(new QLabel(QStringLiteral("Port")), 0, 0);
    layout->addWidget(portSpin, 0, 1);

    gainSpin = new FloatDragValueWidget();
    gainSpin->setRange(-60.0, 24.0);
    gainSpin->setDecimals(1);
    gainSpin->setSingleStep(0.5);
    gainSpin->setValue(0.0);
    gainSpin->setSuffix(QStringLiteral(" dB"));
    layout->addWidget(new QLabel(QStringLiteral("Gain")), 1, 0);
    layout->addWidget(gainSpin, 1, 1);

    statusLabel = new QLabel(QStringLiteral("Listening"));
    statusLabel->setWordWrap(true);
    layout->addWidget(new QLabel(QStringLiteral("Status")), 2, 0);
    layout->addWidget(statusLabel, 2, 1);

    auto *hint = new QLabel(
        QStringLiteral("固定 mono / 48 kHz / float32 PCM。网页 MIC 卡片连接本端口后按住说话。"));
    hint->setWordWrap(true);
    layout->addWidget(hint, 3, 0, 1, 2);

    mainLayout->addWidget(group);
    mainLayout->addStretch(1);
}

WebMicInterface::~WebMicInterface() = default;

void WebMicInterface::setStatusText(const QString &text)
{
    if (statusLabel) {
        statusLabel->setText(text);
    }
}
