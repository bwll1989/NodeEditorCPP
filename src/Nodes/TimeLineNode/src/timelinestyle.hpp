//
// Created by bwll1 on 2024/5/30.
//
#include "QColor"
#ifndef NODEEDITORCPP_STYLE_H
#define NODEEDITORCPP_STYLE_H
inline QColor clipColour("#0099CC");
inline QColor videoColour("#CCCCCC");
inline QColor audioColour("#FF6666");
inline QColor controlColour("#99CC66");
inline QColor cornerColour("#c8c8c8");


inline int trackHeight = 30;
inline int rulerHeight = 40;



inline QColor bgColour = QColor("#262626");
inline QColor fillColour = QColor("#202020");
inline QColor seperatorColour = QColor("#313131");
inline QColor rulerColour = QColor("#4F4F4F");
inline int textoffset = 13;
inline int baseTimeScale = 50;


inline double m_scale = 1;
inline int timescale = 1;
inline int trackwidth;//50*timescale;
inline int playheadwidth = 5;//real width is 2x this
inline int playheadheight = 16;
inline int playheadCornerHeight = playheadheight/4;
inline double fps=25.00;

inline bool drawtime=true;
#endif //NODEEDITORCPP_STYLE_H
