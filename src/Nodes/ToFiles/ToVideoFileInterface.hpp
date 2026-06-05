#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QWidget>

#include "Common/AppConfig/ConfigManager.h"
#include "ToFilePath.hpp"
#include "Common/GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace Nodes
{
class ToVideoFileInterface : public QWidget
{
    Q_OBJECT

public:
    explicit ToVideoFileInterface(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* layout = new QGridLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        _statusLabel = new QLabel(QStringLiteral("未录制"), this);
        _statusLabel->setAlignment(Qt::AlignCenter);

        _fileEdit = new QLineEdit(this);
        _fileEdit->setText(QStringLiteral("output.mp4"));
        _folderButton = new QPushButton(QStringLiteral("..."), this);
        _folderButton->setFixedWidth(28);
        _folderButton->setToolTip(defaultOutputDir());

        _fpsWidget = new FloatDragValueWidget(this);
        _fpsWidget->setRange(1.0, 120.0);
        _fpsWidget->setDecimals(2);
        _fpsWidget->setSingleStep(0.1);
        _fpsWidget->setSuffix(QStringLiteral(" fps"));
        _fpsWidget->setValue(25.0);

        _recordToggle = new QPushButton(QStringLiteral("开始"), this);
        _recordToggle->setCheckable(true);

        layout->addWidget(_statusLabel, 0, 0, 1, 2);
        layout->addWidget(_fileEdit, 1, 0, 1, 1);
        layout->addWidget(_folderButton, 1, 1, 1, 1);
        layout->addWidget(_fpsWidget, 2, 0, 1, 2);
        layout->addWidget(_recordToggle, 3, 0, 1, 2);

        connect(_recordToggle, &QPushButton::toggled, this, &ToVideoFileInterface::onRecordToggled);
        connect(_fileEdit, &QLineEdit::textChanged, this, &ToVideoFileInterface::fileTextChanged);
        connect(_folderButton, &QPushButton::clicked, this, &ToVideoFileInterface::onSelectFolder);
        connect(_fpsWidget, &FloatDragValueWidget::valueChanged, this, &ToVideoFileInterface::fpsChanged);

        setRecording(false);
    }

    QLineEdit* fileEdit() const { return _fileEdit; }
    FloatDragValueWidget* fpsWidget() const { return _fpsWidget; }

    QString outputDir() const { return m_outputDir; }

    void setFps(double fps)
    {
        QSignalBlocker blocker(_fpsWidget);
        _fpsWidget->setValue(fps);
    }

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
        if (_fpsWidget) {
            _fpsWidget->setEnabled(!recording);
        }
        updateRecordToggle(recording);
    }

signals:
    void recordingToggled(bool recording);
    void fileTextChanged(const QString& text);
    void outputDirChanged(const QString& dir);
    void fpsChanged(double fps);

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
    FloatDragValueWidget* _fpsWidget = nullptr;
    QPushButton* _recordToggle = nullptr;
    QString m_outputDir;
};
} // namespace Nodes
