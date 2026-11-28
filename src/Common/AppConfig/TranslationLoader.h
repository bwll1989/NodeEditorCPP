#pragma once
// ===========================================================================
//  TranslationLoader.h —— 应用级翻译加载（Qt Linguist，2026-11-28）
// ---------------------------------------------------------------------------
// 设计要点：
//  • 源文语言 = 中文：tr("中文") 查不到译文时原样返回中文，因此中文界面
//    无需任何翻译文件（零成本）；英文界面由 flow_en.qm 提供译文。
//  • 单一 .qm 服务全程序：翻译查找按 context（类名）+源文 在进程内所有
//    已安装 QTranslator 中进行，与类住在哪个 DLL 无关——主程序启动时装
//    一个 .qm 即可覆盖全部插件（context 天然按类名隔离），插件无需各自
//    加载翻译。第三方 FlowSDK 插件如需自译，可另行 installTranslator
//    （后装的先查，不冲突）。
//  • 查找顺序：exe 同目录 translations/flow_<lang>.qm（部署形态 + 免重编
//    调试：改 .qm 即生效）→ 无则告警并保持中文显示。
//  • 语言切换需重启生效（UI 文本在构造期写入，无 retranslate 链路）。
//    切换入口：MenuBarWidget「语言」菜单 → General/Language（Settings.ini）。
// ===========================================================================
#include <QTranslator>
#include <QCoreApplication>
#include <QLocale>
#include <QDebug>
#include <QStringList>
#include "ConfigManager.h"

class TranslationLoader {
public:
    /// 在 QApplication 构造后、任何窗口/控件创建前调用一次
    static void installAppTranslators()
    {
        const QString lang = ConfigManager::instance().getLanguage();

        // 【i18n】默认 locale 跟随界面语言：QCalendarWidget 表头/月份、
        // QDateEdit 等按 QLocale()（默认 locale）取本地化文本。不设置时
        // 跟随系统（中文 Windows 上英文界面仍显示"星期一"）。
        // 必须在任何控件构造前调用（widget 的 locale 属性在构造期取默认值）。
        if (lang == QStringLiteral("en")) {
            QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
        } else if (lang == QStringLiteral("ja")) {
            QLocale::setDefault(QLocale(QLocale::Japanese, QLocale::Japan));
        } else if (lang == QStringLiteral("zh_TW")) {
            QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::Taiwan));
        } else if (lang == QStringLiteral("fr")) {
            QLocale::setDefault(QLocale(QLocale::French, QLocale::France));
        } else {
            QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));
        }

        if (lang == QStringLiteral("zh")) {
            return; // 源文即简体中文，原生显示
        }

        auto* translator = new QTranslator(QCoreApplication::instance());
        const QString qmName = QStringLiteral("flow_%1.qm").arg(lang);
        // 优先 exe 同目录 translations/（安装布局与开发构建一致），支持免重建热替换译文
        const QStringList searchPaths = {
            QCoreApplication::applicationDirPath() + QStringLiteral("/translations/"),
            QCoreApplication::applicationDirPath() + QStringLiteral("/"),
        };
        for (const QString& dir : searchPaths) {
            if (translator->load(dir + qmName)) {
                QCoreApplication::installTranslator(translator);
                qDebug().noquote() << QStringLiteral("[i18n] loaded %1%2").arg(dir, qmName);
                return;
            }
        }
        qWarning().noquote() << QStringLiteral("[i18n] translation file not found for language '%1' (tried %2)")
                                .arg(lang, searchPaths.join(QStringLiteral("; ")));
        delete translator;
    }
};
