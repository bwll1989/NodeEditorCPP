#ifndef STAGEWIDGET_HPP
#define STAGEWIDGET_HPP

#include <QWidget>
#include <QDockWidget>
#include <QQuickWidget>
#include <QVBoxLayout>
#include "Widget/TimeLineWidget/TimelineStageWidget/TimeLineStage.hpp"

class StageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StageWidget(QWidget *parent = nullptr);
    ~StageWidget() override;
   
    // 设置stage
    void setStage(TimeLineStage* stage);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupUI();

    QQuickWidget *m_quickWidget;
    QVBoxLayout *m_layout;
    TimeLineStage* m_stage;
};

#endif // STAGEWIDGET_HPP