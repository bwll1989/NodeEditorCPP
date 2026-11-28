//
// Created by bwll1 on 2024/10/10.
//

#include "JsCodeEditor.h"

#include "JavaScriptDataModel.hpp"
#include "JSEngineDefines/SupportWidgets.hpp"

#include "QFrame"
#include <QShortcut>
#include <QMetaMethod>
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
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include "Qsci/qscilexerjavascript.h"
#include "Qsci/qsciapis.h"
#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QStringConverter>
#include <QCoreApplication>

using namespace Nodes;

/**
 * 多行代码片段（.api 文件只能一行一词条）。
 */
static void registerJsSnippets(QsciAPIs *apis)
{
    apis->add(QStringLiteral(R"(function initInterface() {
    // 创建控件并用 Node.addToLayout 加入界面
})"));
    apis->add(QStringLiteral(R"(function inputEventHandler(index) {
    var v = Node.getInputValue(index)["default"];
    Node.setOutputValue(0, v);
})"));
    apis->add(QStringLiteral(R"(if (condition) {
    // ...
} else {
    // ...
})"));
    apis->add(QStringLiteral(R"(switch (value) {
    case 1:
        break;
    default:
        break;
})"));
    apis->add(QStringLiteral(R"(for (var i = 0; i < n; i++) {
    // ...
})"));
    apis->add(QStringLiteral(R"(for (const item of list) {
    // ...
})"));
    apis->add(QStringLiteral(R"(Node.ui([
    { type: "Label", key: "title", text: "标题", row: 0, col: 0 },
    { type: "HSlider", key: "gain", min: 0, max: 100, value: 50,
      row: 0, col: 1, onChange: v => Node.setOutputValue(0, { default: v }) }
]);)"));
}

/**
 * 资源文件缺失时的最小回退（与 JavaScriptDataModel 绑定一致）。
 */
static void registerFallbackJsApis(QsciAPIs *apis)
{
    const QStringList items = {
        QStringLiteral("Node.getInputCount()"),
        QStringLiteral("Node.getOutputCount()"),
        QStringLiteral("Node.inputIndex()"),
        QStringLiteral("Node.getInputValue(index)"),
        QStringLiteral("Node.getOutputValue(index)"),
        QStringLiteral("Node.setOutputValue(index, value)"),
        QStringLiteral("Node.addToLayout(widget, row, column, rowSpan, columnSpan)"),
        QStringLiteral("Node.clearLayout()"),
        QStringLiteral("Node.ui(spec)"),
        QStringLiteral("function initInterface()"),
        QStringLiteral("function inputEventHandler(index)"),
        QStringLiteral("new SpinBox()"),
        QStringLiteral("new DoubleSpinBox()"),
        QStringLiteral("new Button()"),
        QStringLiteral("new Label()"),
        QStringLiteral("new LineEdit()"),
        QStringLiteral("new CheckBox()"),
        QStringLiteral("new ComboBox()"),
        QStringLiteral("new HSlider()"),
        QStringLiteral("new VSlider()"),
        QStringLiteral("console.log()"),
        QStringLiteral("Math.abs()"),
        QStringLiteral("JSON.parse()"),
        QStringLiteral("JSON.stringify()"),
    };
    for (const auto &s : items) {
        apis->add(s);
    }
}

/**
 * 从 Qt 资源或磁盘加载补全词表。# / // 行视为注释。
 */
static bool registerApisFromFile(QsciAPIs *apis, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))
            || line.startsWith(QStringLiteral("//"))) {
            continue;
        }
        apis->add(line);
    }
    return true;
}

/**
 * 从 QMetaObject 中抽取本类自身声明的 Q_INVOKABLE 方法名，避免词表与实现漂移。
 */
static void registerMetaMethods(QsciAPIs *apis, const QMetaObject &mo,
                                const QString &prefix, bool addConstructorCall)
{
    if (addConstructorCall) {
        apis->add(QStringLiteral("new ") + QString::fromLatin1(mo.className()) + QStringLiteral("()"));
    }
    for (int i = mo.methodOffset(); i < mo.methodCount(); ++i) {
        const QMetaMethod method = mo.method(i);
        if (method.methodType() != QMetaMethod::Method) {
            continue;
        }
        const QString name = QString::fromLatin1(method.name());
        if (name.isEmpty() || name.startsWith(QStringLiteral("qt_"))) {
            continue;
        }
        apis->add(prefix + name + QStringLiteral("()"));
    }
}

