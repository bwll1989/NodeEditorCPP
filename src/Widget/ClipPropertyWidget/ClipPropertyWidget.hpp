#ifndef CLIPPROPERTYWIDGET_HPP
#define CLIPPROPERTYWIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPointer>
#include "Widget/TimeLineWidget/AbstractClipModel.hpp"
#include "Widget/TimeLineWidget/timelinemodel.hpp"

class ClipPropertyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ClipPropertyWidget(QWidget *parent = nullptr);
    ~ClipPropertyWidget() override = default;

    void setClip(AbstractClipModel* clip, TimelineModel* model);

signals:
    void propertyChanged();

protected:
    void setupUI();
    void updateUI();
    void connectSignals();

protected slots:
    void onStartFrameChanged(int value);
    void onEndFrameChanged(int value);
    void onClipDataChanged();

private:
    QVBoxLayout* m_layout;
    QSpinBox* m_startFrameSpinBox;
    QSpinBox* m_endFrameSpinBox;
    QPointer<AbstractClipModel> m_currentClip;
    TimelineModel* m_model = nullptr;
};

#endif // CLIPPROPERTY_HPP 