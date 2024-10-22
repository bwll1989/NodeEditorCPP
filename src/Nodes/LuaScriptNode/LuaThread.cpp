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
// 函数：将 QVariant 转换为 Lua 支持的类型并推送到 Lua 栈上
void QVariantToLua(lua_State* L, const QVariant& variant) {
    using namespace luabridge;

    switch (variant.typeId()) {
        case QVariant::Int:
            lua_pushinteger(L, variant.toInt());  // 推送 int 到 Lua 栈
        break;
        case QVariant::Double:
            lua_pushnumber(L, variant.toDouble());  // 推送 double 到 Lua 栈
        break;
        case QVariant::String:
            lua_pushstring(L, variant.toString().toStdString().c_str());  // 推送 string 到 Lua 栈
        break;
        case QVariant::Bool:
            lua_pushboolean(L, variant.toBool());  // 推送 bool 到 Lua 栈
        break;
        // 根据需要添加更多类型支持
        default:
            lua_pushnil(L);  // 未知类型推送 nil 到 Lua 栈
        std::cerr << "Unsupported QVariant type\n";
    }
}
// C++ 注册到 Lua 的函数，接受参数并返回 QVariant
static QVariant getSomeValue(int input) {
    if (input == 1) {
        return QVariant(42);  // 返回 int
    } else if (input == 2) {
        return QVariant(3.14);  // 返回 double
    } else {
        return QVariant("Hello from C++!");  // 返回 QString
    }
}

LuaThread::LuaThread(const QString &script,
                    QObject *ptr
                    ,QObject *parent):
                    scriptContent(script),
                    nodeInstance(ptr){}
LuaThread::~LuaThread() {

}

void LuaThread::run() {
    // 初始化Lua状态机
    luaState = luaL_newstate();
    luaL_openlibs(luaState);  // 打开标准库
    lua_pushcfunction(luaState, luaPrint);
    lua_setglobal(luaState, "print");
    // luabridge::getGlobalNamespace(luaState)
    //      .addFunction("getSomeValue", [](lua_State* L) {
    //          int input = luabridge::Stack<int>::get(L, 1);  // 从 Lua 获取第一个参数
    //          QVariant value = getSomeValue(input);          // 调用 C++ 函数返回 QVariant
    //          QVariantToLua(L, value);                       // 将 QVariant 转换并推送到 Lua 栈上
    //          return 1;  // 返回一个值
    //      });
    registerLuaQVariant(luaState);
    // 注册 LuaDataModel 类到 Lua
    registerLuaNode(luaState);
    // 创建 userdata 并将 Qt 实例压入 Lua
    auto instancePtr = static_cast<LuaDataModel*>(nodeInstance);
    luabridge::setGlobal(luaState,instancePtr,"Node");

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


