#pragma once

#include <QApplication>
#include <QDir>
#include <QFileDialog>

#include "Common/AppConfig/ConfigManager.h"

namespace Nodes
{
inline QString pickOutputDirectory(const QString& currentDir)
{
    return QFileDialog::getExistingDirectory(
        QApplication::activeWindow(),
        QStringLiteral("选择输出文件夹"),
        currentDir);
}

inline QString resolveOutputPath(const QString& outputDir, const QString& fileName)
{
    const QString trimmedName = fileName.trimmed();
    const QString dir = outputDir.trimmed().isEmpty()
        ? AppConstants::MEDIA_LIBRARY_STORAGE_DIR
        : QDir(outputDir.trimmed()).absolutePath();

    QDir().mkpath(dir);
    return QDir(dir).filePath(trimmedName);
}
} // namespace Nodes
