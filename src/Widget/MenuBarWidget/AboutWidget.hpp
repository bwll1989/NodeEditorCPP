//
// Created by 吴斌 on 2024/1/17.
//

#pragma once

#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QFile>
#include <QFileInfo>
#include <QTextBrowser>
#include <QTextStream>
#include <QTextDocument>
#include "../../ConstantDefines.h"
class AboutWidget : public QDialog
{
public:
    /**
     * 函数级注释：关于窗口构造函数
     * - 标题显示“产品名 + 版本号”
     * - 使用 QTextBrowser 渲染 Markdown 文档并支持超链接点击
     */
    AboutWidget(const QString &filePath, QWidget *parent = nullptr)
            : QDialog(parent)
    {
        setWindowTitle(QString("关于 %1 v%2")
                           .arg(AppConstants::PRODUCT_NAME)
                           .arg(QApplication::applicationVersion()));

        QVBoxLayout *layout = new QVBoxLayout(this);

        // 用 QTextBrowser 替换 QTextEdit，支持超链接
        QTextBrowser *textBrowser = new QTextBrowser(this);
        textBrowser->setOpenExternalLinks(true);                // 直接在系统浏览器打开外链
        textBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction);
        layout->addWidget(textBrowser);

        loadTextFile(filePath, textBrowser);

        setMinimumSize(640,480);
    }
private:
    /**
     * 函数级注释：加载文本并渲染
     * - Qt6 使用 setEncoding(Utf8)，Qt5 使用 setCodec("UTF-8")
     * - Markdown 优先使用 QTextDocument::setMarkdown（Qt ≥ 5.14）
     * - 设置 BaseUrl 以支持相对链接（本地图片、相对路径）
     */
    void loadTextFile(const QString &filePath, QTextBrowser *textBrowser)
    {
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream stream(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            stream.setEncoding(QStringConverter::Utf8);
#else
            stream.setCodec("UTF-8");
#endif
            const QString fileContent = stream.readAll();
            file.close();

            // 设置文档基础路径，保证相对链接工作
            const QString baseDir = QFileInfo(filePath).absolutePath();
            textBrowser->document()->setBaseUrl(QUrl::fromLocalFile(baseDir + "/"));

            const bool isMarkdown = filePath.endsWith(".md", Qt::CaseInsensitive);
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            if (isMarkdown) {
                textBrowser->document()->setMarkdown(fileContent);
            } else {
                textBrowser->document()->setMarkdown(fileContent);
            }
#else
            textBrowser->setPlainText(fileContent);
#endif
        } else {
            textBrowser->setPlainText(QString("无法打开文件：%1\n错误：%2")
                                          .arg(filePath)
                                          .arg(file.errorString()));
        }
    }
};
