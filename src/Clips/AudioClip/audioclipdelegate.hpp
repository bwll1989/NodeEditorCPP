#ifndef AUDIOCLIPDELEGATE_H
#define AUDIOCLIPDELEGATE_H

#include "TimeLineWidget/abstractclipdelegate.hpp"
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
class AudioClipDelegate : public AbstractClipDelegate {
    Q_OBJECT
public:
    explicit AudioClipDelegate(QObject *parent = nullptr) : AbstractClipDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
      
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QWidget* editor = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(editor);
        layout->setContentsMargins(5, 5, 5, 5);
  
        // Add file selection button
        QPushButton* button = new QPushButton("Select Audio", editor);
        connect(button, &QPushButton::clicked, this, &AudioClipDelegate::openFileDialog);
        layout->addWidget(button);
        button->setFlat(true);

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

private slots:
    void openFileDialog() {
        QString filePath = QFileDialog::getOpenFileName(nullptr, 
            "Select Audio File", 
            "", 
            "Audio Files (*.mp3 *.wav *.ogg *.aac);;All Files (*)");
        if (!filePath.isEmpty()) {
            // Handle audio file selection
            // You can emit a signal here to handle the selected file
        }
    }
};

#endif // AUDIOCLIPDELEGATE_H 