#ifndef CLIPPROPERTY_HPP
#define CLIPPROPERTY_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPointer>
#include "AbstractClipModel.hpp"
#include "AbstractClipDelegate.hpp"
#include "timelinemodel.hpp"

class ClipProperty : public QWidget
{
    Q_OBJECT
public:
    explicit ClipProperty(AbstractClipModel* model, TimelineModel* timelineModel, QWidget *parent = nullptr);
    ~ClipProperty() override {
        if (m_delegate) {
            delete m_delegate;
        }
    }

Q_SIGNALS:
    void propertyChanged();

protected:
    void setupUI();
    void connectSignals();
    void setupDelegate();
    void updateUI();

protected slots:
    void onStartFrameChanged(int value);
    void onEndFrameChanged(int value);
    void onClipDataChanged();

private:
    AbstractClipModel* m_model;
    TimelineModel* m_timelineModel;
    QVBoxLayout* m_layout;
    QSpinBox* m_startFrameSpinBox;
    QSpinBox* m_endFrameSpinBox;
    QWidget* m_delegateWidget = nullptr;
    AbstractClipDelegate* m_delegate = nullptr;
};

#endif // CLIPPROPERTY_HPP 