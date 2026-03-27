//
// Created by WuBin on 2026/2/6.
//

#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>
#include "ConstantDefines.h"
#ifdef APPCONFIG_LIBRARY
#define APPCONFIG_EXPORT Q_DECL_EXPORT
#else
#define APPCONFIG_EXPORT Q_DECL_IMPORT
#endif

class APPCONFIG_EXPORT ConfigManager : public QObject
{
    Q_OBJECT

public:
    static ConfigManager& instance();

    // Getters
    int getMaxRecentFiles() const;
    int getHttpServerPort() const;
    QString getExtraFeedbackHost() const;
    int getExtraFeedbackPort() const;
    int getExtraControlPort() const;
    QString getOscInternalControlHost() const;
    bool isDefaultDarkTheme() const;
    int getMaxLogEntries() const;
    QStringList getRecentFiles() const;
    QString getCurrentFlowPath() const;
    bool isOscEnabled() const;
    QString getWebAccessPassword() const;
    /**
     * 函数级注释：将新路径加入最近文件列表
     * - 规则：去重后插入到首位；保留最多 MaxRecentFiles 个
     * - 同步写入配置文件
     */
    void addRecentFile(const QString& path);
    /**
     * 更新配置并保存
     * @param newConfig 包含新配置值的 JSON 对象
     */
    void updateConfig(const QJsonObject& newConfig);

    // Public for reset functionality
    void loadConfig();
    void createDefaultConfig();

private:
    ConfigManager();
    ~ConfigManager() override = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    void saveConfig();

    QString m_configPath;

    // Config values
    int m_maxRecentFiles;
    int m_httpServerPort;
    QString m_extraFeedbackHost;
    int m_extraFeedbackPort;
    int m_extraControlPort;
    QString m_oscInternalControlHost;
    bool m_oscEnabled;
    QString m_webAccessPassword;
    bool m_defaultDarkTheme;
    int m_MaxLogEntries;
    QStringList m_recentFiles;
    QString m_currentFlowPath;
};


