//
// Created by 吴斌 on 2024/1/18.
//

#ifndef NODEEDITORCPP_CURVEEDITOR_H
#define NODEEDITORCPP_CURVEEDITOR_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class CurveEditor; }
QT_END_NAMESPACE

class CurveEditor : public QWidget {
Q_OBJECT

public:
    explicit CurveEditor(QWidget *parent = nullptr);

    ~CurveEditor() override;

private:
    Ui::CurveEditor *ui;
};


#endif //NODEEDITORCPP_CURVEEDITOR_H
