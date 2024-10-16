//
// Created by Administrator on 2023/12/13.
//

#include "QWidget"
#include "QLabel"
#include "QHBoxLayout"

#include "CodeEditor.h"
using namespace std;
class LuaScriptInterface: public QFrame{
public:
    explicit LuaScriptInterface(QWidget *parent = nullptr){
        this->setLayout(main_layout);
        main_layout->setContentsMargins(0,0,0,0);
        main_layout->addWidget(codeWidget);
        this->setFocus();
    }
public:

    QHBoxLayout *main_layout=new QHBoxLayout(this);
    CodeEditor *codeWidget=new CodeEditor(this);

};

