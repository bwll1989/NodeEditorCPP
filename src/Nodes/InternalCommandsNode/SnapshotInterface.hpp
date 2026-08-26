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
     * @brief Snapshot 节点界面：预设列表（召回 / 更新 / 删除）+ 添加
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
        /// 将当前选中节点状态写入指定预设（无需先选中该预设）
        void presetUpdateRequested(int index);
        /// 删除指定预设
        void presetRemoveRequested(int index);
        void addPresetRequested();

    private slots:
        void onPresetButtonClicked(int index);

    private:
        void rebuildButtons(const QStringList &names);

        QVBoxLayout *m_rootLayout = nullptr;
        QHBoxLayout *m_toolbarLayout = nullptr;
        QPushButton *m_addButton = nullptr;
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
