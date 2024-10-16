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
//lua_pushcfunction(L, luaPrint);
//lua_setglobal(L, "print");



LuaThread::LuaThread(QObject *parent)
        : QThread(parent),
          m_running(false),
          m_stopRequested(false),
          m_interruptRequested(false),
          L(nullptr)
{
}

LuaThread::~LuaThread() {
    stop();
    wait();
}

void LuaThread::executeLuaScript(const QString &script) {
    QMutexLocker locker(&m_mutex);

    if (m_running) {
        m_interruptRequested = true;  // 标记请求中断当前脚本
    }

    m_script = script;

    if (!m_running) {
        m_running = true;
        m_stopRequested = false;
        m_interruptRequested = false;
        start();  // 启动线程
    } else {
        m_condition.wakeOne();  // 如果线程已经在运行，唤醒线程执行新的脚本
    }
}

void LuaThread::stop() {
    QMutexLocker locker(&m_mutex);
    if (m_running) {
        m_stopRequested = true;
        m_interruptRequested = true;  // 请求中断当前脚本
        m_condition.wakeOne();        // 唤醒线程进行停止处理
    }
}

bool LuaThread::isRunningScript() const {
    return m_running;
}

void LuaThread::run() {
    while (!m_stopRequested) {
        m_mutex.lock();
        if (m_script.isEmpty()) {
            m_condition.wait(&m_mutex);  // 如果没有新的脚本，等待
        }
        QString scriptToRun = m_script;
        m_script.clear();
        m_mutex.unlock();

        if (!scriptToRun.isEmpty()) {
            runLuaScript();  // 执行 Lua 脚本
        }

        QMutexLocker locker(&m_mutex);
        if (m_script.isEmpty()) {
            m_running = false;  // 如果没有新的脚本，标记停止运行
            break;
        }
    }
}

void LuaThread::runLuaScript() {
    L = luaL_newstate();
    luaL_openlibs(L);
lua_pushcfunction(L, luaPrint);
lua_setglobal(L, "print");
    // 注册中断检查函数
    lua_register(L, "checkForInterruption", checkForInterruption);

    int loadStatus = luaL_loadstring(L, m_script.toStdString().c_str());
    if (loadStatus == LUA_OK) {
        int execStatus = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (execStatus != LUA_OK) {
            qDebug() << "Error executing Lua script:" << lua_tostring(L, -1);
        }
    } else {
        qDebug() << "Error loading Lua script:" << lua_tostring(L, -1);
    }

    lua_close(L);
    L = nullptr;
}

int LuaThread::checkForInterruption(lua_State *L) {
    LuaThread *thread = static_cast<LuaThread *>(lua_getextraspace(L));

    if (thread->m_interruptRequested) {
        return luaL_error(L, "Script interrupted by a new request.");
    }
    return 0;  // 返回 0 表示没有中断
}


