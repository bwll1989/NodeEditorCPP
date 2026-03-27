//
// Created by WuBin on 2025/12/25.
//
#include "SettingWidget.h"
#include "Common/AppConfig/ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>
#include <QGroupBox>
#include <QScrollArea>

#include "PushButton.h"

SettingWidget::SettingWidget(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("设置"));
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
        const QStringList items = {QStringLiteral("常规设置"), QStringLiteral("网络设置"), QStringLiteral("日志设置")};
        QFontMetrics fm(m_listWidget->font());
        int maxTextW = 0;
        for (const auto& t : items) maxTextW = std::max(maxTextW, fm.horizontalAdvance(t));
        const int padding = 24;
        const int minWidth = std::max(140, maxTextW + padding);
        m_listWidget->setFixedWidth(minWidth);
        const int rowHeight = std::max(32, fm.height() + fm.leading() + 10);
        auto* it1 = new QListWidgetItem(QStringLiteral("常规设置"));
        it1->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(it1);
        auto* it2 = new QListWidgetItem(QStringLiteral("网络设置"));
        it2->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(it2);
        auto* it3 = new QListWidgetItem(QStringLiteral("日志设置"));
        it3->setSizeHint(QSize(0, rowHeight));
        m_listWidget->addItem(it3);
        m_listWidget->setCurrentRow(0);
    }
    contentLayout->addWidget(m_listWidget);

    // 右侧内容区域
    m_stackedWidget = new QStackedWidget(this);
    contentLayout->addWidget(m_stackedWidget);

    // --- 页面 1: 常规设置 ---
    auto* pageGeneral = new QWidget();
    auto* layoutGeneral = new QVBoxLayout(pageGeneral);
    
    auto* lblGeneral = new QLabel("常规设置");
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
    formGeneral->addRow("最近文件数量:", m_maxRecentFilesSpin);

    m_darkThemeCheck = new QCheckBox("启用暗色主题", this);
    formGeneral->addRow("主题:", m_darkThemeCheck);
    
    layoutGeneral->addLayout(formGeneral);
    layoutGeneral->addStretch();
    m_stackedWidget->addWidget(pageGeneral);

    // --- 页面 2: 网络设置 ---
    auto* pageNet = new QWidget();
    auto* layoutNet = new QVBoxLayout(pageNet);
    
    auto* lblNet = new QLabel("网络设置 (重启生效)");
    lblNet->setFont(titleFont);
    layoutNet->addWidget(lblNet);
    
    auto* lineNet = new QFrame();
    lineNet->setFrameShape(QFrame::HLine);
    lineNet->setFrameShadow(QFrame::Sunken);
    layoutNet->addWidget(lineNet);

    auto* formNet = new QFormLayout();
    formNet->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_httpPortSpin = new IntDragValueWidget(this);
    m_httpPortSpin->setRange(1024, 65535);
    formNet->addRow("HTTP 服务器端口:", m_httpPortSpin);

    m_webPasswordEdit = new QLineEdit(this);
    formNet->addRow("网页访问密码:", m_webPasswordEdit);

    m_extraFeedbackHostEdit = new QLineEdit(this);
    formNet->addRow("OSC 反馈主机 IP:", m_extraFeedbackHostEdit);

    m_extraFeedbackPortSpin = new IntDragValueWidget(this);
    m_extraFeedbackPortSpin->setRange(1024, 65535);
    formNet->addRow("OSC 反馈端口:", m_extraFeedbackPortSpin);

    m_extraControlPortSpin = new IntDragValueWidget(this);
    m_extraControlPortSpin->setRange(1024, 65535);
    formNet->addRow("OSC 控制端口:", m_extraControlPortSpin);

    m_oscInternalHostEdit = new QLineEdit(this);
    formNet->addRow("OSC 内部主机 IP:", m_oscInternalHostEdit);

    m_oscEnabledCheck = new QCheckBox("启用", this);
    formNet->addRow("OSC外部反馈/控制:", m_oscEnabledCheck);

    layoutNet->addLayout(formNet);
    layoutNet->addStretch();
    m_stackedWidget->addWidget(pageNet);

    // --- 页面 3: 日志设置 ---
    auto* pageLog = new QWidget();
    auto* layoutLog = new QVBoxLayout(pageLog);

    auto* lblLog = new QLabel("日志设置");
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
    formLog->addRow("最大日志显示条目:", m_maxLogEntriesSpin);

    layoutLog->addLayout(formLog);
    layoutLog->addStretch();
    m_stackedWidget->addWidget(pageLog);

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
    m_darkThemeCheck->setChecked(config.isDefaultDarkTheme());
    
    m_httpPortSpin->setValue(config.getHttpServerPort());
    m_extraFeedbackHostEdit->setText(config.getExtraFeedbackHost());
    m_extraFeedbackPortSpin->setValue(config.getExtraFeedbackPort());
    m_extraControlPortSpin->setValue(config.getExtraControlPort());
    m_oscInternalHostEdit->setText(config.getOscInternalControlHost());
    m_oscEnabledCheck->setChecked(config.isOscEnabled());
    m_webPasswordEdit->setText(config.getWebAccessPassword());
    // Log Settings
    m_maxLogEntriesSpin->setValue(config.getMaxLogEntries());
}

void SettingWidget::saveSettings() {
    QJsonObject obj;
    obj["MaxRecentFiles"] = m_maxRecentFilesSpin->value();
    obj["HttpServerPort"] = m_httpPortSpin->value();

    obj["ExtraFeedbackHost"] = m_extraFeedbackHostEdit->text();
    obj["ExtraFeedbackPort"] = m_extraFeedbackPortSpin->value();
    obj["ExtraControlPort"] = m_extraControlPortSpin->value();
    obj["OscInternalControlHost"] = m_oscInternalHostEdit->text();
    obj["DefaultDarkTheme"] = m_darkThemeCheck->isChecked();
    obj["OscEnabled"] = m_oscEnabledCheck->isChecked();
    obj["WebAccessPassword"] = m_webPasswordEdit->text();
    // Log Settings
    obj["MaxLogEntries"] = m_maxLogEntriesSpin->value();

    ConfigManager::instance().updateConfig(obj);
    
    QMessageBox::information(this, "设置", "设置已保存，部分更改将在重启后生效。");
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
