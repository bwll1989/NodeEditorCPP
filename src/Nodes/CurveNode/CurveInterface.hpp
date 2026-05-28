#pragma once
//
// Created by Administrator on 2023/12/13.
//
#include "QWidget"
#include "QGridLayout"
#include "QPushButton"
#include "QCheckBox"
#include "QTimeEdit"
#include "QFont"
#include "QEvent"
#include "QSignalBlocker"

using namespace std;
namespace Nodes {
    class CurveInterface : public QWidget {
        Q_OBJECT
    public:
        explicit CurveInterface(QWidget *parent = nullptr)
            : QWidget(parent)
        {
            setLayout(mainlayout);
            layout()->setContentsMargins(0, 0, 0, 0);

            statusLabel = new QTimeEdit(this);
            statusLabel->setDisplayFormat("hh:mm:ss:zzz");
            statusLabel->setAlignment(Qt::AlignCenter);
            statusLabel->setTime(QTime(0, 0, 0, 0));
            statusLabel->setReadOnly(true);
            statusLabel->setButtonSymbols(QAbstractSpinBox::NoButtons);
            QFont font = statusLabel->font();
            font.setBold(true);
            font.setPointSize(font.pointSize() + 8);
            statusLabel->setFont(font);

            startButton = new QPushButton(tr("启动"), this);
            startButton->setCheckable(true);
            loopCheckBox = new QCheckBox(tr("循环"), this);
            editButton = new QPushButton(tr("编辑曲线"), this);
            editButton->setToolTip(tr("打开曲线编辑窗口"));

            mainlayout->addWidget(statusLabel, 0, 0, 1, 3);
            mainlayout->addWidget(startButton, 1, 0, 1, 2);
            mainlayout->addWidget(loopCheckBox, 1, 2, 1, 1);
            mainlayout->addWidget(editButton, 2, 0, 1, 3);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setMinimumSize(180, 120);

            connect(editButton, &QPushButton::clicked, this, &CurveInterface::toggleEditorMode);
        }

        void setEditorWidget(QWidget *editor) {
            if (m_editor == editor) {
                return;
            }
            if (m_editor) {
                m_editor->removeEventFilter(this);
            }
            m_editor = editor;
            if (m_editor) {
                m_editor->hide();
                m_editor->installEventFilter(this);
            }
        }

        QWidget *editorWidget() const { return m_editor; }

        void setPlayheadTime(const QTime &time) {
            if (statusLabel) {
                statusLabel->setTime(time);
            }
        }

        void setPlayingState(bool playing) {
            if (!startButton) {
                return;
            }
            QSignalBlocker blocker(startButton);
            startButton->setChecked(playing);
            startButton->setText(playing ? tr("停止") : tr("启动"));
        }

        void setLoopState(bool loop) {
            if (!loopCheckBox) {
                return;
            }
            QSignalBlocker blocker(loopCheckBox);
            loopCheckBox->setChecked(loop);
        }

        void toggleEditorMode() {
            if (!m_editor) {
                return;
            }
            if (m_editor->isVisible() && (m_editor->windowFlags() & Qt::Window)) {
                hideEditorWindow();
                return;
            }
            m_editor->setParent(nullptr);
            m_editor->setWindowTitle(tr("曲线编辑器"));
            m_editor->setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
            m_editor->setAttribute(Qt::WA_DeleteOnClose, false);
            m_editor->setAttribute(Qt::WA_QuitOnClose, false);
            m_editor->resize(900, 400);
            m_editor->show();
            m_editor->activateWindow();
            m_editor->raise();
            editButton->setText(tr("隐藏编辑器"));
        }

        ~CurveInterface() override {
            if (m_editor) {
                m_editor->hide();
                m_editor->setParent(this);
            }
        }

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override {
            if (watched == m_editor && event->type() == QEvent::Close) {
                hideEditorWindow();
                return true;
            }
            return QWidget::eventFilter(watched, event);
        }

    private:
        void hideEditorWindow() {
            if (!m_editor) {
                return;
            }
            m_editor->hide();
            m_editor->setParent(this);
            editButton->setText(tr("编辑曲线"));
        }

    public:
        QGridLayout *mainlayout = new QGridLayout();
        QPushButton *editButton = nullptr;
        QPushButton *startButton = nullptr;
        QCheckBox *loopCheckBox = nullptr;
        QTimeEdit *statusLabel = nullptr;

    private:
        QWidget *m_editor = nullptr;
    };
}
