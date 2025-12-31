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

#include "ConstantDefines.h"
#include "Elements/SelectorComboBox/SelectorComboBox.hpp"
#include "MediaLibrary/MediaLibrary.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using namespace NodeDataTypes;
namespace Nodes
{
    class JsonLoaderModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数：初始化选择器与端口，绑定文件选择到JSON加载
         */
        JsonLoaderModel() {
            InPortCount =0;
            OutPortCount=1;

            CaptionVisible=true;
            Caption="JSON File";
            WidgetEmbeddable= true;
            Resizable=false;
            PortEditable= false;
            m_outJsonData=std::make_shared<VariableData>();
            AbstractDelegateModel::registerOSCControl("/file",_fileSelectComboBox);
            connect(_fileSelectComboBox, &SelectorComboBox::textChanged, this, &JsonLoaderModel::loadJson);
        }
        ~JsonLoaderModel() override = default;

    public:
        /**
         * @brief 端口类型：统一使用VariableData（用于承载JSON）
         */
        NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            return VariableData().type();
        }

        /**
         * @brief 输出端口返回当前解析后的JSON数据
         */
        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override{
            return m_outJsonData;
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
            if (m_outJsonData && !_fileSelectComboBox->text().isEmpty()) {
                modelJson["path"] = _fileSelectComboBox->text();
            }
            return modelJson;
        }

        /**
         * @brief 按保存的路径加载文件并解析为JSON
         */
        void load(QJsonObject const& jsonObj) override{
            const QJsonValue path = jsonObj["path"];
            if (!path.isUndefined()) {
               _fileSelectComboBox->setText(path.toString());
                loadJson(path.toString());
            }
        }

    private:
        /**
         * @brief 打开文本文件并解析为JSON，输出到端口0
         * @param fileName 媒体库中的文件名或相对路径
         * - 解析为对象：直接用对象构造VariableData
         * - 解析为数组：存至默认键（QVariantList），便于下游通过json()得到数组
         * - 失败：清空输出并发送错误反馈
         */
        void loadJson(QString fileName){
            if (fileName.isEmpty()) {
                m_outJsonData.reset();
                emit dataUpdated(0);
                return;
            }

            const QString fullPath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + fileName;

            QFile f(fullPath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "JsonLoaderModel: cannot open file" << fullPath;
                m_outJsonData.reset();
                updateNodeState(QtNodes::NodeValidationState::State::Error,"cannot open file");
                emit dataUpdated(0);
                return;
            }

            const QByteArray raw = f.readAll();
            f.close();

            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "JsonLoaderModel: parse error" << parseError.errorString() << "at" << parseError.offset;
                updateNodeState(QtNodes::NodeValidationState::State::Error,"parse error: " + parseError.errorString());
                m_outJsonData.reset();
                emit dataUpdated(0);
                return;
            }
            updateNodeState(QtNodes::NodeValidationState::State::Valid,"");
            m_path = fullPath;

            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                m_outJsonData = std::make_shared<VariableData>(&obj);
            } else if (doc.isArray()) {
                // 将数组保存到默认键，作为QVariantList，确保json()能输出为数组
                QVariantList list;
                for (const auto& v : doc.array()) list.push_back(v.toVariant());
                m_outJsonData = std::make_shared<VariableData>();
                m_outJsonData->insert("default", list);
            } else {
                // 非对象/数组根（极少见），输出空对象
                m_outJsonData = std::make_shared<VariableData>();
            }
            emit dataUpdated(0);
        }

    private:

        SelectorComboBox* _fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Document);
        QString m_path;
        std::shared_ptr<VariableData> m_outJsonData;

    };
}