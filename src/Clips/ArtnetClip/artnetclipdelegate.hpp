#ifndef ARTNETCLIPDELEGATE_HPP
#define ARTNETCLIPDELEGATE_HPP

#include "TimeLineWidget/AbstractClipDelegate.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QWidget>
class ArtnetClipDelegate : public AbstractClipDelegate {
    Q_OBJECT
public:
    explicit ArtnetClipDelegate(QObject *parent = nullptr) : AbstractClipDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
       
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QWidget* editor = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(editor);
        layout->setContentsMargins(5, 5, 5, 5);
        // Add file selection button
        QPushButton* button = new QPushButton("Select Video", editor);
        // button->setFlat(true);
        connect(button, &QPushButton::clicked, this, &ArtnetClipDelegate::openFileDialog);
        layout->addWidget(button);
        
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
            "Select Video File", 
            "", 
            "Video Files (*.mp4 *.avi *.mkv *.mov);;All Files (*)");
        if (!filePath.isEmpty()) {
            // Handle video file selection
            // You can emit a signal here to handle the selected file
        }
        
    }
};

#endif // ARTNETCLIPDELEGATE_HPP 