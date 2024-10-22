//
// Created by WuBin on 24-10-20.
//
#include <iostream>
#include <QString>
#include <QVariant>
#include "LuaBridge/LuaBridge.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
//lua不支持QVariant数据类型，需要将QVariant数据转换为lua支持的类型
class LuaQVariant {
public:
    LuaQVariant() = default;

    // 使用 int 构造
    LuaQVariant(const int value) {
        variant_=QVariant::fromValue(value);
    }

    // 使用 double 构造
    LuaQVariant(const double value) : variant_(value) {
        variant_=QVariant::fromValue(value);
    }
    // 使用 bool 构造
    LuaQVariant(const bool value){
        variant_=QVariant::fromValue(value);
    }

    // 使用 QString 构造
    LuaQVariant(const std::string& value)  {
        variant_=QVariant::fromValue(QString::fromStdString(value));
    }

    // 从 QVariant 构造
    LuaQVariant(const QVariant& variant) : variant_(variant) {}

    // 转换为 int
    int toInt() const {
        if (variant_.canConvert<int>()) {
            return variant_.toInt();
        }
        return 0;
    }

    bool toBool() const {
        if (variant_.canConvert<bool>()) {
            return variant_.toBool();
        }
        return false;
    }
    // 转换为 double
    double toDouble() const {
        if (variant_.canConvert<double>()) {
            return variant_.toDouble();
        }
        return 0.0;
    }


    // 转换为 string (QString)
    std::string toString() const {
        if (variant_.canConvert<QString>()) {
            return variant_.toString().toStdString();
        }
        return "";
    }

    // 获取内部的 QVariant
    const QVariant& getVariant() const {
        return variant_;
    }

    // 支持从不同类型创建QVariant
    static LuaQVariant fromInt(int value) { return LuaQVariant(value); }
    static LuaQVariant fromBool(bool value) { return LuaQVariant(value); }
    static LuaQVariant fromDouble(double value) { return LuaQVariant(value); }
    static LuaQVariant fromString(const QString& value) { return LuaQVariant(value); }
private:
    QVariant variant_;  // 内部的 QVariant 实例
};
static void registerLuaQVariant(lua_State* L) {
    using namespace luabridge;
    getGlobalNamespace(L)
       .beginClass<LuaQVariant>("QVariant")
       .addConstructor<void(*)()>()  // 默认构造函数
       .addConstructor<void(*)(int)>()  // 支持 int 构造
       .addConstructor<void(*)(double)>()  // 支持 double 构造
        .addConstructor<void(*)(bool)>()  // 支持 bool 构造
       .addConstructor<void(*)(const std::string&)>()  // 支持 string 构造
    // 静态函数用于从其他类型创建QVariant
        .addStaticFunction("fromInt", &LuaQVariant::fromInt)
        .addStaticFunction("fromBool", &LuaQVariant::fromBool)
        .addStaticFunction("fromDouble", &LuaQVariant::fromDouble)
        .addStaticFunction("fromString", &LuaQVariant::fromString)
            // 类型转换方法
       .addFunction("toInt", &LuaQVariant::toInt)
        .addFunction("toBool", &LuaQVariant::toBool)
       .addFunction("toDouble", &LuaQVariant::toDouble)
       .addFunction("toString", &LuaQVariant::toString)

       .endClass();
}


//
// void registerQVariant(lua_State* L) {
//
//
// }

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
// pushUnorderedMapToLua(luaState,Inputs);
// // 将 Lua table 存入一个全局变量
// lua_setglobal(luaState, "NodeInputs");
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
