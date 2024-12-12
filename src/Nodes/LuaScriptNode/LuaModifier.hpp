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
    ~LuaQVariant() = default;
    // 使用 int 构造
    LuaQVariant(const int value) {
        variant_=QVariant::fromValue(value);
    }
    // 使用 double 构造，C++中使用
    LuaQVariant(const double value) : variant_(value) {
        variant_=QVariant::fromValue(value);
    }
    // 使用 bool 构造，C++中使用
    LuaQVariant(const bool value){
        variant_=QVariant::fromValue(value);
    }
    // 使用 QString 构造，C++中使用
    LuaQVariant(const std::string& value)  {
        variant_=QVariant::fromValue(QString::fromStdString(value));
    }
    // 从 QVariant 构造，C++中使用
    LuaQVariant(const QVariant& variant) : variant_(variant) {}
    // Lua传入的构造函数，自动处理Lua中的类型，lua中调用
    LuaQVariant(lua_State* L) {
        setValue(L);  // 调用setValue来处理初始化时的类型
    }
    // 使用 typeId 来根据类型推送值到 Lua，lua中调用
    void getValue(lua_State* L) const {
        int type = variant_.typeId();  // 获取 QVariant 的类型 ID
        switch (type) {
            case QMetaType::Int:
                lua_pushinteger(L, variant_.toInt());
            break;
            case QMetaType::Double:
                lua_pushnumber(L, variant_.toDouble());
            break;
            case QMetaType::Bool:
                lua_pushboolean(L, variant_.toBool());
            break;
            case QMetaType::QString:
                lua_pushstring(L, variant_.toString().toUtf8().constData());
            break;
            case QMetaType::Float:
                lua_pushnumber(L, static_cast<double>(variant_.toFloat()));
            break;
            default:
                lua_pushnil(L);  // 未知或不支持的类型，返回 nil
            break;
        }
    }
    // 转换为 int,c++和lua均可调用
    int toInt() const {
        if (variant_.canConvert<int>()) {
            return variant_.toInt();
        }
        return 0;
    }
    // 转换为 bool,c++和lua均可调用
    bool toBool() const {
        if (variant_.canConvert<bool>()) {
            return variant_.toBool();
        }
        return false;
    }
    // 转换为 double,c++和lua均可调用
    double toDouble() const {
        if (variant_.canConvert<double>()) {
            return variant_.toDouble();
        }
        return 0.0;
    }
    // 转换为 string (QString),c++和lua均可调用
    std::string toString() const {
        if (variant_.canConvert<QString>()) {
            return variant_.toString().toStdString();
        }
        return "";
    }
    // 允许在Lua中设置QVariant的值,用于在lua调用时实例化类
    void setValue(lua_State* L) {
        // 使用lua_gettop确保有足够的参数传入
        int args = lua_gettop(L);
        if (args < 1) {
            std::cout << "No arguments provided!" << std::endl;
            variant_ = QVariant();  // 空值
            return;
        }
        if (lua_isinteger(L, 2)) {
            lua_Integer intValue = luaL_checkinteger(L, 2);
            variant_ = QVariant(static_cast<int>(intValue));
            // std::cout << "Integer value set: " << intValue << std::endl;  // 调试输出
        } else if (lua_isnumber(L, 2)) {
            lua_Number numValue = luaL_checknumber(L, 2);
            variant_ = QVariant(static_cast<double>(numValue));
            // std::cout << "Double value set: " << numValue << std::endl;  // 调试输出
        } else if (lua_isboolean(L, 2)) {
            int boolValue = lua_toboolean(L, 2);
            variant_ = QVariant(static_cast<bool>(boolValue));
            // std::cout << "Boolean value set: " << boolValue << std::endl;  // 调试输出
        } else if (lua_isstring(L, 2)) {
            const char* strValue = luaL_checkstring(L, 2);
            variant_ = QVariant(QString::fromUtf8(strValue));
            // std::cout << "String value set: " << strValue << std::endl;  // 调试输出
        } else {
            std::cout << "Unrecognized type, empty QVariant set." << std::endl;
            variant_ = QVariant();  // 未知类型，设置为空值
        }
    }
    // 使用 typeId 来根据类型推送值到 Lua
    int getValue(lua_State* L) {
        int type = variant_.typeId();  // 获取 QVariant 的类型 ID
        switch (type) {
            case QMetaType::Int:
                lua_pushinteger(L, variant_.toInt());
            break;
            case QMetaType::Double:
                lua_pushnumber(L, variant_.toDouble());
            break;
            case QMetaType::Bool:
                lua_pushboolean(L, variant_.toBool());
            break;
            case QMetaType::QString:
                lua_pushstring(L, variant_.toString().toUtf8().constData());
            break;
            case QMetaType::Float:
                lua_pushnumber(L, static_cast<double>(variant_.toFloat()));
            break;
            default:
                lua_pushnil(L);  // 未知或不支持的类型，返回 nil
            break;
        }

        return 1;  // 返回一个值
    }
    //获取内部QVariant的typeId,lua中调用
     std::string getType() const {
        return variant_.typeName();
    }
    // 获取内部的 QVariant，C++中调用
    QVariant getVariant() const {
        return variant_;
    }
    void setVariant(const QVariant& val) {
        variant_=val;
    }

