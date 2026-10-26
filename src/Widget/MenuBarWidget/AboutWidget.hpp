//
// Created by 吴斌 on 2024/1/17.
//

#pragma once

#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFile>
#include <QFileInfo>
#include <QTextBrowser>
#include <QTextStream>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextFragment>
#include <QColor>
#include <QPixmap>
#include <QFont>
#include <QFrame>
#include "../../Common/AppConfig/ConfigManager.h"
#include "../../Common/AppConfig/LicenseManager.h"

namespace {

constexpr char kAboutLinkColor[] = "#81a4b2";

/** setMarkdown 会给锚点写入内联蓝色，需事后覆盖前景色。 */
void recolorDocumentAnchors(QTextDocument *doc, const QColor &color)
{
    if (!doc) {
        return;
    }
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        for (auto it = block.begin(); !it.atEnd(); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid()) {
                continue;
            }
            QTextCharFormat fmt = frag.charFormat();
            if (!fmt.isAnchor()) {
                continue;
            }
            fmt.setForeground(color);
            QTextCursor cursor(doc);
            cursor.setPosition(frag.position());
            cursor.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
            cursor.setCharFormat(fmt);
        }
    }
}

QLabel *makeMutedLabel(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setStyleSheet(QStringLiteral("color: palette(mid);"));
    return label;
}

} // namespace

class AboutWidget : public QDialog
{
public:
    /**
     * 关于窗口：上方图标 + 版本说明，下方 README，底部版权与 OK。
     */
    AboutWidget(const QString &filePath, QWidget *parent = nullptr)
        : QDialog(parent)
    {
        const QString product = AppConstants::PRODUCT_NAME;
        const QString edition = LicenseManager::isAuthorized()
                                    ? QStringLiteral("Advanced")
                                    : QStringLiteral("Basic");
        const QString version = QApplication::applicationVersion();

        setWindowTitle(tr("About %1").arg(product));
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        setMinimumSize(720, 480);
        resize(780, 560);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(28, 28, 28, 20);
        root->setSpacing(16);

        // —— 上：图标 + 软件版本说明 ——
        auto *headerRow = new QHBoxLayout;
        headerRow->setSpacing(20);

        auto *logoLabel = new QLabel(this);
        const QPixmap logoPixmap(QStringLiteral(":/icons/icons/Flow.png"));
        logoLabel->setPixmap(
            logoPixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        logoLabel->setFixedSize(96, 96);
        logoLabel->setAlignment(Qt::AlignCenter);
        logoLabel->setAttribute(Qt::WA_TranslucentBackground);
        logoLabel->setStyleSheet(QStringLiteral("background: transparent;"));
        headerRow->addWidget(logoLabel, 0, Qt::AlignTop);

        auto *infoCol = new QVBoxLayout;
        infoCol->setSpacing(6);
        infoCol->setContentsMargins(0, 0, 0, 0);

        auto *titleLabel = new QLabel(tr("About %1").arg(product), this);
        {
            QFont font = titleLabel->font();
            font.setPointSizeF(font.pointSizeF() + 5);
            font.setBold(true);
            titleLabel->setFont(font);
        }

        auto *versionLabel = makeMutedLabel(
            tr("本程序为 %1 %2 Version %3。")
                .arg(product, edition, version),
            this);

        auto *frameworkLabel = makeMutedLabel(AppConstants::FILE_DESCRIPTION, this);

        auto *introLabel = new QLabel(
            tr("%1 是面向演出、展陈与现场控制的可视化节点编排软件，"
               "将数据流编辑器、时间轴与外部协议控制整合在同一工程中，"
               "用连线方式搭建灯光、音视频、设备通信与自动化逻辑。")
                .arg(product),
            this);
        introLabel->setWordWrap(true);
        introLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

        infoCol->addWidget(titleLabel);
        infoCol->addWidget(versionLabel);
        infoCol->addWidget(frameworkLabel);
        infoCol->addSpacing(4);
        infoCol->addWidget(introLabel);
        infoCol->addStretch(1);

        headerRow->addLayout(infoCol, 1);
        root->addLayout(headerRow, 0);

        auto *separator = new QFrame(this);
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        separator->setStyleSheet(QStringLiteral("color: palette(mid);"));
        root->addWidget(separator);

        // —— 下：README ——
        auto *textBrowser = new QTextBrowser(this);
        textBrowser->setOpenExternalLinks(true);
        textBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction);
        textBrowser->setFrameShape(QFrame::NoFrame);
        textBrowser->setStyleSheet(QStringLiteral(
            "QTextBrowser {"
            "  background: transparent;"
            "  padding: 0px;"
            "}"));
        textBrowser->document()->setDocumentMargin(0);
        loadTextFile(filePath, textBrowser);
        root->addWidget(textBrowser, 1);

        // —— 底栏：版权 + OK ——
        auto *footer = new QHBoxLayout;
        footer->setContentsMargins(0, 0, 0, 0);
        footer->setSpacing(12);

        auto *copyrightLabel = makeMutedLabel(AppConstants::LEGAL_COPYRIGHT, this);
        footer->addWidget(copyrightLabel, 1);

        auto *okButton = new QPushButton(tr("OK"), this);
        okButton->setDefault(true);
        okButton->setMinimumWidth(96);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
        footer->addWidget(okButton, 0, Qt::AlignRight | Qt::AlignVCenter);

        root->addLayout(footer);
    }

private:
    /**
     * 加载 Markdown / 纯文本并渲染到 QTextBrowser。
     * Qt6 使用 setEncoding(Utf8)；Markdown 优先 setMarkdown（Qt ≥ 5.14）。
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

            const QString baseDir = QFileInfo(filePath).absolutePath();
            textBrowser->document()->setBaseUrl(QUrl::fromLocalFile(baseDir + "/"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
            textBrowser->document()->setMarkdown(fileContent);
            recolorDocumentAnchors(textBrowser->document(),
                                   QColor(QLatin1String(kAboutLinkColor)));
#else
            Q_UNUSED(filePath);
            textBrowser->setPlainText(fileContent);
#endif
        } else {
            textBrowser->setPlainText(
                tr("无法打开文件：%1\n错误：%2")
                    .arg(filePath, file.errorString()));
        }
    }
};
