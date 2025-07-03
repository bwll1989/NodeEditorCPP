//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_LUACODEEDITOR_H
#define NODEEDITORCPP_LUACODEEDITOR_H


// Qt
#include <QWidget> // Required for inheritance
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QString>
#include <QVector>
#include <QPair>
#include <QCloseEvent>
#include "QPushButton"
#include "Qsci/qsciscintilla.h"
class QVBoxLayout;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QCompleter;

namespace Nodes
{
    class LuaCodeEditor : public QWidget
    {
        Q_OBJECT

        public:

        /**
         * @brief Constructor.
         * @param parent Pointer to parent widget.
         */
        explicit LuaCodeEditor(QWidget* parent=nullptr);
        QString loadCode(QString path);
        void loadCodeFromCode(QString code);
        QPushButton *run=new QPushButton("运行");
        QPushButton *detach=new QPushButton("编辑");
        /**
         * @brief 设置窗口关闭事件处理
         * @param event 关闭事件
         */
        void closeEvent(QCloseEvent *event) override {
            // 如果是独立窗口模式，关闭时不销毁，只隐藏
            if (!parent()) {
                event->ignore();

                detach->click();
            } else {
                QWidget::closeEvent(event);
            }
        }

    public Q_SLOTS:
        QString saveCode();
        void setReadOnly(bool readOnly);
    private:
        void createWidgets();

        void setupWidgets();

        void performConnections();

        QVBoxLayout* m_setupLayout;
        QString codePath;

        QCheckBox* m_readOnlyCheckBox;
        // QCodeEditor* m_codeEditor;
        QsciScintilla *m_codeEditor;

        QString m_code;
    };
}

#endif //NODEEDITORCPP_CODEEDITOR_H
