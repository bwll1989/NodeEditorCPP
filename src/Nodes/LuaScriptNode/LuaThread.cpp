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

// 将 std::unordered_map<unsigned int, QVariant> 转换为 Lua table
static void pushUnorderedMapToLua(lua_State* L, const std::unordered_map<unsigned int, QVariant>& map) {
    // 创建一个新的 Lua table
    lua_newtable(L);
    // 遍历 unordered_map 并将每个键值对添加到 Lua table
    for (const auto& pair : map) {
        // 将键压入 Lua 栈 (unsigned int 键)
        lua_pushinteger(L, pair.first);

        // 处理 QVariant 类型并将值压入 Lua 栈
        const QVariant& value = pair.second;
        switch (value.typeId()) {
            case QVariant::Int:
                lua_pushinteger(L, value.toInt());
                break;
            case QVariant::Double:
                lua_pushnumber(L, value.toDouble());
                break;
            case QVariant::String:
                lua_pushstring(L, value.toString().toUtf8().constData());
                break;
            case QVariant::Bool:
                lua_pushboolean(L, value.toBool());
                break;
            // 可以添加更多类型的处理
            default:
                lua_pushnil(L); // 对于不支持的类型，推入 nil
            break;
        }

        // 将键值对添加到 table
        lua_settable(L, -3); // -3 表示 table 在栈中的索引
    }
}


LuaThread::LuaThread(const QString &script,
                    std::unordered_map<unsigned int, QVariant> in_dictionary,
                    unsigned int inCount,
                    unsigned int outCount,
                    QtNodes::NodeDelegateModel *ptr
                    ,QObject *parent):
                    scriptContent(script),
                    Inputs(in_dictionary),
                    inputsCount(inCount),
                    outputsCount(outCount),
                    nodeInstance(ptr){}
LuaThread::~LuaThread() {

}

void LuaThread::run() {
    // 初始化Lua状态机
    luaState = luaL_newstate();
    luaL_openlibs(luaState);  // 打开标准库
    lua_pushcfunction(luaState, luaPrint);
    lua_setglobal(luaState, "print");
    lua_pushinteger(luaState, inputsCount);
    lua_setglobal(luaState, "InputsCount");
    // 在 Lua 中将其命名为 InputsCountt
    lua_pushinteger(luaState, outputsCount);
    lua_setglobal(luaState, "OutputsCount");
    // 在 Lua 中将其命名为 OutputsCount
    pushUnorderedMapToLua(luaState,Inputs);
    // 将 Lua table 存入一个全局变量
    lua_setglobal(luaState, "NodeInputs");

    // luabridge::getGlobalNamespace(luaState)
    //     .beginClass<QtNodes::NodeDelegateModel>("MyClass")
    //         .addConstructor<void(*)()>()
    //         .addFunction("myMethod", &QtNodes::NodeDelegateModel::test)
    //     .endClass();
    //
    // // 创建 MyClass 实例并传递给 Lua
    // QtNodes::NodeDelegateModel** instancePtr = static_cast<QtNodes::NodeDelegateModel**>(lua_newuserdata(luaState, sizeof(QtNodes::NodeDelegateModel*)));
    // *instancePtr = nodeInstance;
    //
    // // 设置元表
    // luaL_getmetatable(luaState, "MyClass");
    // lua_setmetatable(luaState, -2); // 将元表应用于 userdata
    //
    // // 设置为全局变量
    // lua_setglobal(luaState, "myClass");
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


