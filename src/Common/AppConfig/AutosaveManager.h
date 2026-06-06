//
// 周期性自动保存到 Recovery 目录，不覆盖用户原始 .flow 文件
//
#pragma once

#include "ConfigManager.h"

#include <QObject>
#include <QTimer>
#include <functional>

/**
 * @brief 自动保存管理器
 *
 * 按固定间隔将项目快照写入 recovery，序列化逻辑由 setSerializer 注入。
 */
class APPCONFIG_EXPORT AutosaveManager : public QObject {
    Q_OBJECT
public:
    using Serializer = std::function<QJsonObject()>;

    explicit AutosaveManager(QObject* parent = nullptr);

    /** @brief 设置项目序列化回调 */
    void setSerializer(Serializer serializer);
    /** @brief 设置当前逻辑项目路径（写入 meta.projectPath） */
    void setProjectPath(const QString& projectPath);
    /** @brief 启用/禁用自动保存 */
    void setEnabled(bool enabled);
    /** @brief 周期性保存间隔（秒），最小 5 秒 */
    void setIntervalSeconds(int seconds);
    /** @brief 暂停自动保存（加载项目期间使用） */
    void setPaused(bool paused);

    /** @brief 立即写入 recovery */
    bool saveRecoveryNow();
    /** @brief 删除 recovery 文件 */
    void clearRecovery();
    /** @brief 启动周期性定时器 */
    void start();

private Q_SLOTS:
    void onTimerTimeout();

private:
    Serializer _serializer;
    QString _projectPath;
    QTimer _timer;
    bool _paused = false;
    bool _enabled = true;
    bool _saving = false;
};
