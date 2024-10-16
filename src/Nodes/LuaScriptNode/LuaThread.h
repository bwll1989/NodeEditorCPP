//
// Created by bwll1 on 2024/10/11.
//

#ifndef NODEEDITORCPP_LUATHREAD_H
#define NODEEDITORCPP_LUATHREAD_H
#include <QThread>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class LuaThread : public QThread
{
Q_OBJECT

public:
    LuaThread(QObject *parent = nullptr);
    ~LuaThread();

    void executeLuaScript(const QString &script);
    void stop();  // 用于停止线程
    bool isRunningScript() const;

protected:
    void run() override;

private:
    QString m_script;
    QMutex m_mutex;
    QWaitCondition m_condition;
    std::atomic<bool> m_running;
    std::atomic<bool> m_stopRequested;
    std::atomic<bool> m_interruptRequested;  // 用于中断当前的 Lua 脚本

    lua_State *L;  // Lua 状态机

    void runLuaScript();
    static int checkForInterruption(lua_State *L);  // 检查中断请求并抛出错误
};


#endif //NODEEDITORCPP_LUATHREAD_H
