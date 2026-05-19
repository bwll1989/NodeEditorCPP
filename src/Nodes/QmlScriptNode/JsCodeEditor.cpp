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

using namespace Nodes;
/**
 * 批量注册内置的单行提示词与少量多行模板
 * 使用 QStringList 循环，显著减少逐条 apis->add() 的样板代码。
 */
static void registerDefaultJsApis(QsciAPIs* apis)
{
    // 单行提示词（类名、方法、关键字等）
    const QStringList items = {
        "Node.inputIndex()",
        "Node.getOutputCount()",
        "Node.getInputCount()",
        "Node.getInputValue()",
        "Node.setOutputValue()",
        "print()",
        "typeof",
        "var",
        "null",
        "new",
        "SpinBox",
        "CheckBox",
        "LineEdit",
        "Label",
        "Button",
        "VSlider",
        "HSlider",
        "DoubleSpinBox",
        "ComboBox",
        "clearLayout",
        "setRange",
        "setValue",
        "Node.addToLayout",
        "while()",
        "else",
        "for",
        "break",
        "continue",
        "function",
        "console.log()"
    };
    for (const auto& s : items) {
        apis->add(s);
    }

    // 多行模板示例：保留为单独的大字符串添加
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
}
/**
 * 尝试从文件加载 API 列表
 * 优先从 Qt 资源路径（如 :/res/js_apis.api）或磁盘路径读取，每行一个词条。
 * 成功返回 true；失败返回 false（由调用方回退到默认列表）。
 */
static bool registerApisFromFile(QsciAPIs* apis, const QString& filePath)
{
    return apis->load(filePath);
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
    
    // 自动补全：替换单词，大小写敏感，阈值适度，单项不弹窗
    m_codeEditor->setAutoCompletionReplaceWord(true);
    m_codeEditor->setAutoCompletionCaseSensitivity(true);
    m_codeEditor->setAutoCompletionShowSingle(false);
    m_codeEditor->setAutoCompletionThreshold(1);
    
    // 括号匹配：宽松匹配提高效果
    m_codeEditor->setBraceMatching(QsciScintilla::SloppyBraceMatch);
    
    //常用关键字支持自动补全
    QsciAPIs *apis = new QsciAPIs(textLexer);
    // 加载提示词文件（类名、方法、关键字等）
    registerApisFromFile(apis, ":/JS_API/js_apis.api");

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
    m_setupLayout->addWidget(importJS);
    m_setupLayout->addWidget(updateUI);
    m_setupLayout->addWidget(exportJS);
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
    exportDialog->setWindowTitle("导出JS插件");
    exportDialog->setModal(true);
    exportDialog->resize(400, 500);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(exportDialog);
    
    // 创建表单布局
    QFormLayout* formLayout = new QFormLayout();
    
    // 插件名称
    QLineEdit* nameEdit = new QLineEdit("MyPlugin");
    nameEdit->setToolTip("插件的显示名称");
    formLayout->addRow("插件名称:", nameEdit);
    
    // 插件描述
    QTextEdit* descEdit = new QTextEdit("插件描述");
    descEdit->setMaximumHeight(60);
    descEdit->setToolTip("插件的功能描述");
    formLayout->addRow("插件描述:", descEdit);
    
    // 版本号
    QLineEdit* versionEdit = new QLineEdit("1.0.0");
    versionEdit->setToolTip("插件版本号，格式：主版本.次版本.修订版本");
    formLayout->addRow("版本号:", versionEdit);
    
    // 作者
    QLineEdit* authorEdit = new QLineEdit("作者名称");
    authorEdit->setToolTip("插件作者姓名");
    formLayout->addRow("作者:", authorEdit);
    
    // 分类
    QComboBox* categoryCombo = new QComboBox();
    categoryCombo->addItems({"Controls", "Audio", "Video", "Data", "Network", "Utility", "Custom"});
    categoryCombo->setCurrentText("Controls");
    categoryCombo->setToolTip("插件分类，用于在节点库中分组显示");
    formLayout->addRow("分类:", categoryCombo);
    
    // 输入端口数量
    QSpinBox* inputsSpinBox = new QSpinBox();
    inputsSpinBox->setRange(0, 99);
    inputsSpinBox->setValue(1);
    inputsSpinBox->setToolTip("插件的输入端口数量");
    formLayout->addRow("输入端口数:", inputsSpinBox);
    
    // 输出端口数量
    QSpinBox* outputsSpinBox = new QSpinBox();
    outputsSpinBox->setRange(0, 99);
    outputsSpinBox->setValue(1);
    outputsSpinBox->setToolTip("插件的输出端口数量");
    formLayout->addRow("输出端口数:", outputsSpinBox);
    
    // 可嵌入选项
    QCheckBox* embeddableCheckBox = new QCheckBox();
    embeddableCheckBox->setChecked(false);
    embeddableCheckBox->setToolTip("默认是否嵌入到节点中显示界面");
    formLayout->addRow("嵌入:", embeddableCheckBox);
    
    // 可调整大小选项
    QCheckBox* resizableCheckBox = new QCheckBox();
    resizableCheckBox->setChecked(true);
    resizableCheckBox->setToolTip("节点是否可以调整大小");
    formLayout->addRow("可调整大小:", resizableCheckBox);
    
    // 端口可编辑选项
    QCheckBox* portEditableCheckBox = new QCheckBox();
    portEditableCheckBox->setChecked(false);
    portEditableCheckBox->setToolTip("是否允许用户编辑端口数量");
    formLayout->addRow("端口可编辑:", portEditableCheckBox);
    
    // 添加表单到主布局
    mainLayout->addLayout(formLayout);
    
    // 添加按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* exportButton = new QPushButton("导出");
    QPushButton* cancelButton = new QPushButton("取消");
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    // 连接按钮信号
    connect(cancelButton, &QPushButton::clicked, exportDialog, &QDialog::reject);
    connect(exportButton, &QPushButton::clicked, [=]() {
        // 验证输入
        if (nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(exportDialog, "错误", "请输入插件名称！");
            return;
        }
        
        if (descEdit->toPlainText().trimmed().isEmpty()) {
            QMessageBox::warning(exportDialog, "错误", "请输入插件描述！");
            return;
        }
        
        // 获取exe目录路径
        QString exeDir = QApplication::applicationDirPath();
        QString pluginsDir = exeDir + "/plugins/JSScripts";
        
        // 确保目录存在
        QDir dir;
        if (!dir.exists(pluginsDir)) {
            if (!dir.mkpath(pluginsDir)) {
                QMessageBox::critical(exportDialog, "错误", 
                    QString("无法创建目录：\n%1").arg(pluginsDir));
                return;
            }
        }
        
        // 构建文件名（确保文件名安全）
        QString safeName = nameEdit->text().trimmed();
        // 移除文件名中的非法字符
        
        QString fileName = pluginsDir + "/" + safeName + ".js";
        
        // 如果文件已存在，询问是否覆盖
        if (QFile::exists(fileName)) {
            int ret = QMessageBox::question(exportDialog, "文件已存在", 
                QString("文件 %1 已存在，是否覆盖？").arg(safeName + ".js"),
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
            
            QMessageBox::information(exportDialog, "成功", 
                QString("JS插件已成功导出，重启软件生效！"));
                 exportDialog->accept();
        } else {
            QMessageBox::critical(exportDialog, "错误", 
                QString("无法写入文件：\n%1\n\n错误：%2")
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

