//
// Created by WuBin on 2025/12/25.
//
#include "SettingWidget.h"
#include "Common/AppConfig/ConfigManager.h"
#include "Common/AppConfig/LicenseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>
#include <QGroupBox>
#include <QScrollArea>
#include <QGuiApplication>
#include <QClipboard>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>

#include "PushButton.h"

SettingWidget::SettingWidget(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("设置"));
    resize(500, 400); // 增加宽度以容纳侧边栏

    auto* mainLayout = new QVBoxLayout(this);
    
    // 主内容区域（左侧列表 + 右侧堆叠页面）
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(contentLayout);

    // 左侧侧边栏
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->setFrameShape(QFrame::NoFrame);
    m_listWidget->setSpacing(6);
    {
        const QStringList items = {
            tr("常规设置"),
            tr("WEB设置"),
            tr("OSC设置"),
            tr("MQTT设置"),
            tr("Artnet设置"),
            tr("日志设置"),
            tr("授权"),
        };
        QFontMetrics fm(m_listWidget->font());
        int maxTextW = 0;
        for (const auto& t : items) maxTextW = std::max(maxTextW, fm.horizontalAdvance(t));
        const int padding = 24;
        const int minWidth = std::max(140, maxTextW + padding);
        m_listWidget->setFixedWidth(minWidth);
        const int rowHeight = std::max(32, fm.height() + fm.leading() + 10);
        auto* it1 = new QListWidgetItem(tr("常规设置"));
        it1->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(it1);
        auto* itWeb = new QListWidgetItem(tr("WEB设置"));
        itWeb->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(itWeb);
        auto* itOsc = new QListWidgetItem(tr("OSC设置"));
        itOsc->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(itOsc);
        auto* itMqtt = new QListWidgetItem(tr("MQTT设置"));
        itMqtt->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(itMqtt);
        auto* itArtnet = new QListWidgetItem(tr("Artnet设置"));
        itArtnet->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(itArtnet);
        auto* it3 = new QListWidgetItem(tr("日志设置"));
        it3->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(it3);
        auto* it4 = new QListWidgetItem(tr("授权"));
        it4->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(it4);
        m_listWidget->setCurrentRow(0);
    }
    contentLayout->addWidget(m_listWidget);

    // 右侧内容区域
    m_stackedWidget = new QStackedWidget(this);
    contentLayout->addWidget(m_stackedWidget);

    // --- 页面 1: 常规设置 ---
    auto* pageGeneral = new QWidget();
    auto* layoutGeneral = new QVBoxLayout(pageGeneral);
    
    auto* lblGeneral = new QLabel(tr("常规设置"));
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 4);
    lblGeneral->setFont(titleFont);
    layoutGeneral->addWidget(lblGeneral);
    
    auto* lineGeneral = new QFrame();
    lineGeneral->setFrameShape(QFrame::HLine);
    lineGeneral->setFrameShadow(QFrame::Sunken);
    layoutGeneral->addWidget(lineGeneral);

    auto* formGeneral = new QFormLayout();
    formGeneral->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    
    m_maxRecentFilesSpin = new IntDragValueWidget(this);
    m_maxRecentFilesSpin->setRange(1, 50);
    formGeneral->addRow(tr("最近文件数量:"), m_maxRecentFilesSpin);

    // 【i18n】主题下拉（与语言同风格）：currentData 存 bool，直接映射既有
    // General/DefaultDarkTheme 键，配置格式不变；切换随底部确认保存，重启生效
    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem(tr("浅色"), QVariant(false));
    m_themeCombo->addItem(tr("深色"), QVariant(true));
    m_themeCombo->setToolTip(tr("界面主题，切换后重启生效"));
    formGeneral->addRow(tr("主题:"), m_themeCombo);

    // 【i18n 2026-11-28】语言切换入口在设置对话框（原菜单栏入口已撤）：
    // 随底部确认一并持久化，与暗色主题同策略——不主动重启，
    // saveSettings 既有文案“部分更改将在重启后生效”已覆盖
    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem(tr("简体中文"), QStringLiteral("zh"));
    m_languageCombo->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_languageCombo->addItem(QStringLiteral("日本語"), QStringLiteral("ja"));
    m_languageCombo->addItem(tr("繁体中文"), QStringLiteral("zh_TW"));
    m_languageCombo->addItem(QStringLiteral("Français"), QStringLiteral("fr"));
    m_languageCombo->setToolTip(tr("界面语言，切换后重启生效"));
    formGeneral->addRow(tr("语言:"), m_languageCombo);

    auto* lblAutosave = new QLabel(tr("自动保存"));
    lblAutosave->setFont(titleFont);
    formGeneral->addRow(lblAutosave);

    m_autosaveEnabledCheck = new QCheckBox(tr("启用自动保存"), this);
    formGeneral->addRow(tr("自动保存:"), m_autosaveEnabledCheck);

    m_autosaveIntervalSpin = new IntDragValueWidget(this);
    m_autosaveIntervalSpin->setRange(5, 3600);
    m_autosaveIntervalSpin->setSuffix(tr(" 秒"));
    formGeneral->addRow(tr("保存间隔:"), m_autosaveIntervalSpin);
    connect(m_autosaveEnabledCheck, &QCheckBox::toggled,
            m_autosaveIntervalSpin, &QWidget::setEnabled);
    auto* lblClock = new QLabel(tr("时钟设置"));
    lblClock->setFont(titleFont);
    formGeneral->addRow(lblClock);
    m_timestampFrameRateLabel = new QLabel(this);
    m_timestampFrameRateLabel->setToolTip(tr("全局时钟频率已固定，不可在此修改"));
    formGeneral->addRow(tr("全局时钟频率:"), m_timestampFrameRateLabel);

    m_audioOutputDelaySpin = new IntDragValueWidget(this);
    m_audioOutputDelaySpin->setRange(0, 16);
    m_audioOutputDelaySpin->setSuffix(tr(" 帧"));
    m_audioOutputDelaySpin->setToolTip(tr(
        "多源音频处理节点写出时在时间戳上追加的帧数。"
        "50 Hz 下 1 帧约 20 ms。越大越稳，延迟越高。"));
    formGeneral->addRow(tr("音频输出延时:"), m_audioOutputDelaySpin);

    layoutGeneral->addLayout(formGeneral);
    layoutGeneral->addStretch();
    m_stackedWidget->addWidget(pageGeneral);

    auto addPageTitle = [this, titleFont](QVBoxLayout* layout, const QString& title) {
        auto* label = new QLabel(title, this);
        label->setFont(titleFont);
        layout->addWidget(label);
        auto* line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        layout->addWidget(line);
    };

    // --- WEB 设置 ---
    auto* pageWeb = new QWidget();
    auto* layoutWeb = new QVBoxLayout(pageWeb);
    addPageTitle(layoutWeb, tr("WEB设置 (重启生效)"));
    auto* formWeb = new QFormLayout();
    formWeb->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_httpPortSpin = new IntDragValueWidget(this);
    m_httpPortSpin->setRange(1024, 65535);
    formWeb->addRow(tr("HTTP 服务器端口:"), m_httpPortSpin);
    m_webPasswordEdit = new QLineEdit(this);
    formWeb->addRow(tr("网页访问密码:"), m_webPasswordEdit);
    layoutWeb->addLayout(formWeb);
    layoutWeb->addStretch();
    m_stackedWidget->addWidget(pageWeb);

    // --- OSC 设置 ---
    auto* pageOsc = new QWidget();
    auto* layoutOsc = new QVBoxLayout(pageOsc);
    addPageTitle(layoutOsc, tr("OSC设置 (重启生效)"));
    auto* formOsc = new QFormLayout();
    formOsc->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_extraFeedbackHostEdit = new QLineEdit(this);
    formOsc->addRow(tr("OSC 反馈主机 IP:"), m_extraFeedbackHostEdit);
    m_extraFeedbackPortSpin = new IntDragValueWidget(this);
    m_extraFeedbackPortSpin->setRange(1024, 65535);
    formOsc->addRow(tr("OSC 反馈端口:"), m_extraFeedbackPortSpin);
    m_extraControlPortSpin = new IntDragValueWidget(this);
    m_extraControlPortSpin->setRange(1024, 65535);
    formOsc->addRow(tr("OSC 控制端口:"), m_extraControlPortSpin);
    m_oscInternalHostEdit = new QLineEdit(this);
    formOsc->addRow(tr("OSC 内部主机 IP:"), m_oscInternalHostEdit);
    m_oscEnabledCheck = new QCheckBox(tr("启用"), this);
    formOsc->addRow(tr("OSC外部反馈/控制:"), m_oscEnabledCheck);
    layoutOsc->addLayout(formOsc);
    layoutOsc->addStretch();
    m_stackedWidget->addWidget(pageOsc);

    // --- MQTT 设置 ---
    auto* pageMqtt = new QWidget();
    auto* layoutMqtt = new QVBoxLayout(pageMqtt);
    addPageTitle(layoutMqtt, tr("MQTT设置 (重启生效)"));
    auto* formMqtt = new QFormLayout();
    formMqtt->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_mqttHostEdit = new QLineEdit(this);
    m_mqttHostEdit->setPlaceholderText(tr("broker.emqx.io（仅主机名，勿填 mqtt://）"));
    formMqtt->addRow(tr("MQTT Broker 地址:"), m_mqttHostEdit);
    m_mqttPortSpin = new IntDragValueWidget(this);
    m_mqttPortSpin->setRange(1, 65535);
    formMqtt->addRow(tr("MQTT Broker 端口:"), m_mqttPortSpin);
    m_mqttUsernameEdit = new QLineEdit(this);
    m_mqttUsernameEdit->setPlaceholderText(tr("公共 Broker 可留空"));
    formMqtt->addRow(tr("MQTT 用户名:"), m_mqttUsernameEdit);
    m_mqttPasswordEdit = new QLineEdit(this);
    m_mqttPasswordEdit->setEchoMode(QLineEdit::Password);
    formMqtt->addRow(tr("MQTT 密码:"), m_mqttPasswordEdit);
    m_mqttControlTopicEdit = new QLineEdit(this);
    formMqtt->addRow(tr("MQTT 控制主题:"), m_mqttControlTopicEdit);
    m_mqttFeedbackTopicEdit = new QLineEdit(this);
    formMqtt->addRow(tr("MQTT 反馈主题:"), m_mqttFeedbackTopicEdit);
    m_mqttEnabledCheck = new QCheckBox(tr("启用"), this);
    formMqtt->addRow(tr("MQTT外部反馈/控制:"), m_mqttEnabledCheck);
    layoutMqtt->addLayout(formMqtt);
    layoutMqtt->addStretch();
    m_stackedWidget->addWidget(pageMqtt);

    // --- Artnet 设置（独立页；合并策略与发送频率均运行期即时生效） ---
    auto* pageArtnet = new QWidget();
    auto* layoutArtnet = new QVBoxLayout(pageArtnet);
    addPageTitle(layoutArtnet, tr("Artnet 设置（即时生效）"));
    auto* formArtnet = new QFormLayout();
    formArtnet->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_artnetMergeCombo = new QComboBox(this);
    // 与 ArtnetMergeMode 枚举值顺序保持一致：0=HTP 1=LTP
    m_artnetMergeCombo->addItem(tr("HTP（取大，强度合并，多源同写不闪烁）"));
    m_artnetMergeCombo->addItem(tr("LTP（后写优先，最近更新的源整包胜出）"));
    m_artnetMergeCombo->setToolTip(tr(
        "多个节点同时向同一 universe 写入数据时的合并策略。\n"
        "HTP：逐通道取最大值，适合强度合并；\n"
        "LTP：最近更新的源整包胜出，适合单源活跃的接管场景。"));
    formArtnet->addRow(tr("合并策略:"), m_artnetMergeCombo);

    m_artnetFpsSpin = new IntDragValueWidget(this);
    m_artnetFpsSpin->setRange(1, 100);
    m_artnetFpsSpin->setSuffix(QStringLiteral(" Hz"));
    m_artnetFpsSpin->setToolTip(tr(
        "Art-Net 发送节拍频率：发送器以固定频率重发所有活跃端点的最后状态。\n"
        "默认 50 Hz（20ms）；降低可减小网络流量，升高可降低控制延迟。"));
    formArtnet->addRow(tr("发送频率:"), m_artnetFpsSpin);
    layoutArtnet->addLayout(formArtnet);
    layoutArtnet->addStretch();
    m_stackedWidget->addWidget(pageArtnet);

    // --- 页面 3: 日志设置 ---
    auto* pageLog = new QWidget();
    auto* layoutLog = new QVBoxLayout(pageLog);

    auto* lblLog = new QLabel(tr("日志设置"));
    lblLog->setFont(titleFont);
    layoutLog->addWidget(lblLog);
    
    auto* lineLog = new QFrame();
    lineLog->setFrameShape(QFrame::HLine);
    lineLog->setFrameShadow(QFrame::Sunken);
    layoutLog->addWidget(lineLog);

    auto* formLog = new QFormLayout();
    formLog->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_maxLogEntriesSpin = new IntDragValueWidget(this);
    m_maxLogEntriesSpin->setRange(1, 10000);
    formLog->addRow(tr("最大日志显示条目:"), m_maxLogEntriesSpin);

    m_maxLogSaveEntriesSpin = new IntDragValueWidget(this);
    m_maxLogSaveEntriesSpin->setRange(1, 3650);
    formLog->addRow(tr("最大日志保存文件数:"), m_maxLogSaveEntriesSpin);

    layoutLog->addLayout(formLog);
    layoutLog->addStretch();
    m_stackedWidget->addWidget(pageLog);

    // --- 页面 4: 授权 ---
    auto* pageLicense = new QWidget();
    auto* layoutLicense = new QVBoxLayout(pageLicense);

    auto* lblLicense = new QLabel(tr("授权"));
    lblLicense->setFont(titleFont);
    layoutLicense->addWidget(lblLicense);

    auto* lineLicense = new QFrame();
    lineLicense->setFrameShape(QFrame::HLine);
    lineLicense->setFrameShadow(QFrame::Sunken);
    layoutLicense->addWidget(lineLicense);

    auto* formLicense = new QFormLayout();
    formLicense->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    // 激活闭环：激活码（填写一次后持久化到配置）+ 生成加密请求 .ft → 平台离线激活 → 签发令牌
    m_activationCodeEdit = new QLineEdit(this);
    m_activationCodeEdit->setPlaceholderText(tr("向运营索取的激活码"));
    auto* activationRow = new QHBoxLayout();
    activationRow->addWidget(m_activationCodeEdit, 1);
    auto* generateRequestButton = new QPushButton(tr("生成激活请求(.ft)"), this);
    activationRow->addWidget(generateRequestButton);
    formLicense->addRow(tr("激活码:"), activationRow);

    // 导入平台签发的授权文件（LP1. 令牌）：导入即验签并写入配置立即生效；清除激活可撤销
    auto* importLicenseButton = new QPushButton(tr("导入授权文件(.lic)..."), this);
    auto* clearAuthButton = new QPushButton(tr("清除激活"), this);
    auto* authButtonRow = new QHBoxLayout();
    authButtonRow->addWidget(importLicenseButton, 1);
    authButtonRow->addWidget(clearAuthButton);
    formLicense->addRow(tr("授权文件:"), authButtonRow);
    connect(importLicenseButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("选择授权文件"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("授权文件") + " (*.lic);;" + tr("所有文件") + " (*)");
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(this, tr("导入授权"),
                                 tr("无法读取文件：\n%1").arg(path));
            return;
        }
        // 剥除全部空白与 BOM（令牌本身只含 base64url 与 "."）
        QString token;
        const QString content = QString::fromUtf8(file.readAll());
        token.reserve(content.size());
        for (const QChar& c : content) {
            if (!c.isSpace() && c != QChar::ByteOrderMark) {
                token += c;
            }
        }
        if (token.isEmpty()) {
            QMessageBox::warning(this, tr("导入授权"),
                                 tr("文件内容为空：%1").arg(path));
            return;
        }
        if (LicenseManager::isCodeValid(token)) {
            // 验签通过立即写入配置（updateConfig 内部 saveConfig 落盘），不依赖底部 OK
            QJsonObject patch;
            patch["LicenseCode"] = token;
            ConfigManager::instance().updateConfig(patch);
        } else {
            QMessageBox::warning(this, tr("导入授权"),
                                 tr("授权文件无效：\n%1")
                                     .arg(LicenseManager::verifyMessage(token)));
        }
        refreshLicenseStatus();
    });
    connect(clearAuthButton, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, tr("清除激活"),
                                  tr("确定清除本机授权吗？\n清除后 Advanced 插件不再加载，需重新导入授权文件。"))
            != QMessageBox::Yes) {
            return;
        }
        QJsonObject patch;
        patch["LicenseCode"] = QString();
        ConfigManager::instance().updateConfig(patch);
        // exe 旁 license.lic（运维放置的第二授权来源）一并删除
        const QString licPath = LicenseManager::licenseFilePath();
        if (!licPath.isEmpty() && QFile::exists(licPath)) {
            QFile::remove(licPath);
        }
        refreshLicenseStatus();
    });

    m_licenseStatusLabel = new QLabel(this);
    formLicense->addRow(tr("状态:"), m_licenseStatusLabel);

    layoutLicense->addLayout(formLicense);

    // SDK 原生库缺失诊断（fail-closed：此时授权恒为无效）
    m_sdkStatusLabel = new QLabel(this);
    m_sdkStatusLabel->setWordWrap(true);
    m_sdkStatusLabel->setStyleSheet(QStringLiteral("color: #d85a30;"));
    m_sdkStatusLabel->setText(LicenseManager::sdkStatusText());
    m_sdkStatusLabel->setVisible(!LicenseManager::sdkAvailable());
    layoutLicense->addWidget(m_sdkStatusLabel);

    layoutLicense->addStretch();
    m_stackedWidget->addWidget(pageLicense);

    // 生成 .ft 加密激活请求（内含机器码，交运营上传平台离线激活）
    connect(generateRequestButton, &QPushButton::clicked, this, [this]() {
        const QString code = m_activationCodeEdit->text().trimmed();
        if (code.isEmpty()) {
            QMessageBox::warning(this, tr("激活请求"), tr("请先填写激活码。"));
            return;
        }
        if (!LicenseManager::sdkAvailable()) {
            QMessageBox::warning(this, tr("激活请求"), LicenseManager::sdkStatusText());
            return;
        }
        // 默认存到"文档"目录，文件名带机器码（多机收集请求文件时不混淆）
        const QString machine = LicenseManager::machineCode();
        const QString docsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        const QString defaultPath =
            QDir(docsDir.isEmpty() ? QDir::homePath() : docsDir)
                .filePath(machine.isEmpty() ? QStringLiteral("Flow_激活请求.ft")
                                            : QStringLiteral("Flow_激活请求_%1.ft").arg(machine));
        const QString path = QFileDialog::getSaveFileName(
            this, tr("保存激活请求"), defaultPath, tr("激活请求") + " (*.ft)");
        if (path.isEmpty()) {
            return;
        }
        QString error;
        if (LicenseManager::generateActivationRequest(code, path, &error)) {
            QMessageBox::information(
                this, tr("激活请求"),
                tr("已生成加密激活请求文件：\n%1\n\n机器码：%2\n\n"
                   "请将该文件提交给运营平台完成离线激活，下载授权文件(.lic)后，"
                   "在此页点击\"导入授权文件(.lic)...\"导入即可完成授权。")
                    .arg(path, LicenseManager::machineCode()));
        } else {
            QMessageBox::warning(this, tr("激活请求"), error);
        }
    });

    // 连接列表与页面切换
    connect(m_listWidget, &QListWidget::currentRowChanged, m_stackedWidget, &QStackedWidget::setCurrentIndex);

    // 底部按钮
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &SettingWidget::saveSettings);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    loadCurrentSettings();
    
    // 默认选中第一项
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void SettingWidget::loadCurrentSettings() {
    auto& config = ConfigManager::instance();
    
    m_maxRecentFilesSpin->setValue(config.getMaxRecentFiles());
    {
        const int themeIdx = m_themeCombo->findData(QVariant(config.isDefaultDarkTheme()));
        m_themeCombo->setCurrentIndex(themeIdx < 0 ? 0 : themeIdx);
    }
    {
        const int langIdx = m_languageCombo->findData(config.getLanguage());
        m_languageCombo->setCurrentIndex(langIdx < 0 ? 0 : langIdx);
    }
    m_autosaveEnabledCheck->setChecked(config.isAutosaveEnabled());
    m_autosaveIntervalSpin->setValue(config.getAutosaveIntervalSeconds());
    m_autosaveIntervalSpin->setEnabled(m_autosaveEnabledCheck->isChecked());
    {
        const double fps = config.getTimestampFrameRate();
        m_timestampFrameRateLabel->setText(QStringLiteral("%1 Hz").arg(fps, 0, 'f', 0));
    }
    m_audioOutputDelaySpin->setValue(config.getAudioOutputDelayFrames());
    // Art-Net 合并策略（下拉框索引与 ArtnetMergeMode 枚举值一致）与发送频率
    m_artnetMergeCombo->setCurrentIndex(qBound(0, config.getArtnetMergeMode(), 1));
    m_artnetFpsSpin->setValue(config.getArtnetSendFps());

    m_httpPortSpin->setValue(config.getHttpServerPort());
    m_extraFeedbackHostEdit->setText(config.getExtraFeedbackHost());
    m_extraFeedbackPortSpin->setValue(config.getExtraFeedbackPort());
    m_extraControlPortSpin->setValue(config.getExtraControlPort());
    m_oscInternalHostEdit->setText(config.getOscInternalControlHost());
    m_oscEnabledCheck->setChecked(config.isOscEnabled());
    m_mqttHostEdit->setText(config.getMqttHost());
    m_mqttPortSpin->setValue(config.getMqttPort());
    m_mqttUsernameEdit->setText(config.getMqttUsername());
    m_mqttPasswordEdit->setText(config.getMqttPassword());
    m_mqttControlTopicEdit->setText(config.getMqttControlTopic());
    m_mqttFeedbackTopicEdit->setText(config.getMqttFeedbackTopic());
    m_mqttEnabledCheck->setChecked(config.isMqttEnabled());
    m_webPasswordEdit->setText(config.getWebAccessPassword());
    // Log Settings
    m_maxLogEntriesSpin->setValue(config.getMaxLogEntries());
    m_maxLogSaveEntriesSpin->setValue(config.getMaxLogSaveEntries());
    m_activationCodeEdit->setText(config.getActivationCode());
    refreshLicenseStatus();
}

