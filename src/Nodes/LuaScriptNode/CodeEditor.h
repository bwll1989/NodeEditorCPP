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
#include "QPushButton"
class QVBoxLayout;
class QSyntaxStyle;
class QComboBox;
class QCheckBox;
class QSpinBox;
class QCompleter;
class QStyleSyntaxHighlighter;
class QCodeEditor;

/**
 * @brief Class, that describes demo main window.
 */
class CodeEditor : public QWidget
{
Q_OBJECT

public:

    /**
     * @brief Constructor.
     * @param parent Pointer to parent widget.
     */
    explicit CodeEditor(QWidget* parent=nullptr);
    QString loadCode(QString path);
    void loadCodeFromCode(QString code);
    QPushButton *save=new QPushButton("保存");
    QPushButton *run=new QPushButton("运行");
public Q_SLOTS:
    QString saveCode();

private:

    void loadStyle(QString path);

    void initData();

    void createWidgets();

    void setupWidgets();

    void performConnections();

    QVBoxLayout* m_setupLayout;
    QString codePath;
    QComboBox* m_styleCombobox;
    QCheckBox* m_readOnlyCheckBox;
    QCheckBox* m_wordWrapCheckBox;
    QCheckBox* m_tabReplaceEnabledCheckbox;
    QSpinBox*  m_tabReplaceNumberSpinbox;
    QCheckBox* m_autoIndentationCheckbox;

    QCodeEditor* m_codeEditor;

    QVector<QPair<QString, QSyntaxStyle*>> m_styles;
    QString code;
};


#endif //NODEEDITORCPP_CODEEDITOR_H
