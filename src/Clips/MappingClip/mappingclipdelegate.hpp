#ifndef MAPPINGCLIPDELEGATE_H
#define MAPPINGCLIPDELEGATE_H

#include "TimeLineWidget/AbstractClipDelegate.hpp"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class MappingClipDelegate : public AbstractClipDelegate {
    Q_OBJECT
public:
    explicit MappingClipDelegate(QObject *parent = nullptr) : AbstractClipDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
      
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QWidget* editor = new QWidget(parent);

        // Set semi-transparent background
        QString bgColorWithAlpha = QString("background-color: rgba(%1, %2, %3, 180);")
            .arg(ClipColor.red())
            .arg(ClipColor.green())
            .arg(ClipColor.blue());
        editor->setStyleSheet(bgColorWithAlpha);
        editor->setAttribute(Qt::WA_TranslucentBackground);
        editor->setAutoFillBackground(true);

        return editor;
    }
};

#endif // MAPPINGCLIPDELEGATE_H 