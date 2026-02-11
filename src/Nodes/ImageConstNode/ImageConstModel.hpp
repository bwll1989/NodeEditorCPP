#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "ImageConstInterface.hpp"
#include "DataTypes/NodeDataList.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using namespace NodeDataTypes;
using namespace Nodes;
namespace Nodes
{
    class ImageConstModel final : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        ImageConstModel(){
            InPortCount = 3;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Image Constant";
            WidgetEmbeddable=false;
            Resizable=false;
            m_watcher = new QFutureWatcher<void>(this);
            // 连接输入框变化信号到槽
            connect(widget->widthEdit, &IntDragValueWidget::valueChanged, this, &ImageConstModel::onInputChanged);
            connect(widget->heightEdit, &IntDragValueWidget::valueChanged, this, &ImageConstModel::onInputChanged);
            connect(colorEditorWidget, &ColorEditorWidget::colorChanged, this, &ImageConstModel::onInputChanged);
            connect(widget->colorEditButton, &QPushButton::clicked, this, &ImageConstModel::toggleEditorMode);
            updateImage();
        };

        ~ImageConstModel() override{
            if (colorEditorWidget) {
                colorEditorWidget->setParent(nullptr);
                colorEditorWidget->deleteLater();
            }
        }

        QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            switch(portType){
                case QtNodes::PortType::In:
                    return VariableData().type();
                case QtNodes::PortType::Out:
                    return ImageData().type();
                default:
                    return VariableData().type();
            }
        };

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override{
            if (portType == QtNodes::PortType::In)
            {
                switch (portIndex)
                {
                    case 0:
                        return "WIDTH";
                    case 1:
                        return "HEIGHT";
                    case 2:
                        return "RED";
                    case 3:
                        return "GREEN";
                    case 4:
                        return "BLUE";
                    case 5:
                        return "ALPHA";
                    default:
                        return "";
                }
            }else
            {
                return "Image";
            }
        }

        std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override{
            // 输出当前生成的图像
            if (m_image.empty()) return nullptr;
            return std::make_shared<ImageData>(m_image);
        }

        void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override{
            if (!nodeData) return;
            auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (!v) return;
            QVariant val = v->value();
            switch (port) {
                case 0: // WIDTH
                    m_width = val.toInt();
                    widget->widthEdit->setValue(m_width);
                    break;
                case 1: // HEIGHT
                    m_height = val.toInt();
                    widget->heightEdit->setValue(m_height);
                    break;
                case 2: // RED
                    colorEditorWidget->setColor(val.toString());

                default:
                    break;
            }
            updateImage();
            Q_EMIT dataUpdated(0);
        }

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override{
            QJsonObject modelJson1;
            modelJson1["width"] = widget->widthEdit->value();
            modelJson1["height"] = widget->heightEdit->value();
            modelJson1["color"] = colorEditorWidget->getColor().name(QColor::HexArgb);
            // modelJson1["green"] = widget->colorGreenEdit->value();
            // modelJson1["blue"] = widget->colorBlueEdit->value();
            // modelJson1["alpha"] = widget->colorAlphaEdit->value();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override{
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                //            button->setChecked(v["val"].toBool(false));
                widget->widthEdit->setValue(v["width"].toInt());
                widget->heightEdit->setValue(v["height"].toInt());
                colorEditorWidget->setColor(QColor(v["color"].toString()));
                // widget->colorRedEdit->setValue(v["red"].toInt());
                // widget->colorGreenEdit->setValue(v["green"].toInt());
                // widget->colorBlueEdit->setValue(v["blue"].toInt());
                // widget->colorAlphaEdit->setValue(v["alpha"].toInt());
                updateImage();
            }
        }

    public Q_SLOTS:
        void toggleEditorMode(){
            // 移除父子关系，使其成为独立窗口
            colorEditorWidget->setParent(nullptr);

            // 设置为独立窗口
            colorEditorWidget->setWindowTitle("颜色编辑器");

            // 设置窗口图标
            colorEditorWidget->setWindowIcon(QIcon(":/icons/icons/curve.png"));

            // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
            colorEditorWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);

            // 设置窗口属性：当关闭时自动删除
            colorEditorWidget->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
            colorEditorWidget->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序

            // 设置窗口大小和显示
            colorEditorWidget->resize(800, 400);
            colorEditorWidget->show();
            // 激活窗口并置于前台
            colorEditorWidget->activateWindow();
            colorEditorWidget->raise();
        }

    private Q_SLOTS:
        void updateImage(){
        if (m_width <= 0) m_width = 1;
        if (m_height <= 0) m_height = 1;

        // // 立即预览当前颜色
        QPixmap pix(widget->display->width(), widget->display->height());
        pix.fill(m_color);
        widget->display->setPixmap(pix);

        // 异步生成图像
        auto width = m_width;
        auto height = m_height;
        auto color = m_color;
        QFuture<void> future = QtConcurrent::run([this, width, height, color]() {
            // 仅当尺寸变化时重新分配内存
            if (m_image.rows != height || m_image.cols != width) {
                m_image.create(height, width, CV_8UC4);
            }

            // 直接操作现有矩阵内存
            m_image.forEach<cv::Vec4b>([color](cv::Vec4b &pixel, const int*) {
                pixel = cv::Vec4b(
                    color.blue(),
                    color.green(),
                    color.red(),
                    color.alpha()
                );
            });
        });

        QObject::disconnect(m_watcher, nullptr, nullptr, nullptr);
        QObject::connect(m_watcher, &QFutureWatcher<void>::finished, this, [this]() {
            Q_EMIT dataUpdated(0);
        });
        m_watcher->setFuture(future);
    }

        void onInputChanged(){
            // 解析输入框内容
            m_width = widget->widthEdit->value();
            m_height = widget->heightEdit->value();
            m_color = QColor(colorEditorWidget->getColor());
            updateImage();
            Q_EMIT dataUpdated(0);
        }
    private:
        ImageConstInterface *widget = new ImageConstInterface();
        std::weak_ptr<ImageData> m_outData;
        int m_width = 100;
        int m_height = 100;
        ColorEditorWidget *colorEditorWidget=new ColorEditorWidget();
        QColor m_color = QColor(0,0,0,255);
        cv::Mat m_image=cv::Mat(100, 100, CV_8UC4, cv::Scalar(0, 0, 0, 255));;
        QFutureWatcher<void> *m_watcher = nullptr;
    };
}