//
// Created by bwll1 on 2024/10/11.
//

#include "LuaThread.h"
#include "qdebug.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "LuaBridge/LuaBridge.h"
#include "iostream"
static int luaPrint(lua_State* L) {
    int nargs = lua_gettop(L);
    QString output;
    for (int i = 1; i <= nargs; ++i) {
        if (i > 1) output += " ";
        if (lua_isstring(L, i)) {
            output += QString::fromUtf8(lua_tostring(L, i));
        }
        else if (lua_isnumber(L, i)) {
            output += QString::number(lua_tonumber(L, i));
        }
        else if (lua_isboolean(L, i)) {
            output += lua_toboolean(L, i) ? "true" : "false";
        }
        else if (lua_isnil(L,i)) {
            output += "nil";
        }
        else if (lua_isuserdata(L,i)) {
            output += "userdata";
        }
        else if(lua_iscfunction(L,i)){
            output += "function";
        }
        else if(lua_istable(L,i)){
            output += "table";
        }
        else {
            output += "Unsupported type";
        }
    }
    qDebug() << output;
    return 0;
}
static int luaErrorHandler(lua_State* L) {
    // 使用 debug.traceback 获取堆栈信息
    const char* msg = lua_tostring(L, 1);
    if (msg != nullptr) {
        luaL_traceback(L, L, msg, 1);
    } else {
        lua_pushliteral(L, "(no error message)");
    }
    return 1;
}
LuaThread::LuaThread(const QString& scriptContent, QObject* parent)
    : QThread(parent), scriptContent(scriptContent) {}

LuaThread::~LuaThread() {
    if (luaState) {
        lua_close(luaState);
    }
}


void LuaThread::run() {
    // 初始化Lua状态机
    luaState = luaL_newstate();
    luaL_openlibs(luaState);  // 打开标准库
    lua_pushcfunction(luaState, luaPrint);
    lua_setglobal(luaState, "print");
    // 尝试加载 Lua 脚本，并返回错误信息
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


