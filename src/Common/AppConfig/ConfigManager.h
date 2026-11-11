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
    int getMaxLogSaveEntries() const;
    QStringList getRecentFiles() const;
    QString getCurrentFlowPath() const;
    bool isOscEnabled() const;
    bool isMqttEnabled() const;
    QString getMqttHost() const;
    int getMqttPort() const;
    QString getMqttUsername() const;
    QString getMqttPassword() const;
    QString getMqttControlTopic() const;
    QString getMqttFeedbackTopic() const;
    QString getWebAccessPassword() const;
    bool isAutosaveEnabled() const;
    int getAutosaveIntervalSeconds() const;
    double getTimestampFrameRate() const;
    int getAudioOutputDelayFrames() const;
    QString getLicenseCode() const;
    /// 激活码（生成 .ft 激活请求用，填写一次后持久化显示）
    QString getActivationCode() const;
    /**
     * @brief 获取 Art-Net 多源合并策略（0=HTP 取大，1=LTP 后写优先）
     * @return int ArtnetMergeMode 枚举值
     */
    int getArtnetMergeMode() const;
    /**
     * @brief 获取 Art-Net 发送节拍频率（Hz，每周期重发所有活跃端点的当前状态）
     * @return int 频率值，钳制范围 1-100
     */
    int getArtnetSendFps() const;
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
    bool m_mqttEnabled;
    QString m_mqttHost;
    int m_mqttPort;
    QString m_mqttUsername;
    QString m_mqttPassword;
    QString m_mqttControlTopic;
    QString m_mqttFeedbackTopic;
    QString m_webAccessPassword;
    bool m_defaultDarkTheme;
    int m_MaxLogEntries;
    int m_MaxLogSaveEntries;
    bool m_autosaveEnabled;
    int m_autosaveIntervalSeconds;
    double m_timestampFrameRate;
    int m_audioOutputDelayFrames;
    QString m_licenseCode;
    QString m_activationCode; ///< 激活码（设置页填写后持久化）
    int m_artnetMergeMode; ///< Art-Net 多源合并策略（ArtnetMergeMode 枚举值）
    int m_artnetSendFps;   ///< Art-Net 发送节拍频率（Hz）
    QStringList m_recentFiles;
    QString m_currentFlowPath;
};

