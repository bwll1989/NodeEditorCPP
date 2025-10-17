//
// Created by bwll1 on 2024/10/10.
//

#pragma once
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
    /**
     * @brief Class, that describes demo main window.
     */
    class PythonCodeEditor : public QWidget
    {
        Q_OBJECT

        public:

        /**
         * @brief Constructor.
         * @param parent Pointer to parent widget.
         */
        explicit PythonCodeEditor(QString code="",QWidget* parent=nullptr);
        QString loadCode(QString path);
        void loadCodeFromCode(QString code);
        QPushButton *importJS=new QPushButton("保存");
        QPushButton *updateUI=new QPushButton("更新UI");
        QPushButton *exportJS=new QPushButton("导出js节点");

    public Q_SLOTS:
        QString saveCode();
        void setReadOnly(bool readOnly);
        void exportCode();
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
