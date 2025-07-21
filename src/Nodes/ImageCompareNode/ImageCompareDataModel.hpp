#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QtConcurrent/QtConcurrent>
#include <QAbstractScrollArea>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "ImageCompareInterface.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;
namespace Nodes
{
    class ImageCompareDataModel : public NodeDelegateModel
    {
        Q_OBJECT

        public:
        ImageCompareDataModel()
        {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
            m_outVariable=std::make_shared<VariableData>();
            connect(widget->methodEdit,&QComboBox::currentIndexChanged,this,&ImageCompareDataModel::calculateImagedifference);
            NodeDelegateModel::registerOSCControl("/method",widget->methodEdit);

        }

        virtual ~ImageCompareDataModel() override{}

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                    return "IMAGE 0";
                case 1:
                    return "IMAGE 1";
                default:
                    return "INPUT "+QString::number(portIndex);
                }
            case PortType::Out:
                return "OUTPUT "+QString::number(portIndex);
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            // Q_UNUSED(portIndex);
            // Q_UNUSED(portType);
            switch(portType){
            case PortType::In:
                return ImageData().type();
            case PortType::Out:
                return VariableData().type();
            default:
                return ImageData().type();
            }
        }



        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return m_outVariable;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            switch (portIndex)
            {
            case 0:
                m_inImage0=std::dynamic_pointer_cast<ImageData>(data);
                calculateImagedifference();
                break;
            case 1:
                m_inImage1=std::dynamic_pointer_cast<ImageData>(data);
                calculateImagedifference();
                break;
            }
        }
        void calculateImagedifference()
{
    if (m_inImage0 && m_inImage1) {
        // 获取当前方法索引（主线程操作）
        const int methodIndex = widget->methodEdit->currentIndex();

        // 深拷贝图像数据用于异步处理
        cv::Mat img0 = m_inImage0->imgMat().clone();
        cv::Mat img1 = m_inImage1->imgMat().clone();

        // 启动异步任务
        QFuture<double> future = QtConcurrent::run([this, methodIndex, img0, img1]() {
            // 转换为灰度图像
            cv::Mat gray0, gray1;
            cv::cvtColor(img0, gray0, cv::COLOR_BGR2GRAY);
            cv::cvtColor(img1, gray1, cv::COLOR_BGR2GRAY);

            // 强制统一为最小尺寸
            const cv::Size min_size(
                std::min(gray0.cols, gray1.cols),
                std::min(gray0.rows, gray1.rows)
            );
            if (gray0.size() != min_size) cv::resize(gray0, gray0, min_size, 0, 0, cv::INTER_AREA);
            if (gray1.size() != min_size) cv::resize(gray1, gray1, min_size, 0, 0, cv::INTER_AREA);

            double result = 0.0;
            switch (methodIndex) {
            case 0: // MSE
                result = cv::norm(gray0, gray1, cv::NORM_L2SQR) / (gray0.total()*255.0*255.0);
                break;
            case 1: // SSIM
                result = calculateSSIM(gray0, gray1);
                break;
            case 2: // PSNR
                double mse = cv::norm(gray0, gray1, cv::NORM_L2SQR) / gray0.total();
                result = 10.0 * log10((255.0 * 255.0) / mse);
                break;
            }
            return result;
        });

        // 连接完成信号
        if (m_watcher) {
            QObject::disconnect(m_watcher, nullptr, nullptr, nullptr);
            m_watcher->deleteLater();
        }
        m_watcher = new QFutureWatcher<double>(this);
        QObject::connect(m_watcher, &QFutureWatcher<double>::finished, this, [this]() {
            m_outVariable = std::make_shared<VariableData>(m_watcher->result());
            Q_EMIT dataUpdated(0);
        });
        m_watcher->setFuture(future);
    }
}

// 添加SSIM计算方法
    double calculateSSIM(const cv::Mat& i1, const cv::Mat& i2) {

            const double C1 = 6.5025, C2 = 58.5225;
            // 添加尺寸校验
            CV_Assert(i1.size() == i2.size() && i1.type() == i2.type());

            // 修改核尺寸计算为基于最小边长
            const int min_dim = std::min(i1.rows, i1.cols);
            const int kernel_size = std::max(3, (min_dim / 50) | 1);
            // 统一使用动态高斯核参数（基于固定比例）
            const double sigma = kernel_size / 3.0;

            cv::Mat I1, I2;
            i1.convertTo(I1, CV_32F);
            i2.convertTo(I2, CV_32F);

            cv::Mat I1_2 = I1.mul(I1);
            cv::Mat I2_2 = I2.mul(I2);
            cv::Mat I1_I2 = I1.mul(I2);

            cv::Mat mu1, mu2;
            GaussianBlur(I1, mu1, cv::Size(kernel_size,kernel_size), sigma, sigma, cv::BORDER_REPLICATE);
            GaussianBlur(I2, mu2, cv::Size(kernel_size,kernel_size), sigma, sigma, cv::BORDER_REPLICATE);

            cv::Mat mu1_2 = mu1.mul(mu1);
            cv::Mat mu2_2 = mu2.mul(mu2);
            cv::Mat mu1_mu2 = mu1.mul(mu2);

            cv::Mat sigma1_2, sigma2_2, sigma12;
            GaussianBlur(I1_2, sigma1_2, cv::Size(kernel_size,kernel_size), sigma, sigma, cv::BORDER_REPLICATE);
            sigma1_2 -= mu1_2;

            GaussianBlur(I2_2, sigma2_2, cv::Size(kernel_size,kernel_size), sigma, sigma, cv::BORDER_REPLICATE);
            sigma2_2 -= mu2_2;

            GaussianBlur(I1_I2, sigma12, cv::Size(kernel_size,kernel_size), sigma, sigma, cv::BORDER_REPLICATE);
            sigma12 -= mu1_mu2;

        cv::Mat t1 = 2*mu1_mu2 + C1;
        cv::Mat t2 = 2*sigma12 + C2;
        cv::Mat t3 = t1.mul(t2);

        t1 = mu1_2 + mu2_2 + C1;
        t2 = sigma1_2 + sigma2_2 + C2;
        t1 = t1.mul(t2);

        cv::Mat ssim_map;
        divide(t3, t1, ssim_map);

        return cv::mean(ssim_map).val[0]*100;
    }
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson1["method"] = widget->methodEdit->currentIndex();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {

                widget->methodEdit->setCurrentIndex(v.toObject()["method"].toInt());
            }
        }

    private:
        QFutureWatcher<double>* m_watcher = nullptr;
        ImageCompareInterface *widget=new ImageCompareInterface();
        std::shared_ptr<ImageData> m_inImage0;
        std::shared_ptr<ImageData> m_inImage1;
        std::shared_ptr<VariableData> m_outVariable;
    };
}