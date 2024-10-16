#pragma once

#include "QtNodes/NodeDelegateModel"
#include "QThread"
#include <QtCore/QObject>
#include "QDir"
#include <iostream>
#include "../NodeDataList.hpp"
#include <pybind11/pybind11.h>
#include "iostream"
#include <QtCore/qglobal.h>
#include <QComboBox>
#include "PythonScriptInterface.hpp"
#include <pybind11/embed.h>
#include <QToolBox>
#include <utility>

namespace py = pybind11;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace std;
class QDebugStream {
public:
    void write(const std::string& str) {
        QString qstr = QString::fromStdString(str).trimmed();
        if (qstr!="")
        {
            qDebug().noquote() << qstr;
        }

    }
    void flush() {}
};



class Worker : public QObject {
Q_OBJECT
public:
    Worker(QString source_code,unordered_map<unsigned int, QVariant> input): code(std::move(source_code)),in(std::move(input)){

    }


public slots:
//    setenv("PYTHONIOENCODING", "utf-8", 1);

    void doWork() {
            // 初始化Python解释器
        try {
//            py::scoped_interpreter python;

            py::scoped_interpreter guard{};
            // 调用Python代码
            py::module sys= py::module_::import("sys");
            // 将QDebugStream绑定到Python中
            py::class_<QDebugStream>(py::module::import("__main__"), "QDebugStream")
                    .def(py::init<>())
                    .def("write", &QDebugStream::write)
                    .def("flush", &QDebugStream::flush);

            QDebugStream qDebugStream;
            sys.attr("stdout") = qDebugStream;

            py::module main= py::module_::import("__main__");
            py::object global=main.attr("__dict__");

            py::exec(code.toStdString().c_str(),global);
//            int arg1=10;

            py::object res=global["EventHandler"](convert_to_python_dict(in));
            if (!res.is_none()) {
                out= py_dict_to_map(res);
                emit workResult(out);
            }

        } catch (const py::error_already_set& e) {
            qDebug() << "Python error:" << e.what();

        } catch (const std::exception& e) {
            qDebug() << "C++ error:" << e.what();

        } catch (...) {
            qDebug() << "Unknown error occurred";

        }
        emit workFinished();

    }
    // 将 C++ 字典转换为 Python 字典
    py::dict convert_to_python_dict(const std::unordered_map<unsigned int, QVariant>& dict) {
        py::dict py_dict;
        for (const auto& pair : dict) {
            unsigned int key = pair.first;
            const QVariant& value = pair.second;

            switch (value.metaType().id()) {
                case QMetaType::Int:
                    py_dict[py::int_(key)] = value.toInt();
                    break;
                case QMetaType::Double:
                    py_dict[py::int_(key)] = value.toDouble();
                    break;
                case QMetaType::QString:
                    py_dict[py::int_(key)] = value.toString().toStdString();
                    break;
                case QMetaType::Bool:
                    py_dict[py::int_(key)] = value.toBool();
                    break;
                default:
                    py_dict[py::int_(key)] = py::none();
                    break;
            }
        }
        return py_dict;
    }
    // 将 python 字典转换为 C++ 字典
    std::unordered_map<unsigned int, QVariant> py_dict_to_map(const py::dict& py_dict) {
        std::unordered_map<unsigned int, QVariant> result;
        for (auto item : py_dict) {
            unsigned int key = py::cast<unsigned int>(item.first);
            py::object py_value = py::reinterpret_borrow<py::object>(item.second);
            if (py::isinstance<py::int_>(py_value)) {
                result[key] = py::cast<int>(py_value); // 转换为 int
            } else if (py::isinstance<py::float_>(py_value)) {
                result[key] = py::cast<double>(py_value); // 转换为 double
            } else if (py::isinstance<py::str>(py_value)) {
                result[key] = py::cast<std::string>(py_value).c_str(); // 转换为 std::string
            } else if (py::isinstance<py::bool_>(py_value)) {
                result[key] = py::cast<bool>(py_value); // 转换为 std::string
            } else {
                // 其他类型的处理，或者抛出异常
                // 注意：这里可能需要根据您的需求添加更多的类型转换
            }
        }

        return result;
    }
signals:
    void workFinished();
    void workResult(std::unordered_map<unsigned int, QVariant>);
public:
    QString code;
    unordered_map<unsigned int, QVariant> in;
    unordered_map<unsigned int, QVariant> out;
};

