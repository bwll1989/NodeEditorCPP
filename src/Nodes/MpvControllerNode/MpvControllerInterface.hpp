//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QVBoxLayout>
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
// #include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
using namespace std;
using namespace NodeDataTypes;
namespace Nodes
{
    class MpvControllerInterface: public QWidget{
        Q_OBJECT
        public:
        explicit MpvControllerInterface(QWidget *parent = nullptr){
            auto *layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(6);

            const auto addRow = [this, layout](const QString& labelText, QWidget* editor) {
                auto *row = new QWidget(this);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);

                auto *label = new QLabel(labelText, row);
                rowLayout->addWidget(label, 0);
                rowLayout->addWidget(editor, 1);

                layout->addWidget(row);
            };

            {
                auto *row = new QWidget(this);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                rowLayout->addWidget(hostLabel, 0);
                rowLayout->addWidget(hostEdit, 1);
                layout->addWidget(row);
            }

            layout->addWidget(Play);

            auto *playlistRow = new QWidget(this);
            auto *playlistLayout = new QHBoxLayout(playlistRow);
            playlistLayout->setContentsMargins(0, 0, 0, 0);
            playlistLayout->setSpacing(8);
            playlistLayout->addWidget(playlist_prev);
            playlistLayout->addWidget(playlist_next);
            layout->addWidget(playlistRow);

            auto *speedRow = new QWidget(this);
            auto *speedLayout = new QHBoxLayout(speedRow);
            speedLayout->setContentsMargins(0, 0, 0, 0);
            speedLayout->setSpacing(8);
            speedLayout->addWidget(speedSub);
            speedLayout->addWidget(speedAdd);
            layout->addWidget(speedRow);

            layout->addWidget(speedReset);

            addRow("Volume:", volumeEditor);
            volumeEditor->setRange(0,150);
            volumeEditor->setSingleStep(1);
            volumeEditor->setValue(100);

            layout->addWidget(Fullscreen);
            layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        }

        signals:
            // 当 Host 或 Port 发生变化时触发
            void hostChanged(QString host);
    public slots:
        // 处理属性值变化
        void valueChanged(const QString &propertyName, const QVariant &value) {

        if ( propertyName == "Host") {
            // 获取 Host 和 Port 的最新值
            QString host = hostEdit->text();
            emit hostChanged(host);
        }
    }
    public:

        QLabel *hostLabel=new QLabel("Host: ");
        QLineEdit *hostEdit=new QLineEdit("127.0.0.1");
        QPushButton *Play=new QPushButton("play");
        QPushButton *playlist_prev=new QPushButton("playlist_prev");
        QPushButton *playlist_next=new QPushButton("playlist_next");
        QPushButton *speedAdd=new QPushButton("speedAdd");
        QPushButton *speedSub=new QPushButton("speedSub");
        QLabel *volumeLabel=new QLabel("Volume: ");
        FloatDragValueWidget *volumeEditor=new FloatDragValueWidget(this);
        QPushButton *speedReset=new QPushButton("speedReset");
        QPushButton *Fullscreen=new QPushButton("fullscreen");


    };
}
