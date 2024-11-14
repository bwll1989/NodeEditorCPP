//
// Created by Administrator on 2023/12/13.
//
#pragma once
#include <QCheckBox>
#include <QLCDNumber>
#include <QKeySequenceEdit>
#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QPushButton"
#include "QComboBox"
#include "QSpinBox"
#include "QLineEdit"
#include "QTextBrowser"
#include "QComboBox"
#include "QHotkey"

class HotKeyItem: public QWidget{

public:
    explicit HotKeyItem(QWidget *parent = nullptr){

//        this->setStyleSheet("QFrame{background-color:transparent}");
        boolDisplay->setCheckable(true);
        boolDisplay->setChecked(false);
        boolDisplay->setStyleSheet(boolDisplay->isChecked() ? "QPushButton{background-color: #00FF00;}" : "QPushButton{background-color: #FF0000;}");
        main_layout->addWidget(EnableButton,1);
        main_layout->addWidget(Editor,3);
        main_layout->addWidget(boolDisplay,1);
        main_layout->addWidget(resetButton,1);
        main_layout->setContentsMargins(4,2,4,4);
        this->setLayout(main_layout);
//        connect(this->EnableButton, &QCheckBox::toggled,
//                this->Editor, &QKeySequenceEdit::setEnabled);
        connect(this->hotkey, &QHotkey::activated,
                this, &HotKeyItem::valueDisplay);
        connect(this->EnableButton, &QCheckBox::toggled,
                this->hotkey, &QHotkey::setRegistered);
        connect(this->resetButton, &QPushButton::clicked,
            this,&HotKeyItem::valueReset);
        connect(this->Editor, &QKeySequenceEdit::keySequenceChanged,
                this, &HotKeyItem::setShortcut);
    }

public slots:
    void valueDisplay()
    {
        Count++;
        this->boolDisplay->setText(QString::number(Count));
        this->boolDisplay->setChecked(!boolDisplay->isChecked());
        this->boolDisplay->setStyleSheet(boolDisplay->isChecked() ? "QPushButton{background-color: #00FF00;}" : "QPushButton{background-color: #FF0000;}");
    }

    void valueReset()
    {
        Count=0;
        this->boolDisplay->setText(QString::number(Count));
        this->boolDisplay->setChecked(false);
        this->boolDisplay->setStyleSheet(boolDisplay->isChecked() ? "QPushButton{background-color: #00FF00;}" : "QPushButton{background-color: #FF0000;}");
    }

    void setShortcut(const QKeySequence &sequence)
    {

        this->hotkey->setShortcut(sequence, false);
    }


signals:

    void countChanged(int count);

    void boolChanged(bool val);

public:
    QHBoxLayout *main_layout=new QHBoxLayout(this);
    QCheckBox *EnableButton=new QCheckBox("HotKey");
    QKeySequenceEdit *Editor=new QKeySequenceEdit();
    QPushButton *boolDisplay=new QPushButton(" ");
    QPushButton *resetButton=new QPushButton("reset");
    int Count=0;
    QHotkey *hotkey=new QHotkey(this);

};

