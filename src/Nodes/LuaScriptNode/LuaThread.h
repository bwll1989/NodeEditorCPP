//
// Created by bwll1 on 2024/10/11.
//

#ifndef NODEEDITORCPP_LUATHREAD_H
#define NODEEDITORCPP_LUATHREAD_H
#include <QThread>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <iostream>
#include <unordered_map>
#include "QVariant"
#include <QtNodes/NodeDelegateModel>
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class LuaThread : public QThread {
    Q_OBJECT
public:
    LuaThread(const QString& scriptContent,
        std::unordered_map<unsigned int, QVariant> in_dictionary,
        unsigned int inCount,
        unsigned int outCount,
        QtNodes::NodeDelegateModel *ptr,
        QObject* parent = nullptr);
    // 重写析构函数以确保正确退出线程
    ~LuaThread() ;
    QString getError(const char* err);
protected:
    void run() override ;

private:
    lua_State* luaState = nullptr;  // Lua的状态机
    QString scriptContent;  // Lua脚本的内容
    std::unordered_map<unsigned int, QVariant> Inputs; //节点输入值
    unsigned int inputsCount;
    unsigned int outputsCount;
    QtNodes::NodeDelegateModel *nodeInstance;
};
#endif //NODEEDITORCPP_LUATHREAD_H
