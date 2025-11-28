//
// Created by pablo on 2/28/24.
//

#include "ImageData.h"
using namespace  NodeDataTypes;


ImageData::ImageData(QImage const &image) {
    if (image.isNull()) return;

    // 转换为OpenCV兼容的BGR格式
    QImage swapped = image.convertToFormat(QImage::Format_RGB888).rgbSwapped();

    // 创建深拷贝的cv::Mat
    m_image = cv::Mat(
        swapped.height(),
        swapped.width(),
        CV_8UC3,
        const_cast<uchar*>(swapped.bits()),
        static_cast<size_t>(swapped.bytesPerLine())
    ).clone();
    NodeValues.insert("default", QVariant::fromValue(m_image));
}

ImageData::ImageData(QString const &fileName) {
    // 使用Windows API处理中文路径
    const std::wstring wpath = fileName.toStdWString();
    FILE* fp = nullptr;
    errno_t err = _wfopen_s(&fp, wpath.c_str(), L"rb");
    if (err == 0 && fp) {
        // 读取文件内容到内存缓冲区
        fseek(fp, 0, SEEK_END);
        const long size = ftell(fp);
        rewind(fp);
        std::vector<uchar> buffer(size);
        fread(buffer.data(), 1, size, fp);
        fclose(fp);

        // 使用imdecode解码图像
        m_image = cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
    } else {
        qWarning() << "无法打开文件:" << fileName;
        m_image = cv::Mat();
    }
    NodeValues.insert("default", QVariant::fromValue(m_image));
}

ImageData::ImageData(cv::Mat const &mat) : m_image(cv::Mat(mat))
{
    NodeValues.insert("default", QVariant::fromValue(m_image));
}

QtNodes::NodeDataType ImageData::type() const {
    return QtNodes::NodeDataType{"image", "image"};
}

// bool isNull() const { return m_image.isNull(); }
//
// bool isGrayScale() const { return m_image.isGrayscale(); }
//
// bool hasAlphaChannel() const { return m_image.hasAlphaChannel(); }
//
QImage ImageData::image() const {
    if (m_image.empty()) return QImage();

    cv::Mat outputMat;
    QImage::Format targetFormat = QImage::Format_RGB888;

    // 根据通道数处理颜色转换
    if (m_image.channels() == 4) {
        cv::cvtColor(m_image, outputMat, cv::COLOR_BGRA2RGBA);
        targetFormat = QImage::Format_RGBA8888;
    } else {
        cv::cvtColor(m_image, outputMat, cv::COLOR_BGR2RGB);
    }

    return QImage(
        outputMat.data,
        outputMat.cols,
        outputMat.rows,
        static_cast<int>(outputMat.step),
        targetFormat
    ).copy();
}

QPixmap ImageData::pixmap() const {
    return QPixmap::fromImage(image());
}

cv::Mat ImageData::imgMat() const {
    return m_image.clone();
}
bool ImageData::hasKey(const QString &key) const {
    return NodeValues.contains(key);
}

bool ImageData::isEmpty() const {
    return NodeValues.isEmpty();
}

QVariant ImageData::value(const QString &key ) const {
    return hasKey(key) ? NodeValues.value(key) : QVariant();
}
QVariantMap ImageData::getMap() {
    NodeValues.insert("width", QVariant::fromValue(m_image.size().width));
    NodeValues.insert("height", QVariant::fromValue(m_image.size().height));
    NodeValues.insert("isNull", isEmpty());
    NodeValues.insert("channels", QVariant::fromValue(m_image.channels()));
    return NodeValues;
}


