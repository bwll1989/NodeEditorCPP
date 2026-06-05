#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QWidget>

#include "Common/AppConfig/ConfigManager.h"
#include "ToFilePath.hpp"

namespace Nodes
{
class ToTextFileInterface : public QWidget
{
    Q_OBJECT

public:
    explicit ToTextFileInterface(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        _statusLabel = new QLabel(QStringLiteral("未录制"), this);
        _statusLabel->setAlignment(Qt::AlignCenter);

        _fileEdit = new QLineEdit(this);
        _fileEdit->setText(QStringLiteral("output.json"));
        _folderButton = new QPushButton(QStringLiteral("..."), this);
        _folderButton->setFixedWidth(28);
        _folderButton->setToolTip(defaultOutputDir());

        _recordToggle = new QPushButton(QStringLiteral("开始"), this);
        _recordToggle->setCheckable(true);

        layout->addWidget(_statusLabel, 0, 0, 1, 2);
        layout->addWidget(_fileEdit, 1, 0, 1, 1);
        layout->addWidget(_folderButton, 1, 1, 1, 1);
        layout->addWidget(_recordToggle, 2, 0, 1, 2);

        connect(_recordToggle, &QPushButton::toggled, this, &ToTextFileInterface::onRecordToggled);
        connect(_fileEdit, &QLineEdit::textChanged, this, &ToTextFileInterface::fileTextChanged);
        connect(_folderButton, &QPushButton::clicked, this, &ToTextFileInterface::onSelectFolder);

        setRecording(false);
    }

    QLineEdit* fileEdit() const { return _fileEdit; }

    QString outputDir() const { return m_outputDir; }

    void setOutputDir(const QString& dir)
    {
        m_outputDir = dir.trimmed();
        updateFolderToolTip();
    }

    void setRecording(bool recording)
    {
        if (recording) {
            _statusLabel->setText(QStringLiteral("录制中"));
            _statusLabel->setStyleSheet(QStringLiteral("color: #e74c3c; font-weight: bold;"));
        } else {
            _statusLabel->setText(QStringLiteral("未录制"));
            _statusLabel->setStyleSheet(QStringLiteral("color: #95a5a6; font-weight: bold;"));
        }
        updateRecordToggle(recording);
    }

signals:
    void recordingToggled(bool recording);
    void fileTextChanged(const QString& text);
    void outputDirChanged(const QString& dir);

private:
    static QString defaultOutputDir()
    {
        return AppConstants::MEDIA_LIBRARY_STORAGE_DIR;
    }

    void updateRecordToggle(bool recording)
    {
        if (!_recordToggle) {
            return;
        }
        QSignalBlocker blocker(_recordToggle);
        _recordToggle->setChecked(recording);
        _recordToggle->setText(recording ? QStringLiteral("停止") : QStringLiteral("开始"));
    }

    void updateFolderToolTip()
    {
        const QString dir = m_outputDir.isEmpty() ? defaultOutputDir() : m_outputDir;
        _folderButton->setToolTip(QStringLiteral("输出目录: %1").arg(dir));
    }

    void onRecordToggled(bool checked)
    {
        _recordToggle->setText(checked ? QStringLiteral("停止") : QStringLiteral("开始"));
        emit recordingToggled(checked);
    }

    void onSelectFolder()
    {
        const QString currentDir = m_outputDir.isEmpty() ? defaultOutputDir() : m_outputDir;
        const QString selectedDir = pickOutputDirectory(currentDir);

        if (selectedDir.isEmpty()) {
            return;
        }

        setOutputDir(selectedDir);
        emit outputDirChanged(selectedDir);
    }

public:
    QLabel* _statusLabel = nullptr;
    QLineEdit* _fileEdit = nullptr;
    QPushButton* _folderButton = nullptr;
    QPushButton* _recordToggle = nullptr;
    QString m_outputDir;
};
} // namespace Nodes