private:
    QVariant variant_;  // 内部的 QVariant 实例
};
// 注册QVariant转换类到lua
static void registerLuaQVariant(lua_State* L) {
    using namespace luabridge;
    getGlobalNamespace(L)
       .beginClass<LuaQVariant>("Variant")
        .addConstructor<void(*)(lua_State*)>()  // Lua传递的参数
        .addFunction("setValue", &LuaQVariant::setValue)  // 注册setValue函数
        .addFunction("value", &LuaQVariant::getValue)
        .addFunction("type", &LuaQVariant::getType)
            // 类型转换方法
       .addFunction("toInt", &LuaQVariant::toInt)
        .addFunction("toBool", &LuaQVariant::toBool)
       .addFunction("toDouble", &LuaQVariant::toDouble)
       .addFunction("toString", &LuaQVariant::toString)
       .endClass();
}

//注册全局打印函数，用于将lua中的print重定向到qDebug
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

class LuaQVariantMap {
public:
    LuaQVariantMap() = default;

    LuaQVariantMap(const QVariantMap& map) : variantMap(map) {}

    LuaQVariantMap(lua_State* L) {
        if (!lua_istable(L, -1)) {
            qDebug() << "Error: Expected a table.";
            return;
        }

        lua_pushnil(L); // 将 nil 压栈作为初始键
        while (lua_next(L, -2)) {
            QString key;
            if (lua_isnumber(L, -2)) { // 键是字符串
                key = QString::number(lua_tointeger(L, -2));

                if (lua_isnumber(L, -1)) {
                    variantMap.insert(key, lua_tonumber(L, -1));
                } else if (lua_isboolean(L, -1)) {
                    variantMap.insert(key, lua_toboolean(L, -1));
                } else if (lua_isstring(L, -1)) {
                    variantMap.insert(key, lua_tostring(L, -1));
                }
            }
            else if (lua_isstring(L, -2)) { // 键是字符串
                key = lua_tostring(L, -2);
                if (lua_isnumber(L, -1)) {
                    variantMap.insert(key, lua_tonumber(L, -1));
                } else if (lua_isboolean(L, -1)) {
                    variantMap.insert(key, lua_toboolean(L, -1));
                } else if (lua_isstring(L, -1)) {
                    variantMap.insert(key, lua_tostring(L, -1));
                }
            }
            lua_pop(L, 1); // 弹出值，保留键
        }
    }

    int toLuaTable(lua_State* L) {
        lua_newtable(L);
        for (auto it = variantMap.constBegin(); it != variantMap.constEnd(); ++it) {
            lua_pushstring(L, it.key().toUtf8().constData());
            const QVariant& value = it.value();

            switch (value.typeId()) {
                case QMetaType::Int:
                    lua_pushinteger(L, value.toInt());
                    break;
                case QMetaType::Double:
                    lua_pushnumber(L, value.toDouble());
                    break;
                case QMetaType::Bool:
                    lua_pushboolean(L, value.toBool());
                    break;
                case QMetaType::QString:
                    lua_pushstring(L, value.toString().toUtf8().constData());
                    break;
                case QMetaType::QVariantMap: {
                    LuaQVariantMap subMap(value.toMap());
                    subMap.toLuaTable(L);
                    break;
                }
                default:
                    lua_pushnil(L);
                    break;
            }
            lua_settable(L, -3);
        }
        return 1;
    }
    static LuaQVariantMap fromLuaTable(lua_State* L) {
        return LuaQVariantMap(L);
    }

    static LuaQVariantMap fromQVariantMap(const QVariantMap& map) {
        return LuaQVariantMap(map);
    }

public:
    QVariantMap variantMap;
};

static void registerLuaQVariantMap(lua_State* L) {
    using namespace luabridge;
    getGlobalNamespace(L)
            .beginClass<LuaQVariantMap>("VariantMap")
            .addConstructor<void(*)()>()
            .addStaticFunction("fromLuaTable", &LuaQVariantMap::fromLuaTable)
            .addStaticFunction("fromQVariantMap", &LuaQVariantMap::fromQVariantMap)
            .addFunction("toLuaTable", &LuaQVariantMap::toLuaTable)
            .endClass();
}