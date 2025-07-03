//
// Created by bwll1 on 2024/10/10.
//

#ifndef NODEEDITORCPP_CODEEDITOR_H
#define NODEEDITORCPP_CODEEDITOR_H


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
    class JsCodeEditor : public QWidget
    {
        Q_OBJECT

        public:

        /**
         * @brief Constructor.
         * @param parent Pointer to parent widget.
         */
        explicit JsCodeEditor(QString code="",QWidget* parent=nullptr);
        QString loadCode(QString path);
        void loadCodeFromCode(QString code);
        QPushButton *run=new QPushButton("运行");
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
