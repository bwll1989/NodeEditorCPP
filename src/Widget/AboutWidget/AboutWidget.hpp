//
// Created by 吴斌 on 2024/1/17.
//

#pragma once

#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
class AboutWidget : public QDialog
{
public:
    AboutWidget(const QString &filePath, QWidget *parent = nullptr)
            : QDialog(parent)
    {
        setWindowTitle("关于窗口");
        QVBoxLayout *layout = new QVBoxLayout(this);

        QTextEdit *textEdit = new QTextEdit(this);
        textEdit->setReadOnly(true); // 设置为只读模式
        layout->addWidget(textEdit);

        loadTextFile(filePath, textEdit);

        setMinimumSize(640,480);
    }
private:
    void loadTextFile(const QString &filePath, QTextEdit *textEdit)
    {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QString fileContent = stream.readAll();
            textEdit->setPlainText(fileContent);
            file.close();
        } else {
            qDebug() << "Failed to open file:" << file.errorString();
        }
    }
};
