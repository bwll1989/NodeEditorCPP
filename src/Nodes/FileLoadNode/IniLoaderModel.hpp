#pragma once

#include <iostream>
#include <QComboBox>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "DataTypes/NodeDataList.hpp"
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>
#include <QSettings>
#include <QtCore/QFile>
#include <QtCore/QByteArray>

#include "ConstantDefines.h"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using namespace NodeDataTypes;
namespace Nodes
{
    class IniLoaderModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数：初始化选择器与端口，绑定文件选择到INI加载
         */
        IniLoaderModel() {

            InPortCount =0;
            OutPortCount=1;

            CaptionVisible=true;
            Caption="INI File";
            WidgetEmbeddable= true;
            Resizable=false;
            PortEditable= false;
            m_outIniData=std::make_shared<VariableData>();
            AbstractDelegateModel::registerOSCControl("/file",_fileSelectComboBox);
            connect(_fileSelectComboBox, &SelectorComboBox::textChanged, this, &IniLoaderModel::loadIni);
        }
        ~IniLoaderModel() override = default;

    public:
        /**
         * @brief 端口类型：统一使用VariableData（承载INI解析结果）
         */
        NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            return VariableData().type();
        }

        /**
         * @brief 输出端口返回当前解析后的INI数据
         */
        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override{
            return m_outIniData;
        }

        /**
         * @brief 该节点无输入数据依赖
         */
        void setInData(std::shared_ptr<QtNodes::NodeData>, QtNodes::PortIndex const portIndex) override {}

        /**
         * @brief 嵌入式控件：文件选择（文档类）
         */
        QWidget *embeddedWidget() override {
            return _fileSelectComboBox;
        }

        /**
         * @brief 保存当前选中的文件路径以便复现状态
         */
        QJsonObject save() const override{
            QJsonObject modelJson = NodeDelegateModel::save();
            if (m_outIniData && !_fileSelectComboBox->text().isEmpty()) {
                modelJson["path"] = _fileSelectComboBox->text();
            }
            return modelJson;
        }

        /**
         * @brief 按保存的路径加载文件并解析为INI
         */
        void load(QJsonObject const& jsonObj) override{
            const QJsonValue path = jsonObj["path"];
            if (!path.isUndefined()) {
               _fileSelectComboBox->setText(path.toString());
                loadIni(path.toString());
            }
        }


    private:
        /**
         * @brief 打开并解析INI文件，输出到端口0
         * @param fileName 媒体库中的文件名或相对路径
         *
         * 行为：
         * - 根键（无分组）输出到 "global" 分组
         * - 各分组键输出到对应分组
         * - 同时生成“扁平化”默认映射，键格式为 "group.key"；根键为 "key"
         * - 输出结构：
         *   - global: QVariantMap（根键）
         *   - 每个分组: QVariantMap（分组内键值）
         *   - default: QVariantMap（扁平化映射）
         * - 失败：清空输出并发送错误反馈
         */
        void loadIni(QString fileName){
            if (fileName.isEmpty()) {
                m_outIniData.reset();
                emit dataUpdated(0);
                return;
            }

            const QString fullPath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + fileName;

            // 使用QSettings解析INI
            QSettings settings(fullPath, QSettings::IniFormat);
            QFile check(fullPath);
            if (!check.exists()) {
                qWarning() << "IniLoaderModel: file not exists" << fullPath;
                m_outIniData.reset();
                updateNodeState(QtNodes::NodeValidationState::State::Error,"file not exists");  
                emit dataUpdated(0);    
                return;
            }

            // 根键（无分组）
            QVariantMap globalMap;
            const QStringList rootKeys = settings.childKeys();
            for (const auto& k : rootKeys) {
                globalMap.insert(k, settings.value(k));
            }

            // 分组键
            QVariantMap nestedMap;
            if (!globalMap.isEmpty())
                nestedMap.insert("global", globalMap);

            const QStringList groups = settings.childGroups();
            for (const auto& g : groups) {
                settings.beginGroup(g);
                QVariantMap groupMap;
                const QStringList keys = settings.childKeys();
                for (const auto& k : keys) {
                    groupMap.insert(k, settings.value(k));
                }
                settings.endGroup();
                nestedMap.insert(g, groupMap);
            }

            // 扁平化默认映射
            QVariantMap flatMap;
            for (auto it = globalMap.begin(); it != globalMap.end(); ++it) {
                flatMap.insert(it.key(), it.value());
            }
            for (const auto& g : groups) {
                const QVariantMap gm = nestedMap.value(g).toMap();
                for (auto it = gm.begin(); it != gm.end(); ++it) {
                    flatMap.insert(QString("%1.%2").arg(g, it.key()), it.value());
                }
            }

            // 组装输出数据
            m_path = fullPath;
            auto out = std::make_shared<VariableData>();
            // 每个分组作为一个键插入
            for (auto it = nestedMap.begin(); it != nestedMap.end(); ++it) {
                out->insert(it.key(), it.value());
            }
            // 默认值为扁平化映射
            out->insert("default", flatMap);
            m_outIniData = out;

            updateNodeState(QtNodes::NodeValidationState::State::Valid);
            emit dataUpdated(0);    
        }

    private:

        SelectorComboBox* _fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Document);
        QString m_path;
        std::shared_ptr<VariableData> m_outIniData;

    };
}