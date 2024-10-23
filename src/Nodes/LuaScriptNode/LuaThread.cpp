//
// Created by bwll1 on 2024/10/11.
//

#include "LuaThread.h"

#include "LuaDataModel.hpp"
#include "qdebug.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"
#include "iostream"

LuaThread::LuaThread(const QString &script,
                    QObject *ptr,
                    QObject *parent):
                    scriptContent(script),
                    nodeInstance(ptr) {
    init();
}
LuaThread::~LuaThread() {

}

void LuaThread::init() {
    // 初始化Lua状态机
    luaState = luaL_newstate();
    // 打开标准库
    luaL_openlibs(luaState);
    // 注册print函数
    lua_pushcfunction(luaState, luaPrint);
    lua_setglobal(luaState, "print");
    //注册QVariant转换类
    registerLuaQVariant(luaState);
    // 注册 LuaDataModel 类到 Lua
    registerLuaNode(luaState);
    // 创建 userdata 并将 LuaDataModel 实例压入 Lua
    auto instancePtr = static_cast<LuaDataModel*>(nodeInstance);
    //设置c++类实例全局名称为Node
    luabridge::setGlobal(luaState,instancePtr,"Node");
    // 尝试加载 Lua 脚本，解析返回的错误信息
}
void LuaThread::setCode(QString code) {
    scriptContent=code;
}
void LuaThread::run() {

    switch (luaL_loadstring(luaState, scriptContent.toStdString().c_str())) {
        case LUA_ERRSYNTAX: {
            qDebug()<<getError(lua_tostring(luaState,1));
            lua_pop(luaState, 1);  // 清除栈上的错误信息
            return;
        }
        case LUA_YIELD: {
            qDebug()<<getError(lua_tostring(luaState,1));
            lua_pop(luaState, 1);  // 清除栈上的错误信息
            return;
        }
        case LUA_ERRRUN: {
             qDebug()<<getError(lua_tostring(luaState,1));
            lua_pop(luaState, 1);  // 清除栈上的错误信息
            return;
        }
        case LUA_ERRERR: {
            qDebug()<<getError(lua_tostring(luaState,1));
            lua_pop(luaState, 1);  // 清除栈上的错误信息
            return;
        }
        case LUA_ERRMEM: {
             qDebug()<<getError(lua_tostring(luaState,1));
            lua_pop(luaState, 1);  // 清除栈上的错误信息
            return;
        }

    }
    // 尝试运行lua脚本，并返回错误信息
    if (lua_pcall(luaState, 0, LUA_MULTRET, 0)!=LUA_OK) {
        qDebug()<<getError(lua_tostring(luaState,1));
        lua_pop(luaState, 1);  // 清除栈上的错误信息
    }

}
// 将lua中的异常提示解析出来
QString LuaThread::getError(const char* error){
    std::string errorString(error);
    // 查找分隔符位置
    size_t firstColon = errorString.find(":");  // 第一个冒号（chunk 名称和行号之间）
    size_t secondColon = errorString.find(":", firstColon + 1);  // 第二个冒号（行号和错误信息之间）

    // 提取行号，位于第一个冒号和第二个冒号之间
    QString lineNumber = (firstColon != std::string::npos && secondColon != std::string::npos)
                             ? QString::fromStdString(errorString.substr(firstColon + 1, secondColon - firstColon - 1)).trimmed()
                             : "Unknown";

    // 提取错误类型，位于第二个冒号之后
    QString errorType = (secondColon != std::string::npos)
                            ? QString::fromStdString(errorString.substr(secondColon + 2)).trimmed()
                            : "Unknown";

    // 拼接成包含函数名、行号和错误信息的 QString
    QString finalErrorString = QString(" line %1 has error %2")
                                   .arg(lineNumber)
                                   .arg(errorType);
    return finalErrorString;
}


