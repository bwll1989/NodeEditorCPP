/**
 * @file AudioListDecoderInterface.hpp
 * @brief QListView 播放列表：右上角添加；每行 ↑↓ / 文件选择 / 增益 / 删除
 */
#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QListView>
#include <QAbstractItemView>
#include <QStandardItemModel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QSignalBlocker>

#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

namespace Nodes
{
    /**
     * 曲目行代理控件（布局对齐设计稿）：
     * [↑] [↓] [SelectorComboBox ……] [增益 dB] [×]
     */
    class PlaylistTrackRowWidget : public QFrame
    {
        Q_OBJECT
    public:
        explicit PlaylistTrackRowWidget(int index,
                                        const QString &path,
                                        double gainDb,
                                        bool canMoveUp,
                                        bool canMoveDown,
                                        QWidget *parent = nullptr)
            : QFrame(parent)
            , m_index(index)
        {
            setFrameShape(QFrame::NoFrame);
            setObjectName(QStringLiteral("PlaylistTrackRow"));

            auto *layout = new QHBoxLayout(this);
            layout->setContentsMargins(2, 2, 2, 2);
            layout->setSpacing(4);

            upButton->setText(QStringLiteral("↑"));
            upButton->setFixedSize(28, 28);
            upButton->setEnabled(canMoveUp);
            upButton->setToolTip(QStringLiteral("上移"));
            upButton->setFocusPolicy(Qt::NoFocus);

            downButton->setText(QStringLiteral("↓"));
            downButton->setFixedSize(28, 28);
            downButton->setEnabled(canMoveDown);
            downButton->setToolTip(QStringLiteral("下移"));
            downButton->setFocusPolicy(Qt::NoFocus);

            fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Audio, this);
            fileSelectComboBox->setPlaceholderText(QStringLiteral("选择音频…"));
            {
                QSignalBlocker b(fileSelectComboBox);
                if (!path.isEmpty()) {
                    fileSelectComboBox->setCurrentValue(path);
                }
            }

            gainSlider->setRange(-100, 20);
            gainSlider->setSingleStep(0.5);
            gainSlider->setDecimals(2);
            gainSlider->setSuffix(QStringLiteral(" dB"));
            gainSlider->setMinimumWidth(88);
            gainSlider->setMaximumWidth(110);
            gainSlider->setToolTip(QStringLiteral("曲目增益"));
            {
                QSignalBlocker b(gainSlider);
                gainSlider->setValue(gainDb);
            }

            removeButton->setText(QStringLiteral("×"));
            removeButton->setFixedSize(28, 28);
            removeButton->setToolTip(QStringLiteral("删除"));
            removeButton->setFocusPolicy(Qt::NoFocus);

            layout->addWidget(upButton);
            layout->addWidget(downButton);
            layout->addWidget(fileSelectComboBox, 1);
            layout->addWidget(gainSlider);
            layout->addWidget(removeButton);

            connect(upButton, &QPushButton::clicked, this, &PlaylistTrackRowWidget::moveUpRequested);
            connect(downButton, &QPushButton::clicked, this, &PlaylistTrackRowWidget::moveDownRequested);
            connect(removeButton, &QPushButton::clicked, this, &PlaylistTrackRowWidget::removeRequested);
            connect(fileSelectComboBox, &SelectorComboBox::selectionChanged,
                    this, &PlaylistTrackRowWidget::pathChanged);
            connect(gainSlider, &FloatDragValueWidget::valueChanged,
                    this, &PlaylistTrackRowWidget::gainChanged);
        }

        int trackIndex() const { return m_index; }

        void setPlayingHighlight(bool playing)
        {
            setStyleSheet(playing
                              ? QStringLiteral(
                                    "#PlaylistTrackRow { background: rgba(40,120,60,80); border-radius: 3px; }")
                              : QString());
        }

        void setMoveEnabled(bool canMoveUp, bool canMoveDown)
        {
            upButton->setEnabled(canMoveUp);
            downButton->setEnabled(canMoveDown);
        }

    signals:
        void moveUpRequested();
        void moveDownRequested();
        void removeRequested();
        void pathChanged(const QString &path);
        void gainChanged(double gainDb);

