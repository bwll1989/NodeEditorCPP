#ifndef TIMELINETOOLBAR_HPP
#define TIMELINETOOLBAR_HPP

#include <QToolBar>
#include <QAction>
#include <QSpinBox>
#include <QLabel>
#include "timelinestyle.hpp"
class TimelineToolbar : public QToolBar {
    Q_OBJECT
public:
    explicit TimelineToolbar(QWidget* parent = nullptr);
    ~TimelineToolbar() override = default;
    QAction* m_outputAction;

signals:
    void playClicked();
    void stopClicked();
    void pauseClicked();
    void loopToggled(bool enabled);
    void outputWindowToggled(bool show);
    void settingsClicked();
    void prevFrameClicked();
    void nextFrameClicked();
    void moveClipClicked(int dx);
    void deleteClipClicked();
    void zoomInClicked();
    void zoomOutClicked();
private:
    void createActions();
    void setupUI();

private:
    QAction* m_playAction;
    QAction* m_stopAction;
    QAction* m_pauseAction;
    QAction* m_loopAction;
    QAction* m_nextFrameAction;
    QAction* m_previousFrameAction;
    QAction* m_nextMediaAction;
    QAction* m_previousMediaAction;
    QAction* m_fullscreenAction;
    QAction* m_settingsAction;
    QAction* m_moveClipLeftAction;
    QAction* m_moveClipRightAction;
    QAction* m_deleteClipAction;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    bool m_isPlaying = false;
};

#endif // TIMELINETOOLBAR_HPP 