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
#include "OSCMessage.h"
#include "OSCSender/OSCSender.h"
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using namespace NodeDataTypes;
namespace Nodes
{
    class CsvLoaderModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数：初始化选择器与端口，绑定文件选择到CSV加载
         */
        CsvLoaderModel() {

            InPortCount =0;
            OutPortCount=1;

            CaptionVisible=true;
            Caption="CSV File";
            WidgetEmbeddable= true;
            Resizable=false;
            PortEditable= false;
            m_outJsonData=std::make_shared<VariableData>();
            NodeDelegateModel::registerOSCControl("/file",_fileSelectComboBox);
            connect(_fileSelectComboBox, &SelectorComboBox::textChanged, this, &CsvLoaderModel::loadCsv);
        }
        ~CsvLoaderModel() override = default;

    public:
        /**
         * @brief 端口类型：统一使用VariableData（用于承载CSV表结构）
         */
        NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            return VariableData().type();
        }

        /**
         * @brief 输出端口返回当前解析后的CSV表数据
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
         * @brief 按保存的路径加载文件并解析为CSV
         */
        void load(QJsonObject const& jsonObj) override{
            const QJsonValue path = jsonObj["path"];
            if (!path.isUndefined()) {
               _fileSelectComboBox->setText(path.toString());
                loadCsv(path.toString());
            }
        }

        /**
         * @brief 将节点状态通过OSC反馈（保持原有逻辑）
         */
        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    private:
        /**
         * @brief 解析一行CSV文本为字段列表，支持RFC4180基本规则
         * @param line 单行文本
         * @return 字段列表
         */
        QStringList parseCsvLine(const QString& line) {
            QStringList fields;
            QString field;
            bool inQuotes = false;

            for (int i = 0; i < line.size(); ++i) {
                QChar c = line.at(i);
                if (inQuotes) {
                    if (c == '"') {
                        // 处理转义的双引号 ""
                        if (i + 1 < line.size() && line.at(i + 1) == '"') {
                            field.append('"');
                            ++i;
                        } else {
                            inQuotes = false;
                        }
                    } else {
                        field.append(c);
                    }
                } else {
                    if (c == '"') {
                        inQuotes = true;
                    } else if (c == ',') {
                        fields.append(field);
                        field.clear();
                    } else {
                        field.append(c);
                    }
                }
            }
            fields.append(field);
            return fields;
        }

        /**
         * @brief 打开文本文件并解析为CSV表结构，输出到端口0
         * @param fileName 媒体库中的文件名或相对路径
         *
         * 行为：
         * - 首行作为表头；若表头包含空列名，则以 col0/col1... 补齐
         * - 后续各行解析为记录；列数不足以空字符串补齐，超出则忽略多余列
         * - 输出结构：
         *   - headers: QVariantList（字符串列表）
         *   - rows: QVariantList（每行一个 QVariantMap，键为表头）
         *   - default: 同 rows，方便下游直接取默认值
         * - 失败：清空输出并反馈错误
         */
        void loadCsv(QString fileName){
            if (fileName.isEmpty()) {
                m_outJsonData.reset();
                emit dataUpdated(0);
                return;
            }

            const QString fullPath = AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + fileName;

            QFile f(fullPath);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qWarning() << "CsvLoaderModel: cannot open file" << fullPath;
                m_outJsonData.reset();
                updateNodeState(QtNodes::NodeValidationState::State::Error,"cannot open file");
                emit dataUpdated(0);
                return;
            }

            QTextStream ts(&f);
            // ts.setCodec("UTF-8");
            QStringList lines;
            while (!ts.atEnd()) {
                QString line = ts.readLine();
                // 去除UTF-8 BOM
                if (!line.isEmpty() && line.front().unicode() == 0xFEFF) {
                    line.remove(0, 1);
                }
                lines.append(line);
            }
            f.close();

            if (lines.isEmpty()) {
                qWarning() << "CsvLoaderModel: empty file" << fullPath;
                m_outJsonData.reset();
                updateNodeState(QtNodes::NodeValidationState::State::Error,"empty file");
                emit dataUpdated(0);
                return;
            }

            // 解析表头
            QStringList headerFields = parseCsvLine(lines.first());
            for (int i = 0; i < headerFields.size(); ++i) {
                if (headerFields[i].isEmpty()) {
                    headerFields[i] = QString("col%1").arg(i);
                }
            }

            // 解析数据行
            QVariantList rowList;
            for (int r = 1; r < lines.size(); ++r) {
                QStringList fields = parseCsvLine(lines[r]);
                QVariantMap row;
                int colCount = qMin(headerFields.size(), fields.size());
                for (int c = 0; c < colCount; ++c) {
                    row.insert(headerFields[c], fields[c]);
                }
                // 如果列数不足，用空字符串补齐
                for (int c = fields.size(); c < headerFields.size(); ++c) {
                    row.insert(headerFields[c], "");
                }
                rowList.push_back(row);
            }

            // 组装输出数据
            m_path = fullPath;
            auto out = std::make_shared<VariableData>();
            QVariantList headerVar;
            for (const auto& h : headerFields) headerVar.push_back(h);
            out->insert("headers", headerVar);
            out->insert("rows", rowList);
            out->insert("default", rowList); // 默认值即整表行列表，便于下游直接使用
            m_outJsonData = out;

            updateNodeState(QtNodes::NodeValidationState::State::Valid);
            emit dataUpdated(0);
        }

    private:

        SelectorComboBox* _fileSelectComboBox = new SelectorComboBox(MediaLibrary::Category::Document);
        QString m_path;
        std::shared_ptr<VariableData> m_outJsonData;

    };
}