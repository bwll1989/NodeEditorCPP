#include "AudioMatrixInterface.h"
#include "QtDebug"
#include <QGridLayout>

using namespace Nodes;
AudioMatrixInterface::AudioMatrixInterface(int col,int row,QWidget *parent)
    : QWidget(parent)
    , rows(row)
    , cols(col)
    , mMatrixWidget(new MatrixWidget(row,col))
    , mResetButton(new QPushButton("Reset"))
{
    auto *layout = new QGridLayout(this);
    layout->addWidget(mMatrixWidget,0,0,rows,cols);
    for (int i = 0; i < cols; i++) {
        auto *label=new QLabel(QString("OUT%1").arg(i));
        label->setAlignment(Qt::AlignCenter);
        label->setFrameShape(QFrame::Box);
        layout->addWidget(label,rows+1,i,1,1);

    }
    layout->addWidget(mResetButton,rows+2,0,1,cols);
    setMinimumSize(200,300);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    layout->setHorizontalSpacing(0);
    layout->setVerticalSpacing(0);
    connect(mResetButton,&QPushButton::clicked,mMatrixWidget,&MatrixWidget::resetValues);
}   

AudioMatrixInterface::~AudioMatrixInterface()
{}




