#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>

#include <QPointer>
#include <atomic>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "ImageConstInterface.hpp"
#include "ImageConstCommon.hpp"
#include "ImageConstGpu.hpp"
#include "NodeDataList.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

using namespace NodeDataTypes;
using namespace Nodes;

namespace Nodes
{
class ImageConstModel final : public AbstractDelegateModel
{
    Q_OBJECT

public:
    ImageConstModel()
    {
        InPortCount = 3;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Constant";
        WidgetEmbeddable = false;
        Resizable = false;
        ImageConstHelpers::ensureOutputBuffer(m_outImageData, m_outBuffer);

        connect(widget->widthEdit, &IntDragValueWidget::valueChanged, this, &ImageConstModel::onInputChanged);
        connect(widget->heightEdit, &IntDragValueWidget::valueChanged, this, &ImageConstModel::onInputChanged);
        connect(colorEditorWidget, &ColorEditorWidget::colorChanged, this, &ImageConstModel::onInputChanged);
        connect(widget->colorEditButton, &QPushButton::clicked, this, &ImageConstModel::toggleEditorMode);
        ImageConstHelpers::applyRgbaToEditor(colorEditorWidget, m_rgba);
        refreshPreview();
    }

    ~ImageConstModel() override
    {
        m_shuttingDown.store(true);
        disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);
        if (colorEditorWidget) {
            colorEditorWidget->setParent(nullptr);
            colorEditorWidget->deleteLater();
        }
    }

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        ImageGpuUpload::instance().warmup();

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [self = QPointer<ImageConstModel>(this)](qint64) {
                    if (!self || self->m_shuttingDown.load()) {
                        return;
                    }
                    self->publishFrame();
                },
                Qt::QueuedConnection);

        m_paramsDirty = true;
        publishFrame();
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        switch (portType) {
        case QtNodes::PortType::In:
            return VariableData().type();
        case QtNodes::PortType::Out:
            return ImageData().type();
        default:
            return VariableData().type();
        }
    }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In) {
            switch (portIndex) {
            case 0:
                return "WIDTH";
            case 1:
                return "HEIGHT";
            case 2:
                return "RGBA";
            default:
                return "";
            }
        }
        return "Image";
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override
    {
        Q_UNUSED(port);
        return m_outImageData;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex port) override
    {
        if (!nodeData) {
            return;
        }
        auto v = std::dynamic_pointer_cast<VariableData>(nodeData);
        if (!v) {
            return;
        }
        switch (port) {
        case 0:
            m_width = v->asInt();
            widget->widthEdit->setValue(m_width);
            break;
        case 1:
            m_height = v->asInt();
            widget->heightEdit->setValue(m_height);
            break;
        case 2:
            m_rgba = rgbaVectorFromVariant(v->value());
            ImageConstHelpers::applyRgbaToEditor(colorEditorWidget, m_rgba);
            break;
        default:
            break;
        }
        syncParamsFromWidget();
        m_paramsDirty = true;
        refreshPreview();
    }

    QWidget* embeddedWidget() override { return widget; }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["width"] = widget->widthEdit->value();
        modelJson1["height"] = widget->heightEdit->value();
        modelJson1["rgba"] = QJsonArray::fromVariantList(floatVectorToList(m_rgba));
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            widget->widthEdit->setValue(v["width"].toInt());
            widget->heightEdit->setValue(v["height"].toInt());
            m_rgba = ImageConstHelpers::loadRgbaValue(v.toObject(),
                QStringLiteral("rgba"), QStringLiteral("color"), m_rgba);
            ImageConstHelpers::applyRgbaToEditor(colorEditorWidget, m_rgba);
            m_width = widget->widthEdit->value();
            m_height = widget->heightEdit->value();
            m_paramsDirty = true;
            refreshPreview();
        }
    }

public Q_SLOTS:
    void toggleEditorMode()
    {
        colorEditorWidget->setParent(nullptr);
        colorEditorWidget->setWindowTitle("颜色编辑器");
        colorEditorWidget->setWindowIcon(QIcon(":/icons/icons/curve.png"));
        colorEditorWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
        colorEditorWidget->setAttribute(Qt::WA_DeleteOnClose, false);
        colorEditorWidget->setAttribute(Qt::WA_QuitOnClose, false);
        colorEditorWidget->resize(800, 400);
        colorEditorWidget->show();
        colorEditorWidget->activateWindow();
        colorEditorWidget->raise();
    }

private Q_SLOTS:
    void onInputChanged()
    {
        syncParamsFromWidget();
        m_paramsDirty = true;
        refreshPreview();
    }

private:
    void syncParamsFromWidget()
    {
        m_width = widget->widthEdit->value();
        m_height = widget->heightEdit->value();
        m_rgba = rgbaFromQColor(colorEditorWidget->getColor());
    }

    void refreshPreview()
    {
        if (m_width <= 0) {
            m_width = 1;
        }
        if (m_height <= 0) {
            m_height = 1;
        }
        QPixmap pix(widget->display->width(), widget->display->height());
        pix.fill(qcolorFromRgba(m_rgba));
        widget->display->setPixmap(pix);
    }

    void publishFrame()
    {
        if (m_shuttingDown.load()) {
            return;
        }
        if (m_width <= 0) {
            m_width = 1;
        }
        if (m_height <= 0) {
            m_height = 1;
        }
        if (!m_paramsDirty && m_lastPushedTimestamp >= 0) {
            return;
        }

        GpuTextureHandle texture = ImageConstGpu::solidColor(m_width, m_height, qcolorFromRgba(m_rgba));
        if (!texture.valid()) {
            return;
        }

        ImageConstHelpers::ensureOutputBuffer(m_outImageData, m_outBuffer);
        ImageConstHelpers::pushTextureFrame(m_outBuffer, std::move(texture), m_lastPushedTimestamp);
        m_paramsDirty = false;
    }

    ImageConstInterface* widget = new ImageConstInterface();
    ColorEditorWidget* colorEditorWidget = new ColorEditorWidget();
    std::shared_ptr<ImageData> m_outImageData;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    qint64 m_lastPushedTimestamp = -1;
    int m_width = 100;
    int m_height = 100;
    QVector<float> m_rgba{0.0f, 0.0f, 0.0f, 1.0f};
    bool m_paramsDirty = true;
    std::atomic<bool> m_shuttingDown{false};
};
} // namespace Nodes
