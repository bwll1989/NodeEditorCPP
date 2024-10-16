// Demo
#include "CodeEditor.hpp"

// QCodeEditor

#include <QCodeEditor>
#include <QGLSLCompleter>
#include <QLuaCompleter>
#include <QPythonCompleter>
#include <QSyntaxStyle>
#include <QCXXHighlighter>
#include <QGLSLHighlighter>
#include <QXMLHighlighter>
#include <QJavaHighlighter>
#include <QJSHighlighter>
#include <QJSONHighlighter>
#include <QLuaHighlighter>
#include <QPythonHighlighter>
#include "QFrame"
#include <QShortcut>
// Qt
#include<QFile>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

CodeEditor::CodeEditor(QWidget* parent) :

    m_setupLayout(nullptr),
    m_styleCombobox(nullptr),
    m_readOnlyCheckBox(nullptr),
    m_wordWrapCheckBox(nullptr),
    m_tabReplaceEnabledCheckbox(nullptr),
    m_tabReplaceNumberSpinbox(nullptr),
    m_autoIndentationCheckbox(nullptr),
    m_codeEditor(nullptr),
    m_styles()
{
    initData();
    createWidgets();
    performConnections();
    loadCodeFromCode(code);
}

void CodeEditor::initData()
{
    m_styles = {
        {"Default", QSyntaxStyle::defaultStyle()}
    };

    // Loading styles
    loadStyle(":/styles/drakula.xml");
}

//QString CodeEditor::loadCodeFromPath(QString path)
//{
//    QFile fl(path);
//
//    if (!fl.open(QIODevice::ReadOnly))
//    {
//        return QString();
//    }
//    codePath=path;
//    return fl.readAll();
//}

void CodeEditor::loadCodeFromCode(QString val) {

    code=val;
    m_codeEditor->setPlainText  (code);

}

void CodeEditor::loadStyle(QString path)
{
    QFile fl(path);

    if (!fl.open(QIODevice::ReadOnly))
    {
        return;
    }

    auto style = new QSyntaxStyle(this);

    if (!style->load(fl.readAll()))
    {
        delete style;
        return;
    }

    m_styles.append({style->name(), style});
}
//初始化控件
void CodeEditor::createWidgets()
{

    auto hBox = new QHBoxLayout(this);
    auto setupGroup = new QFrame();
    hBox->addWidget(setupGroup);
    m_setupLayout = new QVBoxLayout();
    setupGroup->setLayout(m_setupLayout);
    setupGroup->setMaximumWidth(300);
    // CodeEditor
    m_codeEditor = new QCodeEditor(this);
    hBox->addWidget(m_codeEditor);

    m_styleCombobox       = new QComboBox(setupGroup);
    QStringList list;
    // 样式选项
    for (auto&& el : m_styles)
    {
        list << el.first;
    }
    m_styleCombobox->addItems(list);
    list.clear();

    m_wordWrapCheckBox           = new QCheckBox("Word Wrap", setupGroup);
    m_tabReplaceEnabledCheckbox  = new QCheckBox("Tab Replace", setupGroup);
    m_tabReplaceNumberSpinbox    = new QSpinBox(setupGroup);
    m_autoIndentationCheckbox    = new QCheckBox("Auto Indentation", setupGroup);
    m_readOnlyCheckBox           = new QCheckBox("Read Only", setupGroup);
    auto *saveShortcutAction = new QShortcut((QKeySequence::fromString("Ctrl+S")),this);
    auto *m_actionToggleComment =new QShortcut(QKeySequence::fromString("Ctrl+/"),this);


    connect(m_actionToggleComment , &QShortcut::activated, m_codeEditor, &QCodeEditor::toggleComment);
    connect(saveShortcutAction , &QShortcut::activated, this, &CodeEditor::saveCode);
    m_setupLayout->addWidget(new QLabel(tr("Style"), setupGroup));
    m_setupLayout->addWidget(m_styleCombobox);
    m_setupLayout->addWidget(m_readOnlyCheckBox);
    m_setupLayout->addWidget(m_wordWrapCheckBox);
    m_setupLayout->addWidget(m_tabReplaceEnabledCheckbox);
    m_setupLayout->addWidget(m_tabReplaceNumberSpinbox);
    m_setupLayout->addWidget(m_autoIndentationCheckbox);
    m_setupLayout->addWidget(run);

    m_setupLayout->addSpacerItem(new QSpacerItem(1, 2, QSizePolicy::Minimum, QSizePolicy::Expanding));
    m_codeEditor->setSyntaxStyle(m_styles[0].second);
//    m_codeEditor->setCompleter  (new QLuaCompleter);
    m_codeEditor->setHighlighter(new QPythonHighlighter);

//    m_codeEditor->squiggle(QCodeEditor::SeverityLevel::Warning, {3,2}, {13,5}, "unused variable");
//    m_codeEditor->squiggle(QCodeEditor::SeverityLevel::Error, {7,0}, {8,0}, "Big error");


    m_codeEditor->clearSquiggle();



    m_tabReplaceEnabledCheckbox->setChecked(m_codeEditor->tabReplace());
    m_tabReplaceNumberSpinbox->setValue(m_codeEditor->tabReplaceSize());
    m_tabReplaceNumberSpinbox->setSuffix(tr(" spaces"));
    m_autoIndentationCheckbox->setChecked(m_codeEditor->autoIndentation());
    m_wordWrapCheckBox->setChecked(m_codeEditor->wordWrapMode() != QTextOption::NoWrap);
}

QString CodeEditor::saveCode() {
    QString code = m_codeEditor->toPlainText();
//    QFile file(codePath);
//    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//        QTextStream stream(&file);
//        stream << code;
//        file.close();
//    }
    return code;
    emit run->clicked(true);
}
QString CodeEditor::getCode() {
    QString code = m_codeEditor->toPlainText();
    return code;
}
void CodeEditor::performConnections()
{

    connect(
        m_styleCombobox,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int index)
        { m_codeEditor->setSyntaxStyle(m_styles[index].second); }
    );

    connect(
        m_wordWrapCheckBox,
        &QCheckBox::stateChanged,
        [this](int state)
        {
            if (state != 0)
            {
                m_codeEditor->setWordWrapMode(QTextOption::WordWrap);
            }
            else
            {
                m_codeEditor->setWordWrapMode(QTextOption::NoWrap);
            }
        }
    );

    connect(
        m_tabReplaceEnabledCheckbox,
        &QCheckBox::stateChanged,
        [this](int state)
        { m_codeEditor->setTabReplace(state != 0); }
    );

    connect(
        m_tabReplaceNumberSpinbox,
        QOverload<int>::of(&QSpinBox::valueChanged),
        [this](int value)
        { m_codeEditor->setTabReplaceSize(value); }
    );

    connect(
        m_autoIndentationCheckbox,
        &QCheckBox::stateChanged,
        [this](int state)
        { m_codeEditor->setAutoIndentation(state != 0); }
    );
    connect(
            m_readOnlyCheckBox,
            &QCheckBox::stateChanged,
            [this](int state)
            { m_codeEditor->setReadOnly(state != 0); }
    );

}

