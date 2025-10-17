#pragma once

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <QVariantMap>
#include <QDebug>
#include <atomic>
#include <optional>
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "pybind11/eval.h"
#include "pybind11/subinterpreter.h"
#include "PythonEngineDefines.h"

namespace py = pybind11;

namespace Nodes {

/**
 * @brief Python解释器管理器单例类
 * 负责管理全局唯一的主解释器实例
 */
class PythonInterpreterManager {
public:
    static PythonInterpreterManager& getInstance();
    bool initializeMainInterpreter();
    bool isInitialized() const;
    QMutex& getInitMutex();
    
private:
    PythonInterpreterManager();
    ~PythonInterpreterManager();
    PythonInterpreterManager(const PythonInterpreterManager&) = delete;
    PythonInterpreterManager& operator=(const PythonInterpreterManager&) = delete;
    
    // 移除m_mainInterpreter，使用全局Python状态
    std::atomic<bool> m_initialized{false};
    QMutex m_initMutex;
    QMutex m_subInterpreterCreationMutex;
    
    friend class PythonWorkerThread;
};

/**
 * @brief Python工作线程类，为每个Python脚本创建独立的子解释器实例
 * 提供启动、停止和设置脚本的功能，支持真正的并发执行
 */
class PythonWorkerThread : public QThread
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit PythonWorkerThread(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~PythonWorkerThread();
    
    /**
     * @brief 设置Python脚本
     * @param script Python脚本内容
     */
    void setScript(const QString& script);
    
    /**
     * @brief 启动Python脚本执行
     */
    void startExecution();
    
    /**
     * @brief 执行Python脚本
     */
    void executePythonScript();
    
    /**
     * @brief 停止Python脚本执行
     */
    void stopExecution();
    
    /**
     * @brief 检查是否正在执行
     * @return 是否正在执行
     */
    bool isExecuting() const;
    
    /**
     * @brief 设置输入数据
     * @param portIndex 端口索引
     * @param data 输入数据
     */
    void setInputData(int portIndex, const QVariantMap& data);
    
    /**
     * @brief 获取输出数据
     * @param portIndex 端口索引
     * @return 输出数据
     */
    QVariantMap getOutputData(int portIndex) const;
    
    /**
     * @brief 设置端口数量
     * @param inputCount 输入端口数量
     * @param outputCount 输出端口数量
     */
    void setPortCounts(int inputCount, int outputCount);

protected:
    /**
     * @brief 线程运行函数
     */
    void run() override;

private:
    /**
     * @brief 清理Python子解释器
     */
    void cleanupPythonSubInterpreter();
    
    /**
     * @brief 在子解释器中注册回调函数
     */
    void registerCallbacksInSubInterpreter();

    /**
     * @brief 检查是否应该停止
     * @return 是否应该停止
     */
    bool shouldStop() const;

private slots:
    /**
     * @brief 获取输入值（Python接口）
     * @param portIndex 端口索引
     * @return Python对象
     */
    py::object getInputValuePy(int portIndex);
    
    /**
     * @brief 设置输出值（Python接口）
     * @param portIndex 端口索引
     * @param value Python对象值
     */
    void setOutputValuePy(int portIndex, const py::object& value);
    
    /**
     * @brief 发送消息
     * @param message 消息内容
     */
    void sendMessage(const QString& message);

signals:
    /**
     * @brief 执行完成信号
     * @param success 是否成功
     * @param errorMessage 错误消息
     */
    void executionFinished(bool success, const QString& errorMessage);
    
    /**
     * @brief 输出数据变化信号
     * @param portIndex 端口索引
     * @param data 输出数据
     */
    void outputDataChanged(int portIndex, const QVariantMap& data);
    
    /**
     * @brief 消息接收信号
     * @param message 消息内容
     */
    void messageReceived(const QString& message);

private:
    // Python解释器相关
    std::optional<py::subinterpreter> m_subInterpreter;  // 子解释器
    py::module_ m_mainModule;                            // Python主模块
    QString m_script;                                    // Python脚本内容
    
    // 线程控制
    std::atomic<bool> m_stopRequested{false};  // 停止请求标志
    std::atomic<bool> m_executing{false};      // 执行状态标志
    mutable QMutex m_mutex;                    // 互斥锁
    QWaitCondition m_condition;                // 等待条件
    
    // 数据存储
    QMap<int, QVariantMap> m_inputData;        // 输入数据
    QMap<int, QVariantMap> m_outputData;       // 输出数据
    mutable QMutex m_dataMutex;                // 数据访问互斥锁
    
    // 端口配置
    int m_inputPortCount{4};                   // 输入端口数量
    int m_outputPortCount{1};                  // 输出端口数量
    
    // 脚本执行控制
    bool m_scriptChanged{false};               // 脚本是否已更改
};

} // namespace Nodes