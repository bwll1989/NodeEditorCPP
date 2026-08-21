//
// Created by Administrator on 2023/12/13.
//

#ifndef JAVASCRIPTINTERFACE_HPP
#define JAVASCRIPTINTERFACE_HPP

#include "JsCodeEditor.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPointer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace Nodes
{
    class JavaScriptInterface: public QWidget{
        Q_OBJECT
        public:
        explicit JavaScriptInterface(QWidget *parent = nullptr){
            this->setLayout(new QVBoxLayout(this));
            this->layout()->setContentsMargins(0,0,0,0);

            top_Widget->setLayout(top_layout);
            top_layout->setContentsMargins(0,0,0,0);

            QHBoxLayout *bottomLayout = new QHBoxLayout();
            bottomLayout->setContentsMargins(0,0,5,5);
            bottomLayout->setSpacing(0);

            editButton = new QToolButton();
            editButton->setIcon(QIcon(":/icons/icons/js.png"));
            editButton->setToolTip("编辑脚本");
            editButton->setFixedSize(18, 18);

            bottomLayout->addStretch();
            bottomLayout->addWidget(editButton);

            QWidget *bottomWidget = new QWidget();
            bottomWidget->setLayout(bottomLayout);
            bottomWidget->setFixedHeight(18);

            this->layout()->addWidget(top_Widget);
            this->layout()->addWidget(bottomWidget);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            connect(editButton, &QToolButton::clicked, this, &JavaScriptInterface::toggleEditorMode);
        }

        void toggleEditorMode() {
            if (!codeWidget) {
                return;
            }
            if (codeWidget->isVisible() && codeWidget->isWindow()) {
                codeWidget->raise();
                codeWidget->activateWindow();
                return;
            }

            codeWidget->setParent(nullptr);
            codeWidget->setWindowTitle("JS脚本编辑器");
            codeWidget->setWindowIcon(QIcon(":/icons/icons/js.png"));
            codeWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
            codeWidget->setAttribute(Qt::WA_DeleteOnClose, false);
            codeWidget->setAttribute(Qt::WA_QuitOnClose, false);
            codeWidget->resize(800, 600);
            codeWidget->setReadOnly(false);
            codeWidget->show();
            codeWidget->activateWindow();
            codeWidget->raise();
        }

        /**
         * 关闭独立编辑器窗口。析构中不能把窗口再挂回正在销毁的 this。
         */
        void closeEditorWindow() {
            if (!codeWidget) {
                return;
            }
            codeWidget->hide();
            codeWidget->close();
            if (codeWidget->parent() != this) {
                delete codeWidget.data();
            }
        }

        ~JavaScriptInterface() {
            closeEditorWindow();
        }

    public:
        QGridLayout *top_layout = new QGridLayout();
        QWidget *top_Widget = new QWidget();
        QToolButton *editButton; // 编辑按钮
        QPointer<JsCodeEditor> codeWidget = new JsCodeEditor("", this);
    };
}
#endif // JAVASCRIPTINTERFACE_HPP
