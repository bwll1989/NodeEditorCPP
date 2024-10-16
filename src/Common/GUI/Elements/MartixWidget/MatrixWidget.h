//
// Created by bwll1 on 2024/10/9.
//

#ifndef NODEEDITORCPP_MATRIXWIDGET_H
#define NODEEDITORCPP_MATRIXWIDGET_H


#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QVector>
#include "Common/GUI/Elements/FaderWidget/FaderWidget.h"
#include "Eigen/Core"
class MatrixWidget : public QWidget {
Q_OBJECT

public:
    explicit MatrixWidget(int rows, int cols, QWidget *parent = nullptr);
//    void setClowns(int cols);
//    void setRows(int rows);
//    void setValue();
    void setValuesFromMatrix(const Eigen::MatrixXd& matrix);
    Eigen::MatrixXd getValuesAsMatrix();
signals:
    void valueChanged(Eigen::MatrixXd mat); // 声明值变化的信号

private slots:
    void onValueChanged(float newValue) ;
private:
    int m_rows;
    int m_cols;
    QVector<FaderWidget*> m_buttons;
    QGridLayout *m_layout;
    void createButtons() ;
    void createButtons(int w,int h) ;
};



#endif //NODEEDITORCPP_MATRIXWIDGET_H
