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
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief dB 增益矩阵控件
 * 行 = 输入，列 = 输出；单元格为可拖拽的 dB 值（默认 -60～+20）
 */
class GUI_ELEMENTS_EXPORT MatrixWidget : public QWidget {
    Q_OBJECT

public:
    explicit MatrixWidget(int rows, int cols, QWidget *parent = nullptr);

    /**
     * @brief 调整矩阵行列，重叠区域保留原增益，新增格为最小值（静音）
     */
    void resizeMatrix(int rows, int cols);

    void setValuesFromMatrix(const Eigen::MatrixXd& matrix);
    Eigen::MatrixXd getLinearValuesAsMatrix() const;
    Eigen::MatrixXd getValuesAsMatrix() const;
    FloatDragValueWidget* getMatrixElement(int index);
    int getRows() const;
    int getCols() const;

signals:
    void valueChanged(Eigen::MatrixXd mat);

public slots:
    void onValueChanged(double newValue);
    void resetValues();
    /** 对角线 0 dB，其余静音（1:1 直通） */
    void setIdentity();

private:
    void createButtons();
    FloatDragValueWidget* createCell();

    int m_rows = 0;
    int m_cols = 0;
    QVector<FloatDragValueWidget*> m_buttons;
    QGridLayout *m_layout = nullptr;
    float m_minValue = -60.0f;
    float m_maxValue = 20.0f;
};
