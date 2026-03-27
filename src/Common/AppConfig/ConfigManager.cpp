//
// Created by WuBin on 2026/2/6.
//

#include "ConfigManager.h"
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QSettings>

ConfigManager& ConfigManager::instance()
{
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager()
    : m_maxRecentFiles(AppConfigs::MAX_RECENT_FILES)
    , m_httpServerPort(AppConfigs::HTTP_SERVER_PORT)
    , m_extraFeedbackHost(AppConfigs::EXTRA_FEEDBACK_HOST)
    , m_extraFeedbackPort(AppConfigs::EXTRA_FEEDBACK_PORT)
    , m_extraControlPort(AppConfigs::EXTRA_CONTROL_PORT)
    , m_oscInternalControlHost(AppConfigs::OSC_INTERNAL_CONTROL_HOST)
    , m_oscEnabled(AppConfigs::OSC_ENABLED)
    , m_webAccessPassword(AppConfigs::WEB_ACCESS_PASSWORD)
    , m_defaultDarkTheme(AppConfigs::DEFAULT_DARK_THEME)
    , m_MaxLogEntries(AppConfigs::MAX_LOG_ENTRIES)
{
    QString configDir = AppConstants::RECENT_FILES_STORAGE_DIR;
    QDir().mkpath(configDir);
    m_configPath = configDir + "/Settings.ini";
    
    loadConfig();
}

int ConfigManager::getMaxRecentFiles() const { return m_maxRecentFiles; }
int ConfigManager::getHttpServerPort() const { return m_httpServerPort; }
QString ConfigManager::getExtraFeedbackHost() const { return m_extraFeedbackHost; }
int ConfigManager::getExtraFeedbackPort() const { return m_extraFeedbackPort; }
int ConfigManager::getExtraControlPort() const { return m_extraControlPort; }
QString ConfigManager::getOscInternalControlHost() const { return m_oscInternalControlHost; }
bool ConfigManager::isDefaultDarkTheme() const { return m_defaultDarkTheme; }
int ConfigManager::getMaxLogEntries() const { return m_MaxLogEntries; }
QStringList ConfigManager::getRecentFiles() const { return m_recentFiles; }
QString ConfigManager::getCurrentFlowPath() const { return m_currentFlowPath; }
bool ConfigManager::isOscEnabled() const { return m_oscEnabled; }
QString ConfigManager::getWebAccessPassword() const { return m_webAccessPassword; }

void ConfigManager::addRecentFile(const QString& path)
{
    if (path.isEmpty())
        return;

    QString abs = path;
    abs = abs.replace("\\", "/"); // 统一分隔符
    m_recentFiles.removeAll(abs);
    m_recentFiles.prepend(abs);
    while (m_recentFiles.size() > m_maxRecentFiles) {
        m_recentFiles.removeLast();
    }
    saveConfig();
    m_currentFlowPath = abs;
}


void ConfigManager::loadConfig()
{
    QSettings settings(m_configPath, QSettings::IniFormat);

    // General
    m_maxRecentFiles = settings.value("General/MaxRecentFiles", AppConfigs::MAX_RECENT_FILES).toInt();
    m_defaultDarkTheme = settings.value("General/DefaultDarkTheme", AppConfigs::DEFAULT_DARK_THEME).toBool();
    m_recentFiles = settings.value("General/RecentFiles").toStringList();

    // Network
    m_httpServerPort = settings.value("Network/HttpServerPort", AppConfigs::HTTP_SERVER_PORT).toInt();
    m_extraFeedbackHost = settings.value("Network/ExtraFeedbackHost", AppConfigs::EXTRA_FEEDBACK_HOST).toString();
    m_extraFeedbackPort = settings.value("Network/ExtraFeedbackPort", AppConfigs::EXTRA_FEEDBACK_PORT).toInt();
    m_extraControlPort = settings.value("Network/ExtraControlPort", AppConfigs::EXTRA_CONTROL_PORT).toInt();
    m_oscInternalControlHost = settings.value("Network/OscInternalControlHost", AppConfigs::OSC_INTERNAL_CONTROL_HOST).toString();
    m_oscEnabled = settings.value("Network/OscEnabled", AppConfigs::OSC_ENABLED).toBool();
    m_webAccessPassword = settings.value("Network/WebAccessPassword", AppConfigs::WEB_ACCESS_PASSWORD).toString();
    // Log
    m_MaxLogEntries = settings.value("Log/MaxLogEntries", AppConfigs::MAX_LOG_ENTRIES).toInt();
}

void ConfigManager::createDefaultConfig()
{
    // Defaults are already set in constructor initialization list
    saveConfig();
}

void ConfigManager::saveConfig()
{
    QSettings settings(m_configPath, QSettings::IniFormat);

    // General
    settings.setValue("General/MaxRecentFiles", m_maxRecentFiles);
    settings.setValue("General/DefaultDarkTheme", m_defaultDarkTheme);
    settings.setValue("General/RecentFiles", m_recentFiles);

    // Network
    settings.setValue("Network/HttpServerPort", m_httpServerPort);
    settings.setValue("Network/ExtraFeedbackHost", m_extraFeedbackHost);
    settings.setValue("Network/ExtraFeedbackPort", m_extraFeedbackPort);
    settings.setValue("Network/ExtraControlPort", m_extraControlPort);
    settings.setValue("Network/OscInternalControlHost", m_oscInternalControlHost);
    settings.setValue("Network/OscEnabled", m_oscEnabled);
    settings.setValue("Network/WebAccessPassword", m_webAccessPassword);
    // Log
    settings.setValue("Log/MaxLogEntries", m_MaxLogEntries);

    settings.sync();
}

void ConfigManager::updateConfig(const QJsonObject& newConfig)
{
    if (newConfig.contains("MaxRecentFiles")) m_maxRecentFiles = newConfig["MaxRecentFiles"].toInt();
    if (newConfig.contains("HttpServerPort")) m_httpServerPort = newConfig["HttpServerPort"].toInt();
    if (newConfig.contains("ExtraFeedbackHost")) m_extraFeedbackHost = newConfig["ExtraFeedbackHost"].toString();
    if (newConfig.contains("ExtraFeedbackPort")) m_extraFeedbackPort = newConfig["ExtraFeedbackPort"].toInt();
    if (newConfig.contains("ExtraControlPort")) m_extraControlPort = newConfig["ExtraControlPort"].toInt();
    if (newConfig.contains("OscInternalControlHost")) m_oscInternalControlHost = newConfig["OscInternalControlHost"].toString();
    if (newConfig.contains("DefaultDarkTheme")) m_defaultDarkTheme = newConfig["DefaultDarkTheme"].toBool();
    if (newConfig.contains("MaxLogEntries")) m_MaxLogEntries = newConfig["MaxLogEntries"].toInt();
    if (newConfig.contains("OscEnabled")) m_oscEnabled = newConfig["OscEnabled"].toBool();
    if (newConfig.contains("WebAccessPassword")) m_webAccessPassword = newConfig["WebAccessPassword"].toString();

    saveConfig();
}

