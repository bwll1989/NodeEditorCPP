//
// Created by bwll1 on 2024/10/9.
//

#include "MatrixWidget.h"
#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QVector>
#include "Common/GUI/Elements/FaderWidget/FaderWidget.h"
#include "Eigen/Core"
MatrixWidget::MatrixWidget(int rows, int cols, QWidget *parent)
            : QWidget(parent), m_rows(rows), m_cols(cols) {
        // 创建布局管理器
        m_layout = new QGridLayout(this);
        // 创建按钮矩阵
        createButtons();
        // 设置布局
        setLayout(m_layout);
        m_layout->setHorizontalSpacing(1);
        m_layout->setVerticalSpacing(1);
    }

void MatrixWidget::createButtons() {
    // 清除旧按钮
    qDeleteAll(m_buttons);
    m_buttons.clear();
    // 按行列创建按钮
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            FaderWidget *button = new FaderWidget();
            m_buttons.append(button);
            m_layout->addWidget(button, row, col); // 将按钮添加到网格布局中
            connect(button, &FaderWidget::valueChanged, this, &MatrixWidget::onValueChanged);
        }
    }
}

void MatrixWidget::createButtons(int w,int h) {
    // 清除旧按钮
    qDeleteAll(m_buttons);
    m_buttons.clear();
    // 按行列创建按钮
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            FaderWidget *button = new FaderWidget();
            button->setFixedSize(w,h);
            m_buttons.append(button);
            m_layout->addWidget(button, row, col); // 将按钮添加到网格布局中
            connect(button, &FaderWidget::valueChanged, this, &MatrixWidget::onValueChanged);
        }
    }

}
// 使用 Eigen 矩阵批量设置按钮值
void MatrixWidget::setValuesFromMatrix(const Eigen::MatrixXd& matrix) {
    int rows = m_layout->rowCount();
    int cols = m_layout->columnCount();

    // 确保矩阵大小和按钮矩阵匹配
    if (matrix.rows() != rows || matrix.cols() != cols) {
        qWarning("Matrix size does not match the button layout!");
        return;
    }

    // 根据矩阵中的值设置按钮的 value
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            float value = static_cast<float>(matrix(row, col)); // 获取矩阵中的值
            m_buttons[row * cols + col]->setValue(value); // 设置按钮的 value
        }
    }
}

// 获取按钮的值并返回 Eigen 矩阵
Eigen::MatrixXd MatrixWidget::getValuesAsMatrix() {
    int rows = m_layout->rowCount();
    int cols = m_layout->columnCount();
    Eigen::MatrixXd matrix(rows, cols);
    // 遍历按钮并获取每个按钮的值
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            matrix(row, col) = m_buttons[row * cols + col]->getValue(); // 获取按钮的 value 并存入矩阵
        }
    }
    return matrix;
}

void MatrixWidget::onValueChanged(float newValue) {
    emit valueChanged(getValuesAsMatrix()); // 将按钮值变化的信号传递出去
}