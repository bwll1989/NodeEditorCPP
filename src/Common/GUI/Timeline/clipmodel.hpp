#ifndef CLIPMODEL_H
#define CLIPMODEL_H
#include "timelinetypes.h"
#include "QString"
#include"timelinestyle.hpp"
#include <QJsonObject>
#include <QJsonArray>
#include <QAbstractItemModel>
class TrackModel;


class ClipModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ClipModel(int in, int out,QString name,TrackModel* parent,MediaType type = MediaType::CONTROL):m_pos(in),m_in(in),m_out(out),m_name(name), m_parent(parent),m_length(out),m_type(type)
    {
        m_length=out-in;
    }

    //copy constructor
    ClipModel(const ClipModel& other)
        : m_pos(other.m_pos), m_in(other.m_in), m_out(other.m_out), m_parent(other.m_parent), m_length(other.m_length), m_type(other.m_type) {
    }

    ~ClipModel(){
    }

    //copy assignment operator
    ClipModel& operator=(const ClipModel& other) {
        if (this != &other) {
            m_pos = other.m_pos;
            m_in = other.m_in;
            m_out = other.m_out;
            m_parent = other.m_parent;
            m_length = other.m_length;
            m_type = other.m_type;
            m_name = other.m_name;

        }
        return *this;
    }

    TrackModel* Parent(){
        return m_parent;
    };
    QModelIndex index(int row, int column,const QModelIndex &parent = QModelIndex()) const override{
        return QModelIndex();
    };
    QModelIndex parent(const QModelIndex &child) const override{
        return QModelIndex();
    }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override{
        return 1;
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override{
        return 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override{
        return QVariant();
    }
    int pos() const{
        return m_pos;
    }

    void setPos(int newPos){
        m_pos = newPos;
    }

    void setParent(TrackModel *newParent){
        m_parent = newParent;
    }

    int in() const{
        return m_in;
    }

    void setIn(int newIn){
        m_in = newIn;
    }

    int out() const{
        return m_out;
    }

    void setOut(int newOut){
        m_out = newOut;
    }

    QString name() const{
        return m_name;
    }

    void setName(QString name){
        m_name=name;
    }

    int length() const{
        return m_out-m_in;
    }

    MediaType type() const{
        return m_type;
    }


    QJsonObject save() const
    {
        QJsonObject clipJson;
        clipJson["pos"]=m_pos;
        clipJson["in"]=m_in;
        clipJson["out"]=m_out;
        clipJson["length"]=m_length;
        clipJson["name"]=m_name;
        QString typeString;
        switch (m_type) {
            case MediaType::VIDEO:
                typeString = "VIDEO";
                break;
            case MediaType::AUDIO:
                typeString = "AUDIO";
                break;
            case MediaType::CONTROL:
                typeString = "CONTROL";
                break;
        }
        clipJson["type"] = typeString;
        return clipJson;
    }

    void load() const
    {

    }


private:
    TrackModel* m_parent;
    MediaType m_type;
    //store resoruce here
    int m_pos;//position on timeline of first frame
    int m_in;//start time of clip from resource
    int m_out;//end time of clip from resource
    int m_length;//length of orignal media
    QString m_name; //clip name

};

#endif // CLIPMODEL_H
