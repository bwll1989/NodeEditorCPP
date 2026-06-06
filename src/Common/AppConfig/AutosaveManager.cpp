//
// 自动保存管理器实现
//
#include "AutosaveManager.h"

#include "ConstantDefines.h"
#include "ProjectPersistence.h"

#include <QJsonDocument>

AutosaveManager::AutosaveManager(QObject* parent)
    : QObject(parent)
{
    _timer.setSingleShot(false);
    _timer.setInterval(AppConfigs::AUTOSAVE_INTERVAL_SECONDS * 1000);
    connect(&_timer, &QTimer::timeout, this, &AutosaveManager::onTimerTimeout);
}

void AutosaveManager::setSerializer(Serializer serializer)
{
    _serializer = std::move(serializer);
}

void AutosaveManager::setProjectPath(const QString& projectPath)
{
    _projectPath = projectPath;
}

void AutosaveManager::setEnabled(bool enabled)
{
    _enabled = enabled;
    if (!_enabled) {
        _timer.stop();
    } else if (_serializer) {
        _timer.start();
    }
}

void AutosaveManager::setIntervalSeconds(int seconds)
{
    _timer.setInterval(qMax(5, seconds) * 1000);
}

void AutosaveManager::setPaused(bool paused)
{
    _paused = paused;
}

bool AutosaveManager::saveRecoveryNow()
{
    if (_paused || !_enabled || !_serializer || _saving) {
        return false;
    }

    _saving = true;
    const QJsonObject json = _serializer();
    const QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    const QString recoveryPath = ProjectPersistence::autosaveFlowPath();
    const bool ok = ProjectPersistence::saveBytesAtomic(recoveryPath, data);
    if (ok) {
        ProjectPersistence::writeRecoveryMeta(
            ProjectPersistence::normalizeProjectPath(_projectPath),
            QDateTime::currentDateTime());
    }
    _saving = false;
    return ok;
}

void AutosaveManager::clearRecovery()
{
    ProjectPersistence::removeRecovery();
}

void AutosaveManager::start()
{
    if (_enabled && _serializer) {
        _timer.start();
    }
}

void AutosaveManager::onTimerTimeout()
{
    saveRecoveryNow();
}
