#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QScrollArea>

namespace Nodes
{
    /**
     * @brief Snapshot 节点界面：互斥预设按钮 + 捕获/增删
     */
    class SnapshotInterface final : public QWidget
    {
        Q_OBJECT
    public:
        explicit SnapshotInterface(QWidget *parent = nullptr);

        void setPresets(const QStringList &names, int activeIndex);
        int activeIndex() const { return m_activeIndex; }

        void setStatusText(const QString &text);

    signals:
        void presetRecallRequested(int index);
        void captureRequested();
        void addPresetRequested();
        void removePresetRequested();

    private slots:
        void onPresetButtonClicked(int index);

    private:
        void rebuildButtons(const QStringList &names);

        QVBoxLayout *m_rootLayout = nullptr;
        QHBoxLayout *m_toolbarLayout = nullptr;
        QPushButton *m_addButton = nullptr;
        QPushButton *m_removeButton = nullptr;
        QPushButton *m_captureButton = nullptr;
        QLabel *m_statusLabel = nullptr;
        QScrollArea *m_scrollArea = nullptr;
        QWidget *m_buttonHost = nullptr;
        QVBoxLayout *m_buttonLayout = nullptr;
        QButtonGroup *m_buttonGroup = nullptr;

        QStringList m_presetNames;
        int m_activeIndex = 0;
        bool m_updatingUi = false;
    };
}
