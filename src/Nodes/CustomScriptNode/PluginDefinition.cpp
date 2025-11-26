#include "PluginDefinition.hpp"
#include "CustomScriptDataModel.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <memory>

Plugin *Plugin::_this_plugin = nullptr;

/**
 * @brief 构造函数，初始化插件管理器并扫描JS插件
 */
Plugin::Plugin() : m_pluginManager(nullptr)
{
    _this_plugin = this;
    initializePluginManager();
}

/**
 * @brief 析构函数，清理资源
 */
Plugin::~Plugin()
{
    if (m_pluginManager) {
        delete m_pluginManager;
        m_pluginManager = nullptr;
    }
}

/**
 * @brief 初始化插件管理器，扫描并加载JS插件
 */
void Plugin::initializePluginManager()
{
    // 获取插件目录路径
    QString pluginDir = getPluginDirectory();

    // qDebug() << "初始化JS插件管理器，插件目录:" << pluginDir;
    
    // 创建插件管理器
    m_pluginManager = new Nodes::JSPluginManager(pluginDir, this);
    
    // 扫描JS插件
    m_pluginManager->scanPlugins();
    m_jsPlugins = m_pluginManager->getAvailablePlugins();
    
    // qDebug() << "发现" << m_jsPlugins.size() << "个JS插件";
    //
    // // 输出插件信息
    // for (const auto& plugin : m_jsPlugins) {
    //     qDebug() << "  - 插件:" << plugin.name
    //              << "分类:" << plugin.category
    //              << "输入:" << plugin.inputs
    //              << "输出:" << plugin.outputs;
    // }
}

/**
 * @brief 获取插件目录路径
 * @return 插件目录的绝对路径
 */
QString Plugin::getPluginDirectory() const
{
    // 优先使用应用程序目录下的plugins/JSScripts
    QString appDir = QCoreApplication::applicationDirPath();
    QString pluginDir = QDir(appDir).absoluteFilePath("plugins/JSScripts");
    
    // 如果目录不存在，尝试创建
    QDir dir(pluginDir);
    if (!dir.exists()) {
        if (dir.mkpath(pluginDir)) {
            qDebug() << "创建插件目录:" << pluginDir;
        } else {
            qWarning() << "无法创建插件目录:" << pluginDir;
            // 回退到用户文档目录
            QString userDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            pluginDir = QDir(userDir).absoluteFilePath("plugins/JSPlugins");
            QDir(pluginDir).mkpath(pluginDir);
        }
    }
    
    return pluginDir;
}

/**
 * @brief 获取插件系统名称
 * @return 插件系统名称
 */
QString Plugin::name() const
{
    return "CustomScript Plugin System";
}

/**
 * @brief 获取插件系统版本
 * @return 版本号
 */
QString Plugin::version() const
{
    return PLUGIN_VERSION;
}

/**
 * @brief 获取插件系统描述
 * @return 包含已加载插件数量的描述
 */
QString Plugin::describe() const
{
    return QString("动态JS脚本插件 - 已加载%1个插件").arg(m_jsPlugins.size());
}

/**
 * @brief 获取插件系统标签
 * @return 插件标签
 */
QString Plugin::tag() const
{
    return "Scripts";
}

/**
 * @brief 注册数据模型，为每个JS插件动态创建节点类型
 * @param reg 节点注册表
 */
void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg)
{
    assert(reg);
    
    if (m_jsPlugins.isEmpty()) {
        // 如果没有发现JS插件，注册默认的CustomScriptDataModel
        // reg->registerModel<Nodes::CustomScriptDataModel>("Custom Script", "Scripts");
        // qDebug() << "注册默认JS脚本节点";
    } else {
        // 为每个JS插件注册一个独立的节点类型
        for (const auto& pluginInfo : m_jsPlugins) {
            registerJSPlugin(reg, pluginInfo);
        }
        
        // qDebug() << "成功注册" << m_jsPlugins.size() << "个JS插件节点";
    }
}

/**
 * @brief 为单个JS插件注册工厂函数
 * @param reg 节点注册表
 * @param pluginInfo JS插件信息
 */
void Plugin::registerJSPlugin(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg, 
                             const Nodes::JSPluginInfo& pluginInfo)
{
    // 从插件信息中获取节点名称和分类
    QString nodeName = pluginInfo.name.isEmpty() ? "Unnamed Script" : pluginInfo.name;
    QString nodeCategory = pluginInfo.category.isEmpty() ? "Unnamed Scripts" : pluginInfo.category;
    
    // 创建工厂函数，捕获插件信息副本
    auto factory = [pluginInfo]() -> std::unique_ptr<QtNodes::NodeDelegateModel> {
        // 使用带插件信息的构造函数创建节点实例
        auto model = std::make_unique<Nodes::CustomScriptDataModel>(pluginInfo);
        return model;
    };
    
    // 使用工厂函数注册节点类型
    reg->registerFactory(nodeName, factory, nodeCategory);
    
    // qDebug() << "注册JS插件节点:" << nodeName
    //          << "分类:" << nodeCategory
    //          << "端口配置:" << pluginInfo.inputs << "->" << pluginInfo.outputs;
}



