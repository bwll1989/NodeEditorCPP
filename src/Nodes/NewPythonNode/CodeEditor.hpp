#pragma once

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
//    QString loadCodeFromPath(QString path);
    void loadCodeFromCode(QString code);
    QPushButton *run=new QPushButton("运行");
public Q_SLOTS:
    QString saveCode();
    QString getCode();

private:

    void loadStyle(QString path);

    void initData();

    void createWidgets();

    void setupWidgets();

    void performConnections();

    QVBoxLayout* m_setupLayout;
    QString code="# 导入内建模块\n"
                 "import ScriptEngine as SE\n"
                 "# 创建一个类\n"
                 "class MyClass:\n"
                 "    def __init__(self, value):\n"
                 "        self.value = value\n"
                 "\n"
                 "    def display_value(self):\n"
                 "        print(self.value)\n"
                 "\n"
                 "# 创建一个类的实例\n"
                 "obj = MyClass(42)\n"
                 "\n"
                 "def EventHandler(input={}):\n"
                 "# 入口函数\n"
                 "    out={}\n"
                 "    for k in range(2):\n"
                 "        out[k]=SE.add(k,1)\n"
                 "    return out";
    QComboBox* m_styleCombobox;
    QCheckBox* m_readOnlyCheckBox;
    QCheckBox* m_wordWrapCheckBox;
    QCheckBox* m_tabReplaceEnabledCheckbox;
    QSpinBox*  m_tabReplaceNumberSpinbox;
    QCheckBox* m_autoIndentationCheckbox;

    QCodeEditor* m_codeEditor;

    QVector<QPair<QString, QSyntaxStyle*>> m_styles;
};

