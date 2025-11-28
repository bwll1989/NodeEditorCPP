//
// Created by pablo on 2/28/24.
//

#pragma once
#include <QtNodes/NodeData>
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <QImage>
#include <QPixmap>
#include <io.h>       // 添加文件描述符支持
#include <cstdio>     // 添加FILE类型支持
#include <QVariant>  // 添加QVariant头文件
#include "DataTypesExport.h"
Q_DECLARE_METATYPE(cv::Mat);
namespace NodeDataTypes
{
    class DATATYPES_EXPORT ImageData final : public QtNodes::NodeData {
    public:
        ImageData() = default;

        explicit ImageData(QImage const &image) ;

        explicit ImageData(QString const &fileName) ;

        explicit ImageData(cv::Mat const &mat) ;

        QtNodes::NodeDataType type() const override ;

        // bool isNull() const { return m_image.isNull(); }
        //
        // bool isGrayScale() const { return m_image.isGrayscale(); }
        //
        // bool hasAlphaChannel() const { return m_image.hasAlphaChannel(); }
        //
        QImage image() const ;

        QPixmap pixmap() const ;

        cv::Mat imgMat() const ;
        bool hasKey(const QString &key) const ;

        bool isEmpty() const ;

        QVariant value(const QString &key = "default") const ;
        QVariantMap getMap() ;
        //
    private:
        cv::Mat m_image;
        QVariantMap NodeValues;
    };
}

