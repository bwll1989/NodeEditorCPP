//
// Created by bwll1 on 2024/10/10.
//

#include "JsCodeEditor.h"


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
#include "Qsci/qscilexerjavascript.h"
#include "Qsci/qsciapis.h"

using namespace Nodes;

JsCodeEditor::JsCodeEditor(QString code,QWidget* parent):
        m_code(code),
        m_setupLayout(nullptr),
        m_readOnlyCheckBox(nullptr),
        m_codeEditor(nullptr)
{
    createWidgets();
    setupWidgets();
    performConnections();
    loadCodeFromCode(m_code);
}


QString JsCodeEditor::loadCode(QString path)
{
    QFile fl(path);

    if (!fl.open(QIODevice::ReadOnly))
    {
        return QString();
    }
    codePath=path;
    return fl.readAll();
}

void JsCodeEditor::loadCodeFromCode(QString val) {

    m_code=val;
    m_codeEditor->setText(m_code);
}



void JsCodeEditor::createWidgets()
{
    auto hBox = new QHBoxLayout(this);
    auto setupGroup = new QFrame();
    hBox->addWidget(setupGroup);
    m_setupLayout = new QVBoxLayout();
    setupGroup->setLayout(m_setupLayout);
    setupGroup->setMaximumWidth(100);
    // CodeEditor
    m_codeEditor = new QsciScintilla(this);
    hBox->addWidget(m_codeEditor);
    QsciLexerJavaScript* textLexer = new QsciLexerJavaScript(this);
    m_codeEditor->setLexer(textLexer);
    
    // 设置焦点策略，确保编辑器能够保持焦点
    m_codeEditor->setFocusPolicy(Qt::StrongFocus);
    
    //文档中出现的单词支持自动补全
    m_codeEditor->setAutoCompletionSource(QsciScintilla::AcsAll);
    //设置自动补全的匹配模式为大小写敏感
    m_codeEditor->setAutoCompletionCaseSensitivity(true);
    //设置自动补全的触发字符个数
    m_codeEditor->setAutoCompletionThreshold(1);
    // //常用关键字支持自动补全
    QsciAPIs *apis = new QsciAPIs(textLexer);
    apis->add(QString("Node.inputIndex()"));
    apis->add(QString("Node.getOutputCount()"));
    apis->add(QString("Node.getInputCount()"));
    apis->add(QString("Node.getInputValue()"));
    apis->add(QString("Node.setOutputValue()"));
    apis->add(QString("print()"));
    apis->add(QString("typeof"));
    apis->add(QString("var"));
    apis->add(QString("null"));
    apis->add(QString("new"));
    apis->add(QString("SpinBox"));
    apis->add(QString("CheckBox"));
    apis->add(QString("LineEdit"));
    apis->add(QString("Label"));
    apis->add(QString("Button"));
    apis->add(QString("VSlider"));
    apis->add(QString("HSlider"));
    apis->add(QString("DoubleSpinBox"));
    apis->add(QString("ComboBox"));
    apis->add(QString("clearLayout"));

    apis->add(QString("Node.addToLayout"));

    apis->add(QString(R"(if (condition){
        // 执行代码块 1
    }else{
        // 执行代码块 2
    })"));
    apis->add(QString(R"(switch() {
    case 1:
        // 执行代码块 1
        break;
    case 2:
        // 执行代码块 2
        break;
    default:
        // 默认代码块
})"));
    apis->add(QString("while()"));
    apis->add(QString("else"));
    apis->add(QString("for"));
    apis->add(QString("break"));
    apis->add(QString("continue"));
    apis->add(QString("function"));
    apis->add(QString("console.log()"));
    apis->prepare();

    //设置编码为UTF-8
    m_codeEditor->SendScintilla(QsciScintilla::SCI_SETCODEPAGE, QsciScintilla::SC_CP_UTF8);

    //左侧显示行号
    m_codeEditor->setMarginLineNumbers(0, true);
    m_codeEditor->setMarginWidth(0, 20);

    //TAB缩进4个字符
    m_codeEditor->setTabWidth(4);

    //当前行高亮
    m_codeEditor->setCaretLineVisible(true);
    m_codeEditor->setCaretLineBackgroundColor(QColor("#E8E8FF"));
    //折叠样式
    m_codeEditor->setFolding(QsciScintilla::BoxedTreeFoldStyle);

    m_readOnlyCheckBox           = new QCheckBox("Read Only", setupGroup);
    // auto *saveShortcutAction = new QShortcut((QKeySequence::fromString("Ctrl+S")),this);
    // auto *m_actionToggleComment =new QShortcut(QKeySequence::fromString("Ctrl+/"),this);
    // connect(saveShortcutAction , &QShortcut::activated, this, &CodeEditor::saveCode);
    m_setupLayout->addWidget(m_readOnlyCheckBox);
    m_setupLayout->addWidget(run);
    // m_setupLayout->addWidget(save);
    m_setupLayout->addSpacerItem(new QSpacerItem(1, 2, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void JsCodeEditor::setupWidgets()
{
    setWindowTitle("CodeEditor");
    loadCodeFromCode(m_code);
    
    // 设置初始焦点到编辑器
    m_codeEditor->setFocus();
}

QString JsCodeEditor::saveCode() {
    m_code = m_codeEditor->text();
//    QFile file(codePath);
//    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//        QTextStream stream(&file);
//        stream << code;
//        file.close();
//    }
    return m_code;

}
void JsCodeEditor::performConnections()
{

    connect(
            m_readOnlyCheckBox,
            &QCheckBox::stateChanged,
            [this](int state)
            { m_codeEditor->setReadOnly(state != 0); }
    );
    // connect(save,&QPushButton::clicked,this,&CodeEditor::saveCode);
}
void JsCodeEditor::setReadOnly(bool readOnly)
{
    m_readOnlyCheckBox->setChecked(readOnly);
}