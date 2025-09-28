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
            : QWidget(parent),
        m_rows(rows),
        m_cols(cols) {
        // 创建布局管理器
        m_layout = new QGridLayout(this);
        // 创建按钮矩阵
        createButtons();
        // 设置布局
        setLayout(m_layout);
        m_layout->setContentsMargins(0,0,0,0);
        m_layout->setSpacing(0);
        m_layout->setHorizontalSpacing(0);
        m_layout->setVerticalSpacing(0);

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

// 使用 Eigen 矩阵批量设置按钮值
void MatrixWidget::setValuesFromMatrix(const Eigen::MatrixXd& matrix) {
     // 立即创建本地副本
    Eigen::MatrixXd localMatrix = matrix;
    
    int matrixRows = m_rows;
    int matrixCols = m_cols;

    if (localMatrix.rows() != matrixRows || localMatrix.cols() != matrixCols) {
        qWarning("Matrix size does not match the button layout!");
        return;
    }

    // 使用本地副本进行所有操作
    for (int i = 0; i < matrixRows; ++i) {
        for (int j = 0; j < matrixCols; ++j) {
            float value = static_cast<float>(localMatrix(i, j));
            int buttonIndex = i * matrixCols + j;
            if (buttonIndex < m_buttons.size()) {
                m_buttons[buttonIndex]->setValue(value);
            }
        }
    }
    
    emit valueChanged(getLinearValuesAsMatrix());
    update();
}

// 获取按钮的值并返回 Eigen 矩阵
Eigen::MatrixXd MatrixWidget::getLinearValuesAsMatrix() {
    int rows = m_layout->rowCount();
    int cols = m_layout->columnCount();
    Eigen::MatrixXd res(rows, cols);
    // 遍历按钮并获取每个按钮的值
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            res(row, col) = m_buttons[row * cols + col]->getLinearValue(); // 获取按钮的 value 并存入矩阵
        }
    }
    return res;
}

// 获取按钮的值并返回 Eigen 矩阵
Eigen::MatrixXd MatrixWidget::getValuesAsMatrix() {
    int rows = m_layout->rowCount();
    int cols = m_layout->columnCount();
    Eigen::MatrixXd res(rows, cols);
    // 遍历按钮并获取每个按钮的值
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            res(row, col) = m_buttons[row * cols + col]->getValue(); // 获取按钮的 value 并存入矩阵
        }
    }
    return res;
}
void MatrixWidget::onValueChanged(float newValue) {
    emit valueChanged(getLinearValuesAsMatrix()); // 将按钮值变化的信号传递出去
}

int MatrixWidget::getRows() const {
    return m_rows;
}
int MatrixWidget::getCols() const {
    return m_cols;
}

FaderWidget *MatrixWidget::getMatrixElement(int index) {
    return m_buttons[index];
}

void MatrixWidget::resetValues() {
    for (auto& button : m_buttons) {
        button->setValue(button->getMinValue());
    }
}