#pragma once

#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>

namespace Nodes
{
    class HttpClientInterface : public QWidget
    {
    public:
        explicit HttpClientInterface(QWidget *parent = nullptr)
        {
            setLayout(main_layout);

            method->addItem(QStringLiteral("GET"), 0);
            method->addItem(QStringLiteral("POST"), 1);
            method->addItem(QStringLiteral("PUT"), 2);
            method->addItem(QStringLiteral("DELETE"), 3);
            method->addItem(QStringLiteral("PATCH"), 4);

            baseUrlEdit->setPlaceholderText(QStringLiteral("http://127.0.0.1:8080"));
            baseUrlEdit->setText(QStringLiteral("http://127.0.0.1:8080"));
            pathEdit->setPlaceholderText(QStringLiteral("/api/resource"));
            headersEdit->setPlaceholderText(QStringLiteral("{\"Authorization\":\"Bearer token\"}"));
            bodyEdit->setPlaceholderText(QStringLiteral("{\"key\":\"value\"}"));
            bodyEdit->setMaximumBlockCount(2000);

            int row = 0;
            main_layout->addWidget(new QLabel(QStringLiteral("Base URL:")), row, 0);
            main_layout->addWidget(baseUrlEdit, row++, 1);
            main_layout->addWidget(new QLabel(QStringLiteral("Method:")), row, 0);
            main_layout->addWidget(method, row++, 1);
            main_layout->addWidget(new QLabel(QStringLiteral("Path:")), row, 0);
            main_layout->addWidget(pathEdit, row++, 1);
            main_layout->addWidget(new QLabel(QStringLiteral("Headers:")), row, 0);
            main_layout->addWidget(headersEdit, row++, 1);
            main_layout->addWidget(new QLabel(QStringLiteral("Body:")), row, 0);
            main_layout->addWidget(bodyEdit, row++, 1);
            main_layout->addWidget(statusLabel, row++, 0, 1, 2);
            main_layout->addWidget(sendButton, row++, 0, 1, 2);

            statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
            main_layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), row, 0);
            main_layout->setRowStretch(row, 1);
            main_layout->setColumnStretch(1, 1);
            // setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setFixedSize(500,300);
        }

    public:
        QGridLayout *main_layout = new QGridLayout(this);
        QLineEdit *baseUrlEdit = new QLineEdit();
        QComboBox *method = new QComboBox();
        QLineEdit *pathEdit = new QLineEdit();
        QLineEdit *headersEdit = new QLineEdit();
        QPlainTextEdit *bodyEdit = new QPlainTextEdit();
        QLabel *statusLabel = new QLabel(QStringLiteral("Ready"));
        QPushButton *sendButton = new QPushButton(QStringLiteral("Send"));
    };
}
