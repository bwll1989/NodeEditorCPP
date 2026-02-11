//
// Created by bwll1 on 2024/10/9.
//

#pragma once


#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QVector>
#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"
#include "Eigen/Core"
#include <QDial>
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif
class GUI_ELEMENTS_EXPORT MatrixWidget : public QWidget {
Q_OBJECT

public:
    explicit MatrixWidget(int rows, int cols, QWidget *parent = nullptr);
//    void setClowns(int cols);
//    void setRows(int rows);
//    void setValue();
    void setValuesFromMatrix(const Eigen::MatrixXd& matrix);
    Eigen::MatrixXd getLinearValuesAsMatrix();
    Eigen::MatrixXd getValuesAsMatrix();
    FloatDragValueWidget* getMatrixElement(int index);
    int getRows() const;
    int getCols() const;
signals:
    void valueChanged(Eigen::MatrixXd mat); // 声明值变化的信号

public slots:
    void onValueChanged(float newValue) ;
    void resetValues();
private:
    int m_rows;
    int m_cols;
    QVector<FloatDragValueWidget*> m_buttons;
    QGridLayout *m_layout;
    float m_minValue=-60;
    float m_maxValue=20;
    void createButtons() ;
};

