#ifndef CLIPMODEL_H
#define CLIPMODEL_H
#include "timelinetypes.h"
#include "QString"
#include"timelinestyle.hpp"
class TrackModel;


class ClipModel
{
public:
    ClipModel(int in, int out,QString name,TrackModel* parent,MediaType type = MediaType::CONTROL):m_pos(in),m_in(in),m_out(out),m_name(name), m_parent(parent),m_length(out),m_type(type)
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
