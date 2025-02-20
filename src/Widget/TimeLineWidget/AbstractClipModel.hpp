#ifndef ABSTRACTCLIPMODEL_HPP
#define ABSTRACTCLIPMODEL_HPP

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QJsonObject>
#include "timelinetypes.h"

class AbstractClipModel : public QObject {
    Q_OBJECT
public:
    explicit AbstractClipModel(int start, int end, const QString& type, QObject* parent = nullptr)
        : QObject(parent), m_start(start), m_end(end), m_type(type)  
        {
            // 是否可调整大小
            RESIZEABLE = true;
            // 是否显示小部件
            EMBEDWIDGET = true;
            // 是否显示边框
            SHOWBORDER = true;
        }

    virtual ~AbstractClipModel();

    // Getters
    int start() const ;
    int end() const ;
    QString type() const;
    int length() const ;
    int trackIndex() const ;
    bool isResizable() const ;
    bool isEmbedWidget() const ;
    bool isShowBorder() const ;
    // Setters
    void setStart(int start) ;
    void setEnd(int end);
    void setTrackIndex(int index);
    void setResizable(bool resizable) ;
    void setEmbedWidget(bool embedWidget) ;
    void setShowBorder(bool showBorder);
    // Save/Load
    virtual QJsonObject save() const ;

    virtual void load(const QJsonObject& json);

    virtual QVariant data(int role) const ;

    virtual QVariant currentData(int currentFrame) const;

Q_SIGNALS:
    void dataChanged();  // 添加数据变化信号

protected:
    int m_start;
    int m_end;
    QString m_type;
    int m_trackIndex;
    // 是否可调整大小
    bool RESIZEABLE;
    // 是否显示小部件
    bool EMBEDWIDGET;
    // 是否显示边框
    bool SHOWBORDER;
};

#endif // ABSTRACTCLIPMODEL_H 