void SettingWidget::refreshLicenseStatus() {
    // 展示优先级：配置中的令牌 > exe 旁 license.lic
    const QString token = ConfigManager::instance().getLicenseCode();
    if (!token.trimmed().isEmpty() && LicenseManager::isCodeValid(token)) {
        const QString summary = LicenseManager::tokenSummary(token);
        m_licenseStatusLabel->setText(summary.isEmpty() ? tr("已授权") : summary);
        m_licenseStatusLabel->setStyleSheet(QStringLiteral("color: green;"));
        m_licenseStatusLabel->setToolTip(QString());
        return;
    }
    if (LicenseManager::isAuthorized()) {
        m_licenseStatusLabel->setText(tr("已授权（license.lic）"));
        m_licenseStatusLabel->setStyleSheet(QStringLiteral("color: green;"));
        m_licenseStatusLabel->setToolTip(QString());
        return;
    }
    if (!token.trimmed().isEmpty()) {
        m_licenseStatusLabel->setText(tr("授权无效"));
        m_licenseStatusLabel->setStyleSheet(QStringLiteral("color: red;"));
        m_licenseStatusLabel->setToolTip(LicenseManager::verifyMessage(token));
        return;
    }
    m_licenseStatusLabel->setText(tr("未授权"));
    m_licenseStatusLabel->setStyleSheet(QString());
    m_licenseStatusLabel->setToolTip(QString());
}

