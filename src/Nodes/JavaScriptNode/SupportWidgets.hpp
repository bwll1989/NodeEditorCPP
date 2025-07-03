//
// Created by bwll1 on 2025/7/3.
//

#ifndef NODEEDITORCPP_SUPPORTWIDGETS_H
#define NODEEDITORCPP_SUPPORTWIDGETS_H
#include <QSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>

class SpinBox : public QSpinBox
{
    Q_OBJECT
public:
    Q_INVOKABLE SpinBox(QWidget *parent = nullptr)
        : QSpinBox(parent) {}

    Q_INVOKABLE int value() const
    {
        return QSpinBox::value();
    }
};

class Button : public QPushButton
{
    Q_OBJECT
public:
    Q_INVOKABLE Button(QWidget *parent = nullptr)
        : QPushButton(parent) {}
    Q_INVOKABLE void setText(const QString &text)
    {
        if (this->text() != text)
        {
            QPushButton::setText(text);
        }
    }
};

class VSlider : public QSlider
{
    Q_OBJECT
public:
    Q_INVOKABLE VSlider(QWidget *parent = nullptr)
        : QSlider(parent) {}
    Q_INVOKABLE int value() const
    {
        return QSlider::value();
    }
};

class HSlider : public QSlider
{
    Q_OBJECT
public:
    Q_INVOKABLE HSlider(QWidget *parent = nullptr)
        : QSlider(parent)
    {
        this->setOrientation(Qt::Horizontal);
    }
    Q_INVOKABLE int value() const
    {
        return QSlider::value();
    }
};

class Label : public QLabel
{
    Q_OBJECT
public:
    Q_INVOKABLE Label(QWidget *parent = nullptr)
        : QLabel(parent) {}
    Q_INVOKABLE QString text() const
    {
        return QLabel::text();
    }
};

class LineEdit : public QLineEdit
{
    Q_OBJECT
public:
    Q_INVOKABLE LineEdit(QWidget *parent = nullptr)
        : QLineEdit(parent) {}

    Q_INVOKABLE QString text() const
    {
        return QLineEdit::text();
    }

};

class ComboBox : public QComboBox
{

    Q_OBJECT
public:
    Q_INVOKABLE ComboBox(QWidget *parent = nullptr)
        : QComboBox(parent) {}
    Q_INVOKABLE void addItem(const QString &text)
    {
        if (this->findText(text) == -1)
        {
            QComboBox::addItem(text);
        }
    }
    Q_INVOKABLE void addItems(const QStringList &texts)
    {
        for (const QString &text : texts)
        {
            if (this->findText(text) == -1)
            {
                QComboBox::addItem(text);
            }
        }
    }
};

class CheckBox : public QCheckBox
{
    Q_OBJECT
public:
    Q_INVOKABLE CheckBox(QWidget *parent = nullptr)
        : QCheckBox(parent) {}

    Q_INVOKABLE bool isChecked() const
    {
        return QCheckBox::isChecked();
    }
};

class DoubleSpinBox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    Q_INVOKABLE DoubleSpinBox(QWidget *parent = nullptr)
        : QDoubleSpinBox(parent) {}

    Q_INVOKABLE double value() const
    {
        return QDoubleSpinBox::value();
    }
};


#endif //NODEEDITORCPP_SUPPORTWIDGETS_H
