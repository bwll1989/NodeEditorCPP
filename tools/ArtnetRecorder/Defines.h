//
// Created by WuBin on 2025/8/19.
//
#include <QDate>
#pragma once
inline float FPS=50;                //录制频率
inline QString FilePrefix = QDate::currentDate().toString("yyyyMMdd");  // 文件前缀为当前日期
inline QString DefaultFilenameP = FilePrefix + QString(".dmx");  // 默认文件名
