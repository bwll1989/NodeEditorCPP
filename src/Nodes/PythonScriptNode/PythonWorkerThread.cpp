#include "PythonWorkerThread.hpp"
#include <QDebug>
#include <QMutexLocker>

namespace Nodes {


// ==================== PythonInterpreterManager 实现 ====================

/**
 * @brief 获取解释器管理器单例实例
 * @return 管理器实例引用
 */
PythonInterpreterManager& PythonInterpreterManager::getInstance() {
    static PythonInterpreterManager instance;
    return instance;
}

// 全局Python解释器实例
static std::unique_ptr<py::scoped_interpreter> g_pythonInterpreter;
static std::once_flag g_pythonInitFlag;

/**
 * @brief PythonInterpreterManager构造函数
 */
PythonInterpreterManager::PythonInterpreterManager() {
    // 确保Python只初始化一次
    std::call_once(g_pythonInitFlag, []() {
        try {
            g_pythonInterpreter = std::make_unique<py::scoped_interpreter>();
            qDebug() << "全局Python解释器初始化成功";
            
            // 打印Python版本信息
            try {
                py::object sys = py::module_::import("sys");
                py::object version = sys.attr("version");
                py::object version_info = sys.attr("version_info");
                
                QString versionStr = QString::fromStdString(py::str(version));
                int major = py::int_(version_info.attr("major"));
                int minor = py::int_(version_info.attr("minor"));
                int micro = py::int_(version_info.attr("micro"));
                
                qDebug() << "Python版本:" << QString("%1.%2.%3").arg(major).arg(minor).arg(micro);
                qDebug() << "Python详细版本信息:" << versionStr.split('\n').first();
                
                // 检查是否为自由线程版本
                try {
                    py::object threading = py::module_::import("threading");
                    if (py::hasattr(threading, "_is_gil_enabled")) {
                        bool gil_enabled = py::bool_(threading.attr("_is_gil_enabled")());
                        qDebug() << "GIL状态:" << (gil_enabled ? "启用" : "禁用(自由线程模式)");
                    }
                } catch (const std::exception& e) {
                    qDebug() << "无法检测GIL状态:" << e.what();
                }
                
            } catch (const std::exception& e) {
                qDebug() << "获取Python版本信息失败:" << e.what();
            }
            
        } catch (const std::exception& e) {
            qDebug() << "全局Python解释器初始化失败:" << e.what();
        }
    });
}
/**
 * @brief PythonInterpreterManager析构函数
 */
PythonInterpreterManager::~PythonInterpreterManager() {
    // 由于使用单例模式和全局Python解释器，
    // 析构函数通常不会被调用，但需要提供实现以满足链接器要求
    qDebug() << "PythonInterpreterManager析构函数被调用";
}
/**
 * @brief 初始化主解释器
 * @return 是否初始化成功
 */
bool PythonInterpreterManager::initializeMainInterpreter() {
    QMutexLocker locker(&m_initMutex);
    
    if (m_initialized.load()) {
        return true;
    }
    
    if (g_pythonInterpreter && Py_IsInitialized()) {
        qDebug() << "Python主解释器已就绪";
        m_initialized.store(true);
        return true;
    }
    
    return false;
}

/**
 * @brief 检查主解释器是否已初始化
 * @return 是否已初始化
 */
bool PythonInterpreterManager::isInitialized() const {
    return m_initialized.load();
}

/**
 * @brief 获取初始化互斥锁
 * @return 互斥锁引用
 */
QMutex& PythonInterpreterManager::getInitMutex() {
    return m_subInterpreterCreationMutex;
}

// ==================== PythonWorkerThread 实现 ====================

/**
 * @brief 构造函数 - 创建PythonWorkerThread实例并初始化子解释器
 * @param parent 父对象指针
 */
PythonWorkerThread::PythonWorkerThread(QObject *parent)
    : QThread(parent)
{
    // 确保主解释器已初始化
    auto& manager = PythonInterpreterManager::getInstance();
    if (!manager.initializeMainInterpreter()) {
        qWarning() << "主解释器初始化失败";
        return;
    }
    
    // 在构造函数中直接初始化子解释器
    try {
        // 确保主解释器已经初始化
        if (!manager.isInitialized()) {
            throw std::runtime_error("主解释器未初始化");
        }

        qDebug() << "开始在构造函数中初始化Python子解释器";

        // 直接创建子解释器对象
        qDebug() << "正在创建子解释器...";

        PyInterpreterConfig config;
        std::memset(&config, 0, sizeof(config));
        config.allow_threads = 1;
        config.check_multi_interp_extensions = 1;
        config.gil = PyInterpreterConfig_OWN_GIL;
        m_subInterpreter = py::subinterpreter::create(config);

        // 激活子解释器并设置环境
        {
            py::subinterpreter_scoped_activate guard(*m_subInterpreter);

            // 打印子解释器信息（包装在try-catch中）
            try {
                auto sys_module = py::module_::import("sys");
                auto version_info = sys_module.attr("version_info");
                auto version = sys_module.attr("version");
                auto platform = sys_module.attr("platform");
                
                int major = py::cast<int>(version_info.attr("major"));
                int minor = py::cast<int>(version_info.attr("minor"));
                int micro = py::cast<int>(version_info.attr("micro"));
                
                qDebug() << "=== 子解释器信息 ===";
                qDebug() << "子解释器ID:" << m_subInterpreter->id();
                qDebug() << "Python版本:" << QString("%1.%2.%3").arg(major).arg(minor).arg(micro);
                qDebug() << "Python详细版本:" << QString::fromStdString(py::cast<std::string>(version));
                qDebug() << "平台:" << QString::fromStdString(py::cast<std::string>(platform));
                
                // 检查是否为自由线程模式
                bool is_free_threaded = false;
                try {
                    auto threading_module = py::module_::import("threading");
                    if (py::hasattr(threading_module, "_is_free_threaded")) {
                        is_free_threaded = py::cast<bool>(threading_module.attr("_is_free_threaded"));
                    }
                } catch (...) {
                    // 忽略错误，可能是旧版本Python
                }
                
                qDebug() << "自由线程模式:" << (is_free_threaded ? "是" : "否");
                qDebug() << "独立GIL:" << "是"; // 因为配置了PyInterpreterConfig_OWN_GIL
                qDebug() << "========================";
            } catch (const std::exception& e) {
                qDebug() << "获取子解释器信息时出错:" << e.what();
            }

            // 在子解释器中导入主模块
            m_mainModule = py::module_::import("__main__");

            // 注册回调函数
            registerCallbacksInSubInterpreter();
            
            // 设置Python环境（包装在try-catch中）
            try {
                py::exec(R"(
import sys
import io
# 自定义输出流类，将输出重定向到qDebug
class QDebugStream(io.TextIOBase):
    def __init__(self):
        super().__init__()
        self.buffer = ""

    def write(self, text):
        if text:
            self.buffer += text
            if '\n' in text:
                lines = self.buffer.split('\n')
                for line in lines[:-1]:
                    if line.strip():
                        qdebug_print(line)
                self.buffer = lines[-1]
        return len(text)

    def flush(self):
        if self.buffer.strip():
            qdebug_print("[Python] " + self.buffer)
            self.buffer = ""

# 重定向标准输出和错误输出
sys.stdout = QDebugStream()
sys.stderr = QDebugStream()
)");
            } catch (const std::exception& e) {
                qWarning() << "设置Python输出重定向时出错:" << e.what();
            }

            // 注入 worker 运行时 API（供脚本直接调用）
            try {
                py::exec(R"(
class Worker:
    def get_input_value(self, idx):
        return get_input_value_callback(idx)
    def set_output_value(self, idx, value):
        return set_output_value_callback(idx, value)
    def get_input_count(self):
        return get_input_count_callback()
    def get_output_count(self):
        return get_output_count_callback()
    def should_stop(self):
        return should_stop_callback()
    def send_message(self, msg):
        return send_message_callback(msg)

# 提供一个全局实例
worker = Worker()
)", m_mainModule.attr("__dict__"));
            } catch (const std::exception& e) {
                qWarning() << "安装 worker 运行时 API 失败:" << e.what();
            }
    }

    qDebug() << "Python子解释器在构造函数中初始化完成";

    } catch (const py::error_already_set& e) {
        qCritical() << "Python子解释器初始化失败(Python错误):" << e.what();
      
        
    } catch (const std::bad_alloc& e) {
        qCritical() << "Python子解释器初始化失败(内存不足):" << e.what();
       
        
    } catch (const std::runtime_error& e) {
        qCritical() << "Python子解释器初始化失败(运行时错误):" << e.what();
      
        
    } catch (const std::exception& e) {
        qCritical() << "Python子解释器初始化失败(标准异常):" << e.what();
     
        
    } catch (...) {
        qCritical() << "Python子解释器初始化失败(未知异常)";
   
    }
}

/**
 * @brief 析构函数
 */
PythonWorkerThread::~PythonWorkerThread()
{
    // 确保线程停止
    stopExecution();
    if (isRunning()) {
        wait(5000);
        if (isRunning()) {
            terminate();
            wait(1000);
        }
    }
    
    // 清理Python子解释器
    cleanupPythonSubInterpreter();
}
/**
 * @brief 线程主函数
 */
void PythonWorkerThread::run()
{
    qDebug() << "Python工作线程开始运行";
    
    try {
        // 检查子解释器是否已在构造函数中成功初始化
        if (!m_subInterpreter.has_value()) {
            qWarning() << "子解释器未初始化，无法运行线程";
            return;
        }
        
        qDebug() << "线程开始运行，使用已初始化的子解释器ID:" << m_subInterpreter->id();
        
        // 线程主循环
        while (!shouldStop()) {
            try {
                {
                    QMutexLocker locker(&m_mutex);
                    // 等待执行请求或脚本变更
                    if (!m_executing.load() && !m_scriptChanged) {
                        m_condition.wait(&m_mutex, 100);
                        continue;
                    } 
                }
                
                // 处理脚本执行请求
                if (m_executing.load()) {
                    executePythonScript();
                }
                
                // 处理脚本变更
                if (m_scriptChanged) {
                    m_scriptChanged = false;
                    // qDebug() << "脚本已更新";
                }
                
            } catch (const std::exception& e) {
                qWarning() << "线程循环中发生异常:" << e.what();
                // 重置状态并继续
                m_executing = false;
                m_scriptChanged = false;
                
            } catch (...) {
                qCritical() << "线程循环中发生未知异常";
                // 重置状态并继续
                m_executing = false;
                m_scriptChanged = false;
            }
        }
        
    } catch (const std::exception& e) {
        qCritical() << "线程运行时发生严重异常:" << e.what();
        
    } catch (...) {
        qCritical() << "线程运行时发生未知的严重异常";
    }
    
    qDebug() << "线程结束运行";
    // 注意：不在这里清理子解释器，是在析构函数中清理
}

/**
 * @brief 清理Python子解释器
 */
void PythonWorkerThread::cleanupPythonSubInterpreter()
{
    if (m_subInterpreter.has_value()) {
        auto& manager = PythonInterpreterManager::getInstance();

        try {
            qDebug() << "开始清理Python子解释器，ID:" << m_subInterpreter->id();

            // 重置子解释器
            m_subInterpreter.reset();


        } catch (const std::exception& e) {
            qDebug() << "清理Python子解释器时发生错误:" << e.what();
            // 即使出错也要减少计数
            m_subInterpreter.reset();
        }
    }
}


void PythonWorkerThread::registerCallbacksInSubInterpreter()
{
    // 注册 qDebug 输出回调
    m_mainModule.attr("qdebug_print") = py::cpp_function([](const std::string& message) {
        qDebug() << QString::fromStdString(message);
    });

    // 注册底层 callback（保留）
    m_mainModule.attr("get_input_value_callback") = py::cpp_function([this](int index) -> py::object {
        return this->getInputValuePy(index);
    });
    m_mainModule.attr("set_output_value_callback") = py::cpp_function([this](int index, py::object value) {
        this->setOutputValuePy(index, value);
    });
    m_mainModule.attr("should_stop_callback") = py::cpp_function([this]() -> bool {
        return this->shouldStop();
    });
    m_mainModule.attr("send_message_callback") = py::cpp_function([this](const std::string& message) {
        this->sendMessage(QString::fromStdString(message));
    });
    m_mainModule.attr("get_input_count_callback") = py::cpp_function([this]() -> int {
        return m_inputPortCount;
    });
    m_mainModule.attr("get_output_count_callback") = py::cpp_function([this]() -> int {
        return m_outputPortCount;
    });

    // 提供更易用的别名函数
    m_mainModule.attr("get_input_value") = m_mainModule.attr("get_input_value_callback");
    m_mainModule.attr("set_output_value") = m_mainModule.attr("set_output_value_callback");
    m_mainModule.attr("get_input_count") = m_mainModule.attr("get_input_count_callback");
    m_mainModule.attr("get_output_count") = m_mainModule.attr("get_output_count_callback");
    m_mainModule.attr("should_stop") = m_mainModule.attr("should_stop_callback");
    m_mainModule.attr("send_message") = m_mainModule.attr("send_message_callback");

    // 提供当前输入索引查询
    m_mainModule.attr("get_current_input_index") = py::cpp_function([this]() -> int {
        return m_lastInputIndex;
    });

    // 在子解释器中注入一个简单的 Worker 类与实例
    py::exec(R"(
class Worker:
    def get_input_value(self, index): return get_input_value(index)
    def set_output_value(self, index, value): return set_output_value(index, value)
    def get_input_count(self): return get_input_count()
    def get_output_count(self): return get_output_count()
    def get_current_input_index(self): return get_current_input_index()
    def should_stop(self): return should_stop()
    def send_message(self, msg): return send_message(msg)

# 提供全局实例
worker = Worker()
)", m_mainModule.attr("__dict__"));
}

py::object PythonWorkerThread::getInputValuePy(int portIndex)
{
    // 在子解释器中，不需要额外的GIL管理，因为已经在正确的线程状态中
    QMutexLocker locker(&m_dataMutex);
    if (m_inputData.contains(portIndex)) {
        return PythonEngineDefines::variantMapToPyDict(m_inputData[portIndex]);
    }
    return py::dict();
}

void PythonWorkerThread::setOutputValuePy(int portIndex, const py::object& value)
{
    if (portIndex >= 0 && portIndex < m_outputPortCount) {
        QVariantMap data = PythonEngineDefines::pyObjectToVariantMap(value);
        {
            QMutexLocker locker(&m_dataMutex);
            m_outputData[portIndex] = data;
        }
        emit outputDataChanged(portIndex, data);
    }
}

void PythonWorkerThread::setScript(const QString& script)
{
    QMutexLocker locker(&m_mutex);
    m_script = script;
    m_scriptChanged = true;
    m_uiInitialized = false; // 标记需要重新初始化 UI（init_interface）
}

/**
 * @brief 标记要触发的输入事件索引，并唤醒线程执行
 *        同步记录最近一次触发索引，供脚本查询。
 * @param index 输入端口索引
 */
void PythonWorkerThread::setInputEventIndex(int index)
{
    QMutexLocker locker(&m_mutex);
    m_pendingInputIndex = index;
    m_lastInputIndex = index;      // 记录最近一次触发的索引
    m_executing = true;
    m_condition.wakeOne();
}

/**
 * @brief 启动Python脚本执行
 *        若线程未运行则启动线程；已运行则唤醒执行。
 */
void PythonWorkerThread::startExecution()
{
    if (!isRunning()) {
        m_stopRequested = false;
        m_executing = true;  // 设置执行状态
        start();
    } else {
        // 如果已经在运行，设置执行状态并唤醒线程执行新脚本
        QMutexLocker locker(&m_mutex);
        m_executing = true;
        m_condition.wakeOne();
    }
}

bool PythonWorkerThread::isExecuting() const
{
    return m_executing;
}

void PythonWorkerThread::setInputData(int portIndex, const QVariantMap& data)
{
    QMutexLocker locker(&m_dataMutex);
    m_inputData[portIndex] = data;
}

QVariantMap PythonWorkerThread::getOutputData(int portIndex) const
{
    QMutexLocker locker(&m_dataMutex);
    return m_outputData.value(portIndex);
}

void PythonWorkerThread::setPortCounts(int inputCount, int outputCount)
{
    QMutexLocker locker(&m_mutex);
    m_inputPortCount = inputCount;
    m_outputPortCount = outputCount;
}

/**
 * @brief 在子解释器中执行Python脚本
 * 使用RAII方式管理子解释器的激活状态
 */
void PythonWorkerThread::executePythonScript()
{
    if (!m_subInterpreter) {
        qDebug() << "Python子解释器未初始化";
        m_executing = false;  // 重置执行状态
        return;
    }
    
    try {  
        // 激活子解释器
        py::subinterpreter_scoped_activate guard(*m_subInterpreter);
        
        py::exec(m_script.toStdString(), m_mainModule.attr("__dict__"));
        
    } catch (const py::error_already_set& e) {
        qDebug() << "Python脚本执行错误:" << e.what();
        emit executionFinished(false, QString::fromStdString(e.what()));

    } catch (const std::exception& e) {
        qDebug() << "执行Python脚本时发生异常:" << e.what();
        emit executionFinished(false, QString::fromStdString(e.what()));
    }
    
    // 执行完成后重置执行状态
    m_executing = false;
    emit executionFinished(true, "");
}


bool PythonWorkerThread::shouldStop() const
{
    return m_stopRequested;
}

void PythonWorkerThread::stopExecution()
{
    m_stopRequested = true;
    m_executing = false;
    QMutexLocker locker(&m_mutex);
    m_condition.wakeOne();
}

void PythonWorkerThread::sendMessage(const QString& message)
{
    emit messageReceived(message);
}

} // namespace Nodes