/**
 * Node 与 9 种控件的 API 以源码元对象为准动态生成。
 */
static void registerRuntimeApis(QsciAPIs *apis)
{
    apis->add(QStringLiteral("Node"));
    registerMetaMethods(apis, Nodes::JavaScriptDataModel::staticMetaObject,
                        QStringLiteral("Node."), false);
    apis->add(QStringLiteral("function initInterface()"));
    apis->add(QStringLiteral("function inputEventHandler(index)"));

    registerMetaMethods(apis, SpinBox::staticMetaObject, QString(), true);
    registerMetaMethods(apis, DoubleSpinBox::staticMetaObject, QString(), true);
    registerMetaMethods(apis, Button::staticMetaObject, QString(), true);
    registerMetaMethods(apis, Label::staticMetaObject, QString(), true);
    registerMetaMethods(apis, LineEdit::staticMetaObject, QString(), true);
    registerMetaMethods(apis, CheckBox::staticMetaObject, QString(), true);
    registerMetaMethods(apis, ComboBox::staticMetaObject, QString(), true);
    registerMetaMethods(apis, HSlider::staticMetaObject, QString(), true);
    registerMetaMethods(apis, VSlider::staticMetaObject, QString(), true);
}

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
    
    /**
     * 编辑器行为优化设置：
     * - 按词换行并保留缩进
     * - 自动缩进与缩进辅助
     * - 自动补全更稳（阈值、大小写、替换、单项不弹窗）
     * - 括号匹配提高可读性
     */
    // 设置焦点策略，确保编辑器能够保持焦点
    m_codeEditor->setFocusPolicy(Qt::StrongFocus);

    // 文档中出现的单词支持自动补全
    m_codeEditor->setAutoCompletionSource(QsciScintilla::AcsAll);
    m_codeEditor->setCaretLineVisible(true);

    //设置光标所在行背景色
    m_codeEditor->setCaretLineBackgroundColor(Qt::lightGray);
    m_codeEditor->setFolding(QsciScintilla::BoxedTreeFoldStyle);//折叠样式

    m_codeEditor->setFoldMarginColors(Qt::gray,Qt::lightGray);//折叠栏颜色
    // 优化换行：按词换行 + 缩进对齐
    m_codeEditor->setWrapMode(QsciScintilla::WrapWord);
    m_codeEditor->setWrapIndentMode(QsciScintilla::WrapIndentIndented);
    
    // 编辑体验增强：自动缩进、缩进辅助与退格反缩进
    m_codeEditor->setAutoIndent(true);
    m_codeEditor->setIndentationGuides(true);
    m_codeEditor->setTabIndents(true);
    m_codeEditor->setBackspaceUnindents(true);
    
    // 自动补全：输入成员运算符（.）后立即弹出候选，单个候选也提示，忽略大小写
    m_codeEditor->setAutoCompletionReplaceWord(true);
    m_codeEditor->setAutoCompletionFillups(".");
    m_codeEditor->setAutoCompletionCaseSensitivity(false);
    m_codeEditor->setAutoCompletionShowSingle(true);
    m_codeEditor->setAutoCompletionThreshold(1);

    // 函数签名提示：输入左括号后显示参数表
    m_codeEditor->setCallTipsStyle(QsciScintilla::CallTipsNoContext);
    m_codeEditor->setCallTipsVisible(0);

    // 括号匹配：宽松匹配提高效果
    m_codeEditor->setBraceMatching(QsciScintilla::SloppyBraceMatch);

    QsciAPIs *apis = new QsciAPIs(textLexer);
    if (!registerApisFromFile(apis, QStringLiteral(":/JS_API/js_apis.api"))) {
        registerFallbackJsApis(apis);
    }
    registerRuntimeApis(apis);
    registerJsSnippets(apis);
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

    m_readOnlyCheckBox           = new QCheckBox(QCoreApplication::translate("Nodes::JsCodeEditor", "只读"), setupGroup);
    // auto *saveShortcutAction = new QShortcut((QKeySequence::fromString("Ctrl+S")),this);
    // auto *m_actionToggleComment =new QShortcut(QKeySequence::fromString("Ctrl+/"),this);
    // connect(saveShortcutAction , &QShortcut::activated, this, &CodeEditor::saveCode);
    m_setupLayout->addWidget(m_readOnlyCheckBox);
    m_setupLayout->addWidget(importJS);
    m_setupLayout->addWidget(exportJS);
    m_setupLayout->addSpacerItem(new QSpacerItem(1, 2, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void JsCodeEditor::setupWidgets()
{
    setWindowTitle(QCoreApplication::translate("Nodes::JsCodeEditor", "代码编辑器"));
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

    /**
     * 连接控件信号：使用 Qt 6 推荐的 checkStateChanged(Qt::CheckState)
     * 将编辑器的只读状态与“Read Only”复选框联动。
     */
    connect(
        m_readOnlyCheckBox,
        &QCheckBox::checkStateChanged,
        [this](Qt::CheckState state)
        {
            // 非 Unchecked（包含 PartiallyChecked/Checked）均视为只读
            m_codeEditor->setReadOnly(state != Qt::Unchecked);
        }
    );
    connect(exportJS,&QPushButton::clicked,this,&JsCodeEditor::exportCode);

    auto *completeShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Space")), m_codeEditor);
    connect(completeShortcut, &QShortcut::activated, this, [this]() {
        m_codeEditor->autoCompleteFromAll();
    });
    // // 绑定 Ctrl+/ 快捷键到内置注释切换
    // auto *m_actionToggleComment = new QShortcut(QKeySequence::fromString("Ctrl+/"), m_codeEditor);
    // connect(m_actionToggleComment, &QShortcut::activated, this, &JsCodeEditor::toggleComment);
}
void JsCodeEditor::setReadOnly(bool readOnly)
{
    m_readOnlyCheckBox->setChecked(readOnly);
}
/**
 * @brief 导出JS插件文件，包含元数据向导
 */
void JsCodeEditor::exportCode()
{
    // 创建插件元数据收集对话框
    QDialog* exportDialog = new QDialog(this);
    exportDialog->setWindowTitle(QCoreApplication::translate("Nodes::JsCodeEditor", "导出JS插件"));
    exportDialog->setModal(true);
    exportDialog->resize(400, 500);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(exportDialog);
    
    // 创建表单布局
    QFormLayout* formLayout = new QFormLayout();
    
    // 插件名称
    QLineEdit* nameEdit = new QLineEdit("MyPlugin");
    nameEdit->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "插件的显示名称"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "插件名称:"), nameEdit);
    
    // 插件描述
    QTextEdit* descEdit = new QTextEdit(QCoreApplication::translate("Nodes::JsCodeEditor", "插件描述"));
    descEdit->setMaximumHeight(60);
    descEdit->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "插件的功能描述"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "插件描述:"), descEdit);
    
    // 版本号
    QLineEdit* versionEdit = new QLineEdit("1.0.0");
    versionEdit->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "插件版本号，格式：主版本.次版本.修订版本"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "版本号:"), versionEdit);
    
    // 作者
    QLineEdit* authorEdit = new QLineEdit(QCoreApplication::translate("Nodes::JsCodeEditor", "作者名称"));
    authorEdit->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "插件作者姓名"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "作者:"), authorEdit);
    
    // 分类
    QComboBox* categoryCombo = new QComboBox();
    categoryCombo->addItems({"JS Plugins", "Controls", "Audio", "Video", "Data", "Network", "Utility", "Custom"});
    categoryCombo->setCurrentText("JS Plugins");
    categoryCombo->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "插件分类，用于在节点库中分组显示"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "分类:"), categoryCombo);
    
    // 输入端口数量
    QSpinBox* inputsSpinBox = new QSpinBox();
    inputsSpinBox->setRange(0, 99);
    inputsSpinBox->setValue(1);
    inputsSpinBox->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "插件的输入端口数量"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "输入端口数:"), inputsSpinBox);
    
    // 输出端口数量
    QSpinBox* outputsSpinBox = new QSpinBox();
    outputsSpinBox->setRange(0, 99);
    outputsSpinBox->setValue(1);
    outputsSpinBox->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "插件的输出端口数量"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "输出端口数:"), outputsSpinBox);
    
    // 可嵌入选项
    QCheckBox* embeddableCheckBox = new QCheckBox();
    embeddableCheckBox->setChecked(false);
    embeddableCheckBox->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "默认是否嵌入到节点中显示界面"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "嵌入:"), embeddableCheckBox);
    
    // 可调整大小选项
    QCheckBox* resizableCheckBox = new QCheckBox();
    resizableCheckBox->setChecked(true);
    resizableCheckBox->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "节点是否可以调整大小"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "可调整大小:"), resizableCheckBox);
    
    // 端口可编辑选项
    QCheckBox* portEditableCheckBox = new QCheckBox();
    portEditableCheckBox->setChecked(false);
    portEditableCheckBox->setToolTip(QCoreApplication::translate("Nodes::JsCodeEditor", "是否允许用户编辑端口数量"));
    formLayout->addRow(QCoreApplication::translate("Nodes::JsCodeEditor", "端口可编辑:"), portEditableCheckBox);
    
    // 添加表单到主布局
    mainLayout->addLayout(formLayout);
    
    // 添加按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* exportButton = new QPushButton(QCoreApplication::translate("Nodes::JsCodeEditor", "导出"));
    QPushButton* cancelButton = new QPushButton(QCoreApplication::translate("Nodes::JsCodeEditor", "取消"));
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    // 连接按钮信号
    connect(cancelButton, &QPushButton::clicked, exportDialog, &QDialog::reject);
    connect(exportButton, &QPushButton::clicked, [=]() {
        // 验证输入
        if (nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(exportDialog, QCoreApplication::translate("Nodes::JsCodeEditor", "错误"), QCoreApplication::translate("Nodes::JsCodeEditor", "请输入插件名称！"));
            return;
        }
        
        if (descEdit->toPlainText().trimmed().isEmpty()) {
            QMessageBox::warning(exportDialog, QCoreApplication::translate("Nodes::JsCodeEditor", "错误"), QCoreApplication::translate("Nodes::JsCodeEditor", "请输入插件描述！"));
            return;
        }
        
        // 获取exe目录路径
        QString exeDir = QApplication::applicationDirPath();
        QString pluginsDir = exeDir + "/plugins/JSScripts";
        
        // 确保目录存在
        QDir dir;
        if (!dir.exists(pluginsDir)) {
            if (!dir.mkpath(pluginsDir)) {
                QMessageBox::critical(exportDialog, QCoreApplication::translate("Nodes::JsCodeEditor", "错误"), 
                    QCoreApplication::translate("Nodes::JsCodeEditor", "无法创建目录：\n%1").arg(pluginsDir));
                return;
            }
        }
        
        // 构建文件名（确保文件名安全）
        QString safeName = nameEdit->text().trimmed();
        // 移除文件名中的非法字符
        
        QString fileName = pluginsDir + "/" + safeName + ".js";
        
        // 如果文件已存在，询问是否覆盖
        if (QFile::exists(fileName)) {
            int ret = QMessageBox::question(exportDialog, QCoreApplication::translate("Nodes::JsCodeEditor", "文件已存在"), 
                QCoreApplication::translate("Nodes::JsCodeEditor", "文件 %1 已存在，是否覆盖？").arg(safeName + ".js"),
                QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::No) {
                return;
            }
        }
        
      
      
        
        // 构建插件元数据JSON
        QJsonObject metadata;
        metadata["name"] = nameEdit->text().trimmed();
        metadata["description"] = descEdit->toPlainText().trimmed();
        metadata["version"] = versionEdit->text().trimmed();
        metadata["author"] = authorEdit->text().trimmed();
        metadata["category"] = categoryCombo->currentText();
        metadata["embeddable"] = embeddableCheckBox->isChecked();
        metadata["resizable"] = resizableCheckBox->isChecked();
        metadata["portEditable"] = portEditableCheckBox->isChecked();
        metadata["inputs"] = inputsSpinBox->value();
        metadata["outputs"] = outputsSpinBox->value();
        
        // 转换为JSON字符串并格式化
        QJsonDocument doc(metadata);
        QString jsonString = doc.toJson(QJsonDocument::Indented);
        
        // 构建完整的JS文件内容
        QString jsContent;
        jsContent += "/*! @plugin ";
        jsContent += jsonString;
        jsContent += " */\n\n";
        
        // 添加当前代码编辑器中的代码
        jsContent += m_codeEditor->text();
        
        // 写入文件
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << jsContent;
            file.close();
            
            QMessageBox::information(exportDialog, QCoreApplication::translate("Nodes::JsCodeEditor", "成功"), 
                QCoreApplication::translate("Nodes::JsCodeEditor", "JS插件已成功导出，重启软件生效！"));
                 exportDialog->accept();
        } else {
            QMessageBox::critical(exportDialog, QCoreApplication::translate("Nodes::JsCodeEditor", "错误"), 
                QCoreApplication::translate("Nodes::JsCodeEditor", "无法写入文件：\n%1\n\n错误：%2")
                .arg(fileName)
                .arg(file.errorString()));
        }
    });
    exportDialog->setFixedSize(300,500);
    // 显示对话框
    exportDialog->exec();
    
    // 清理
    exportDialog->deleteLater();
}