    private:
        int m_index = 0;
        QPushButton *upButton = new QPushButton(this);
        QPushButton *downButton = new QPushButton(this);
        SelectorComboBox *fileSelectComboBox = nullptr;
        FloatDragValueWidget *gainSlider = new FloatDragValueWidget(this);
        QPushButton *removeButton = new QPushButton(this);
    };

    class AudioListDecoderInterface : public QWidget
    {
    public:
        static constexpr int kRoleTrackIndex = Qt::UserRole;

        explicit AudioListDecoderInterface(QWidget *parent = nullptr)
        {
            auto *layout = new QVBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(6);

            const auto addLabeledRow = [layout](const QString &labelText, QWidget *editor) {
                auto *row = new QWidget();
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                rowLayout->addWidget(new QLabel(labelText));
                rowLayout->addWidget(editor, 1);
                layout->addWidget(row);
            };

            indexSpinBox->setMinimum(0);
            indexSpinBox->setMaximum(0);
            indexSpinBox->setValue(0);
            indexSpinBox->setToolTip(QStringLiteral("PLAY 将播放的列表序号；列表编辑与自动切曲不改变此值"));

            playButton->setCheckable(true);
            playButton->setAutoExclusive(false);
            playButton->setText(QStringLiteral("Play"));
            playButton->setToolTip(QStringLiteral("勾选播放 / 取消停止（按当前 Index）"));

            loopCheckBox->setText(QStringLiteral("单曲循环"));
            loopCheckBox->setChecked(false);
            loopCheckBox->setToolTip(QStringLiteral("当前曲目循环（与列表循环互斥）"));

            listLoopCheckBox->setText(QStringLiteral("列表循环"));
            listLoopCheckBox->setChecked(false);
            listLoopCheckBox->setToolTip(QStringLiteral("列表播完后从第一首继续（与单曲循环互斥）"));

            statusLabel->setWordWrap(true);
            statusLabel->setStyleSheet(QStringLiteral("color: gray;"));
            statusLabel->setText(QStringLiteral("状态: —"));

            addButton->setText(QStringLiteral("添加"));
            addButton->setToolTip(QStringLiteral("在列表末尾新增一条空白曲目"));
            addButton->setFixedHeight(24);

            addLabeledRow(QStringLiteral("Index:"), indexSpinBox);
            layout->addWidget(playButton);

            auto *loopRow = new QWidget();
            auto *loopLayout = new QHBoxLayout(loopRow);
            loopLayout->setContentsMargins(0, 0, 0, 0);
            loopLayout->setSpacing(8);
            loopLayout->addWidget(loopCheckBox);
            loopLayout->addWidget(listLoopCheckBox);
            loopLayout->addStretch(1);
            layout->addWidget(loopRow);

            layout->addWidget(statusLabel);

            // 列表标题行：左侧标题，右侧添加
            auto *listHeader = new QWidget();
            auto *listHeaderLayout = new QHBoxLayout(listHeader);
            listHeaderLayout->setContentsMargins(0, 0, 0, 0);
            listHeaderLayout->setSpacing(8);
            listHeaderLayout->addWidget(new QLabel(QStringLiteral("播放列表:")));
            listHeaderLayout->addStretch(1);
            listHeaderLayout->addWidget(addButton);
            layout->addWidget(listHeader);

            playlistModel = new QStandardItemModel(this);
            playlistView->setModel(playlistModel);
            playlistView->setMinimumHeight(160);
            playlistView->setSelectionMode(QAbstractItemView::SingleSelection);
            playlistView->setEditTriggers(QAbstractItemView::NoEditTriggers);
            playlistView->setUniformItemSizes(false);
            playlistView->setSpacing(2);
            playlistView->setToolTip(QStringLiteral("双击曲目播放（不改 Index）"));
            layout->addWidget(playlistView, 1);

            setMinimumSize(360, 420);
        }

        void setStatusText(const QString &text)
        {
            statusLabel->setText(QStringLiteral("状态: %1").arg(text));
        }

        void syncPlayButton(bool playing)
        {
            QSignalBlocker b(playButton);
            playButton->setAutoExclusive(false);
            playButton->setChecked(playing);
            playButton->setText(playing ? QStringLiteral("Stop") : QStringLiteral("Play"));
        }

        QSpinBox *indexSpinBox = new QSpinBox();
        QPushButton *playButton = new QPushButton();
        QCheckBox *loopCheckBox = new QCheckBox();
        QCheckBox *listLoopCheckBox = new QCheckBox();
        QPushButton *addButton = new QPushButton();
        QLabel *statusLabel = new QLabel();
        QListView *playlistView = new QListView();
        QStandardItemModel *playlistModel = nullptr;
    };
}