void SettingWidget::saveSettings() {
    QJsonObject obj;
    obj["MaxRecentFiles"] = m_maxRecentFilesSpin->value();
    obj["HttpServerPort"] = m_httpPortSpin->value();

    obj["ExtraFeedbackHost"] = m_extraFeedbackHostEdit->text();
    obj["ExtraFeedbackPort"] = m_extraFeedbackPortSpin->value();
    obj["ExtraControlPort"] = m_extraControlPortSpin->value();
    obj["OscInternalControlHost"] = m_oscInternalHostEdit->text();
    obj["DefaultDarkTheme"] = m_themeCombo->currentData().toBool();
    obj["AutosaveEnabled"] = m_autosaveEnabledCheck->isChecked();
    obj["AutosaveIntervalSeconds"] = m_autosaveIntervalSpin->value();
    obj["AudioOutputDelayFrames"] = m_audioOutputDelaySpin->value();
    obj["ArtnetMergeMode"] = m_artnetMergeCombo->currentIndex();
    obj["ArtnetSendFps"] = m_artnetFpsSpin->value();
    obj["OscEnabled"] = m_oscEnabledCheck->isChecked();
    obj["MqttEnabled"] = m_mqttEnabledCheck->isChecked();
    obj["MqttHost"] = m_mqttHostEdit->text();
    obj["MqttPort"] = m_mqttPortSpin->value();
    obj["MqttUsername"] = m_mqttUsernameEdit->text();
    obj["MqttPassword"] = m_mqttPasswordEdit->text();
    obj["MqttControlTopic"] = m_mqttControlTopicEdit->text();
    obj["MqttFeedbackTopic"] = m_mqttFeedbackTopicEdit->text();
    obj["WebAccessPassword"] = m_webPasswordEdit->text();
    // Log Settings
    obj["MaxLogEntries"] = m_maxLogEntriesSpin->value();
    obj["MaxLogSaveEntries"] = m_maxLogSaveEntriesSpin->value();
    obj["ActivationCode"] = m_activationCodeEdit->text().trimmed();
    // LicenseCode 不在此处覆盖：由“导入授权文件/清除激活”即时写入配置（updateConfig 为按键合并）

    // 【i18n】语言随确认一并持久化（setLanguage 内部落盘），重启后生效，不主动重启
    ConfigManager::instance().setLanguage(m_languageCombo->currentData().toString());

    ConfigManager::instance().updateConfig(obj);

    const QString licenseHint = LicenseManager::isAuthorized()
        ? tr("授权有效，重启后将加载外部插件。")
        : tr("授权无效，Advanced 插件不会加载。");
    QMessageBox::information(this, tr("设置"),
                             tr("设置已保存，部分更改将在重启后生效。\n%1").arg(licenseHint));
    accept();
}


static inline QPoint centerOnScreen(QWidget* w) {
    if (QGuiApplication::primaryScreen()) {
        QRect r = QGuiApplication::primaryScreen()->geometry();
        QSize s = w->sizeHint();
        return QPoint(r.center().x() - s.width()/2, r.center().y() - s.height()/2);
    }
    return QPoint(100, 100);
}

void SettingWidget::showDialog(QWidget* parent) {
    SettingWidget dlg(parent);
    dlg.move(centerOnScreen(&dlg));
    dlg.exec();
}