class PythonDataModel : public NodeDelegateModel
{
Q_OBJECT

public:
    PythonDataModel()
    {
        CaptionVisible=true;
        Caption="Python Script";
        WidgetEmbeddable=false;
        InPortCount=1;
        OutPortCount=1;
        Resizable=false;
        PortEditable=true;
        connect(widget->codeWidget->run,SIGNAL(clicked(bool)),this,SLOT(runScripts()));
    }

    ~PythonDataModel()
    {
        if(thread->isRunning()){
            thread->quit();
            thread->wait();
        }
        widget->deleteLater();
//        delete thread;
        thread->deleteLater();
    }

public:

    QString caption() const override { return Caption; }
    QString name() const override { return Caption; }
    bool widgetEmbeddable() const override { return WidgetEmbeddable; }
    bool captionVisible() const override{ return CaptionVisible; }
    bool resizable() const override { return Resizable; }
    bool portCaptionVisible(QtNodes::PortType, QtNodes::PortIndex) const override{
        return true;
    }
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "➩";
        QString out = "➩";
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
    }
public Q_SLOTS:

    void displayResult(unordered_map<unsigned int, QVariant> res)
    {

        for (const auto& pair : res) {
            unsigned int key = pair.first;
            const QVariant& value = pair.second;
            // 检查键是否在 map2 中存在
            auto it = out_dictionary.find(key);
            if (it == res.end() || it->second != res[key]) {
                // 键不存在于 map2 中，或者与 map2 中的值不同
                out_dictionary[key]=res[key];
                //更新结果存储
                Q_EMIT dataUpdated(key);
//                端口输出
            }
        }
//        this->widget->adjustSize();
    }
    void runScripts()
    {
        if(!thread->isRunning()) {

            Worker *worker = new Worker(widget->get_code(),in_dictionary);

            worker->moveToThread(thread);
            QObject::connect(thread, &QThread::started, worker, &Worker::doWork);
            QObject::connect(worker, &Worker::workFinished, thread, &QThread::quit);
            QObject::connect(worker, &Worker::workFinished, worker, &Worker::deleteLater);
//            QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);
            QObject::connect(worker, &Worker::workResult, this, &PythonDataModel::displayResult);
            thread->start();
        }
    }

public:
    unsigned int nPorts(PortType portType) const override
    {

        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result =InPortCount;
                break;

            case PortType::Out:
                result = OutPortCount;

            default:
                break;
        }

        return result;

    };

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        switch (portType) {
            case PortType::In:
                return VariantData().type();
            case PortType::Out:
                return VariantData().type();
            case PortType::None:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return TextData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex port) override
    {
        return std::make_shared<VariantData>(out_dictionary[port]);
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
        if (auto d = std::dynamic_pointer_cast<VariantData>(data))
        {
            in_dictionary[portIndex] = d->NodeValues;
            runScripts();
        }

    }

    QWidget *embeddedWidget() override
    {

        return widget;
    }

    QJsonObject save() const
    {
        QJsonObject modelJson1;
        modelJson1["scripts"] = widget->codeWidget->saveCode();
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        modelJson["InPortCount"] =int(InPortCount);
        modelJson["OutPortCount"] =int(OutPortCount);
        return modelJson;
    };

    void load(const QJsonObject &p) {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            script = v["scripts"].toString();
            widget->codeWidget->loadCodeFromCode(script);
            InPortCount=p["InPortCount"].toInt();
            OutPortCount=p["OutPortCount"].toInt();
        };
    }

    QString greet(const QString &name) {
        return QString("Hello, ") + name + "!";
    }
private:
    QString script;
    unordered_map<unsigned int, QVariant> in_dictionary;
    unordered_map<unsigned int, QVariant> out_dictionary;
    QThread* thread = new QThread(this);
    PythonScriptInterface *widget=new PythonScriptInterface();
};

