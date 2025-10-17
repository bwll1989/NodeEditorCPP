#include "AudioAnalysisInterface.h"
#include "QtDebug"
#include <QGridLayout>

using namespace Nodes;

AudioAnalysisInterface::AudioAnalysisInterface(QWidget *parent)
    : QWidget(parent)
    , mResetButton(new QPushButton("Enable"))
{
    auto *layout = new QGridLayout(this);
    mResetButton->setCheckable(true);

    layout->addWidget(mResetButton,0,0,1,2);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->setHorizontalSpacing(0);
    layout->setVerticalSpacing(0);
}   

AudioAnalysisInterface::~AudioAnalysisInterface()
{}




