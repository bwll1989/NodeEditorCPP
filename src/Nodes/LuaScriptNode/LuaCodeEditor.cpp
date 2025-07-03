//
// Created by bwll1 on 2024/10/10.
//

#include "LuaCodeEditor.h"


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
#include "Qsci/qscilexerlua.h"
#include "Qsci/qsciapis.h"
static QString DEFAULT_CODE=R"(-- Write the Lua code here, and be careful not to use an endless loop like while(true).
-- function about lua read/write Node
local luaTable = {
    key1 = Node:outputCount(),
    key2 = Node:inputCount(),
    key3 = 12321,
    key4 = Node:getTableValue("In",0):toLuaTable()["default"]
}

local out= VariantMap.fromLuaTable(luaTable)
Node:setTableValue(0,out)

-- 将端口0的输入与lua表合并后从输出0输出)";
using namespace Nodes;
LuaCodeEditor::LuaCodeEditor(QWidget* parent):
        m_code(DEFAULT_CODE),
        m_setupLayout(nullptr),
        m_readOnlyCheckBox(nullptr),
        m_codeEditor(nullptr)
{
    createWidgets();
    setupWidgets();
    performConnections();
    loadCodeFromCode(m_code);
}


QString LuaCodeEditor::loadCode(QString path)
{
    QFile fl(path);

    if (!fl.open(QIODevice::ReadOnly))
    {
        return QString();
    }
    codePath=path;
    return fl.readAll();
}

void LuaCodeEditor::loadCodeFromCode(QString val) {

    m_code=val;
    m_codeEditor->setText(m_code);
}



void LuaCodeEditor::createWidgets()
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
    QsciLexerLua* textLexer = new QsciLexerLua(this);
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
    apis->add(QString("Node:"));
    apis->add(QString("outputCount()"));
    apis->add(QString("inputCount()"));
    apis->add(QString("getTableValue()"));
    apis->add(QString("toLuaTable()"));
    apis->add(QString("print()"));
    apis->add(QString("setTableValue()"));
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
    m_setupLayout->addWidget(detach);
    m_setupLayout->addWidget(m_readOnlyCheckBox);
    m_setupLayout->addWidget(run);
    // m_setupLayout->addWidget(save);
    m_setupLayout->addSpacerItem(new QSpacerItem(1, 2, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void LuaCodeEditor::setupWidgets()
{
    setWindowTitle("CodeEditor");
    loadCodeFromCode(m_code);
    
    // 设置初始焦点到编辑器
    m_codeEditor->setFocus();
}

QString LuaCodeEditor::saveCode() {
    m_code = m_codeEditor->text();
//    QFile file(codePath);
//    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//        QTextStream stream(&file);
//        stream << code;
//        file.close();
//    }
    return m_code;

}
void LuaCodeEditor::performConnections()
{

    connect(
            m_readOnlyCheckBox,
            &QCheckBox::stateChanged,
            [this](int state)
            { m_codeEditor->setReadOnly(state != 0); }
    );
    // connect(save,&QPushButton::clicked,this,&CodeEditor::saveCode);
}
void LuaCodeEditor::setReadOnly(bool readOnly)
{
    m_readOnlyCheckBox->setChecked(readOnly);
}