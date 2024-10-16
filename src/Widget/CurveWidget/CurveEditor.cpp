//
// Created by 吴斌 on 2024/1/18.
//

// You may need to build the project (run Qt uic code generator) to get "ui_CurveEditor.h" resolved

#include "CurveEditor.h"
#include "ui_CurveEditor.h"


CurveEditor::CurveEditor(QWidget *parent) : ui(new Ui::CurveEditor) {
    ui->setupUi(this);

//    this->hide();
}

CurveEditor::~CurveEditor() {
    delete ui;
}

