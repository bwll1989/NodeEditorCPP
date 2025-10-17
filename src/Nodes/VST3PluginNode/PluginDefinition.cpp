#include "PluginDefinition.hpp"

#include <QStandardPaths>

#include "VST3PluginDataModel.hpp"
#include "Vst3PluginManager.hpp"
Plugin *Plugin::_this_plugin = nullptr;

Plugin::Plugin():m_pluginManager(nullptr)
{
    _this_plugin = this;
    initializePluginManager();
}


Plugin::~Plugin()
{
    if (m_pluginManager) {
        delete m_pluginManager;
        m_pluginManager = nullptr;
    }
}
/**
 * @brief 初始化插件管理器，扫描并加载Vst3插件
 */
void Plugin::initializePluginManager()
{
    // 获取插件目录路径
    QString pluginDir = getPluginDirectory();
    // 创建插件管理器
    m_pluginManager = new Nodes::Vst3PluginManager(pluginDir, this);

    // 扫描Vst3插件
    m_pluginManager->scanPlugins();
    m_Vst3Plugins = m_pluginManager->getAvailablePlugins();


}
/**
 * @brief 获取插件目录路径
 * @return 插件目录的绝对路径
 */
QString Plugin::getPluginDirectory() const
{
    // 优先使用应用程序目录下的plugins/Vst3Scripts
    QString appDir = QCoreApplication::applicationDirPath();
    QString pluginDir = QDir(appDir).absoluteFilePath("plugins/VST3");

    // 如果目录不存在，尝试创建
    QDir dir(pluginDir);
    if (!dir.exists()) {
        if (dir.mkpath(pluginDir)) {
            qDebug() << "创建插件目录:" << pluginDir;
        } else {
            qWarning() << "无法创建插件目录:" << pluginDir;
            // 回退到用户文档目录
            QString userDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            pluginDir = QDir(userDir).absoluteFilePath("plugins/VST3");
            QDir(pluginDir).mkpath(pluginDir);
        }
    }

    return pluginDir;
}
void Plugin::registerDataModels(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg)
{
    // assert(reg);
    //
    // reg->registerModel<Nodes::VST3PluginDataModel>(name(),tag());
    assert(reg);

    if (m_Vst3Plugins.isEmpty()) {
        // 如果没有发现Vst3插件，注册默认的CustomScriptDataModel
        // reg->registerModel<Nodes::CustomScriptDataModel>("Custom Script", "Scripts");
        // qDebug() << "注册默认Vst3脚本节点";
    } else {
        // 为每个Vst3插件注册一个独立的节点类型
        for (const auto& pluginInfo : m_Vst3Plugins) {
            registerVst3Plugin(reg, pluginInfo);
        }

        qDebug() << "成功注册" << m_Vst3Plugins.size() << "个Vst3插件节点";
    }
}

/**
 * @brief 为单个Vst3插件注册工厂函数
 * @param reg 节点注册表
 * @param pluginInfo Vst3插件信息
 */
void Plugin::registerVst3Plugin(std::shared_ptr<QtNodes::NodeDelegateModelRegistry> &reg,
                             const QString& path)
{
    QFileInfo fileinfo(path);
    // 创建工厂函数，捕获插件信息副本
    auto factory = [path]() -> std::unique_ptr<QtNodes::NodeDelegateModel> {
        // 使用带插件信息的构造函数创建节点实例
        auto model = std::make_unique<Nodes::VST3PluginDataModel>(path);
        return model;
    };
    // 使用工厂函数注册节点类型
    reg->registerFactory(fileinfo.baseName(), factory, "Audio");

}