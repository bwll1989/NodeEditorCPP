//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include "QCheckBox"
#include <QQuickWidget>
#include <QQuickItem>
using namespace std;
namespace Nodes
{
    class QMLInterface: public QFrame{
    public:
        explicit QMLInterface(QWidget *parent = nullptr){


            //        this->setStyleSheet("QFrame{background-color:transparent}");

            QUrl source("qrc:qml/content/dynamicview.qml");
            //        m_quickWidget->resize(300,300);
            m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView );
            m_quickWidget->setSource(source);
            this->setLayout(main_layout);
            main_layout->addWidget(m_quickWidget,0,0,2,2);
        }
    public:

        QGridLayout *main_layout=new QGridLayout(this);
        QQuickWidget *m_quickWidget=new QQuickWidget();
    };
}