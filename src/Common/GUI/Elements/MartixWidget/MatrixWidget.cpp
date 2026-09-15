//
// Created by bwll1 on 2024/10/9.
//

#include "MatrixWidget.h"

#include <QSignalBlocker>
#include <QSizePolicy>
#include <QtGlobal>
#include <cmath>

MatrixWidget::MatrixWidget(int rows, int cols, QWidget *parent)
    : QWidget(parent)
    , m_rows(qMax(1, rows))
    , m_cols(qMax(1, cols))
{
    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    // 间距需与 AudioMatrixInterface 轴标签布局一致，否则行列会对不齐
    m_layout->setSpacing(2);
    m_layout->setHorizontalSpacing(2);
    m_layout->setVerticalSpacing(2);
    createButtons();
}

FloatDragValueWidget* MatrixWidget::createCell()
{
    auto *button = new FloatDragValueWidget();
    button->setCompactMode(true);
    button->setDecimals(1);
    button->setSuffix(QStringLiteral(" dB"));
    button->setMinimum(m_minValue);
    button->setMaximum(m_maxValue);
    button->setValue(m_minValue);
    button->setToolTip(QStringLiteral("%1 dB").arg(m_minValue, 0, 'f', 1));
    connect(button, &FloatDragValueWidget::valueChanged, this, [this, button](double v) {
        button->setToolTip(QStringLiteral("%1 dB").arg(v, 0, 'f', 1));
        onValueChanged(v);
    });
    return button;
}

void MatrixWidget::createButtons()
{
    qDeleteAll(m_buttons);
    m_buttons.clear();

    // QGridLayout 的行列数/stretch 在删格后不会收缩，缩小端口时会留下空列把格子挤到左边。
    if (m_layout) {
        while (QLayoutItem *item = m_layout->takeAt(0)) {
            delete item;
        }
        delete m_layout;
        m_layout = nullptr;
    }

    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(2);
    m_layout->setHorizontalSpacing(2);
    m_layout->setVerticalSpacing(2);

    m_buttons.reserve(m_rows * m_cols);
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            FloatDragValueWidget *button = createCell();
            m_buttons.append(button);
            m_layout->addWidget(button, row, col);
            m_layout->setRowStretch(row, 1);
            m_layout->setColumnStretch(col, 1);
        }
    }

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void MatrixWidget::resizeMatrix(int rows, int cols)
{
    rows = qMax(1, rows);
    cols = qMax(1, cols);
    if (rows == m_rows && cols == m_cols) {
        return;
    }

    Eigen::MatrixXd oldValues = getValuesAsMatrix();

    m_rows = rows;
    m_cols = cols;
    createButtons();

    const int copyRows = qMin(rows, static_cast<int>(oldValues.rows()));
    const int copyCols = qMin(cols, static_cast<int>(oldValues.cols()));
    for (int i = 0; i < copyRows; ++i) {
        for (int j = 0; j < copyCols; ++j) {
            const int index = i * m_cols + j;
            QSignalBlocker blocker(m_buttons[index]);
            const double v = oldValues(i, j);
            m_buttons[index]->setValue(static_cast<float>(v));
            m_buttons[index]->setToolTip(QStringLiteral("%1 dB").arg(v, 0, 'f', 1));
        }
    }

    emit valueChanged(getLinearValuesAsMatrix());
    updateGeometry();
}

void MatrixWidget::setValuesFromMatrix(const Eigen::MatrixXd& matrix)
{
    if (matrix.rows() != m_rows || matrix.cols() != m_cols) {
        qWarning("Matrix size does not match the button layout!");
        return;
    }

    for (int i = 0; i < m_rows; ++i) {
        for (int j = 0; j < m_cols; ++j) {
            const int buttonIndex = i * m_cols + j;
            QSignalBlocker blocker(m_buttons[buttonIndex]);
            const double v = matrix(i, j);
            m_buttons[buttonIndex]->setValue(static_cast<float>(v));
            m_buttons[buttonIndex]->setToolTip(QStringLiteral("%1 dB").arg(v, 0, 'f', 1));
        }
    }

    emit valueChanged(getLinearValuesAsMatrix());
    update();
}

Eigen::MatrixXd MatrixWidget::getLinearValuesAsMatrix() const
{
    Eigen::MatrixXd res(m_rows, m_cols);
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            res(row, col) = std::pow(10.0, m_buttons[row * m_cols + col]->value() / 20.0);
        }
    }
    return res;
}

Eigen::MatrixXd MatrixWidget::getValuesAsMatrix() const
{
    Eigen::MatrixXd res(m_rows, m_cols);
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            res(row, col) = m_buttons[row * m_cols + col]->value();
        }
    }
    return res;
}

void MatrixWidget::onValueChanged(double newValue)
{
    Q_UNUSED(newValue);
    emit valueChanged(getLinearValuesAsMatrix());
}

int MatrixWidget::getRows() const
{
    return m_rows;
}

int MatrixWidget::getCols() const
{
    return m_cols;
}

FloatDragValueWidget *MatrixWidget::getMatrixElement(int index)
{
    if (index < 0 || index >= m_buttons.size()) {
        return nullptr;
    }
    return m_buttons[index];
}

void MatrixWidget::resetValues()
{
    for (auto *button : m_buttons) {
        QSignalBlocker blocker(button);
        button->setValue(button->minimum());
        button->setToolTip(QStringLiteral("%1 dB").arg(button->minimum(), 0, 'f', 1));
    }
    emit valueChanged(getLinearValuesAsMatrix());
}

void MatrixWidget::setIdentity()
{
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            const int index = row * m_cols + col;
            QSignalBlocker blocker(m_buttons[index]);
            const double v = (row == col) ? 0.0 : m_minValue;
            m_buttons[index]->setValue(v);
            m_buttons[index]->setToolTip(QStringLiteral("%1 dB").arg(v, 0, 'f', 1));
        }
    }
    emit valueChanged(getLinearValuesAsMatrix());
}
