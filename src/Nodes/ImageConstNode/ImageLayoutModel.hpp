#pragma once

#include <memory>
#include <algorithm>
#include <unordered_map>
#include <vector>

#include <QtCore/QObject>
#include <QPointer>
#include <atomic>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "ImageLayoutInterface.hpp"
#include "ImageConstCommon.hpp"
#include "ImageLayoutGpu.hpp"
#include "NodeDataList.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/ImageReadback.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace NodeDataTypes;
using namespace Nodes;

namespace Nodes
{
class ImageLayoutModel final : public AbstractDelegateModel
{
    Q_OBJECT

public:
    ImageLayoutModel()
    {
        InPortCount = 4;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Image Layout";
        WidgetEmbeddable = false;
        Resizable = false;
        PortEditable = true;
        ImageConstHelpers::ensureOutputBuffer(m_outImageData, m_outBuffer);

        connect(widget->widthEdit, &IntDragValueWidget::valueChanged, this, &ImageLayoutModel::onInputChanged);
        connect(widget->heightEdit, &IntDragValueWidget::valueChanged, this, &ImageLayoutModel::onInputChanged);
        connect(widget->layoutCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &ImageLayoutModel::onInputChanged);
        connect(widget->rowsSpin, &IntDragValueWidget::valueChanged, this, &ImageLayoutModel::onInputChanged);
        connect(widget->colsSpin, &IntDragValueWidget::valueChanged, this, &ImageLayoutModel::onInputChanged);
        connect(widget->spacingSpin, &IntDragValueWidget::valueChanged, this, &ImageLayoutModel::onInputChanged);
        connect(colorEditorWidget, &ColorEditorWidget::colorChanged, this, &ImageLayoutModel::onInputChanged);
        connect(widget->colorEditButton, &QPushButton::clicked, this, &ImageLayoutModel::toggleEditorMode);
    }

    ~ImageLayoutModel() override
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
                [self = QPointer<ImageLayoutModel>(this)](qint64) {
                    if (!self || self->m_shuttingDown.load()) {
                        return;
                    }
                    self->publishFrame();
                },
                Qt::QueuedConnection);

        syncParamsFromWidget();
        m_paramsDirty = true;
        publishFrame();
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex);
        return ImageData().type();
    }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In) {
            return "Image" + QString::number(portIndex + 1);
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
        auto image = std::dynamic_pointer_cast<ImageData>(nodeData);
        if (image) {
            m_inImages[port] = image;
        } else {
            m_inImages.erase(port);
        }
        m_lastSeenInputTimestamp = -1;
        m_paramsDirty = true;
    }

    QWidget* embeddedWidget() override { return widget; }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["width"] = widget->widthEdit->value();
        modelJson1["height"] = widget->heightEdit->value();
        modelJson1["layout"] = widget->layoutCombo->currentIndex();
        modelJson1["rows"] = widget->rowsSpin->value();
        modelJson1["cols"] = widget->colsSpin->value();
        modelJson1["spacing"] = widget->spacingSpin->value();
        modelJson1["color"] = colorEditorWidget->getColor().name(QColor::HexArgb);
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            widget->widthEdit->setValue(v["width"].toInt(0));
            widget->heightEdit->setValue(v["height"].toInt(0));
            widget->layoutCombo->setCurrentIndex(v["layout"].toInt(0));
            widget->rowsSpin->setValue(v["rows"].toInt(2));
            widget->colsSpin->setValue(v["cols"].toInt(2));
            widget->spacingSpin->setValue(v["spacing"].toInt(0));
            colorEditorWidget->setColor(QColor(v["color"].toString()));
            syncParamsFromWidget();
            m_paramsDirty = true;
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
    }

private:
    void syncParamsFromWidget()
    {
        const int w = widget->widthEdit->value();
        const int h = widget->heightEdit->value();
        if (w > 0) {
            m_width = w;
        }
        if (h > 0) {
            m_height = h;
        }
        m_color = QColor(colorEditorWidget->getColor());
    }

    void computeGrid(int inputCount, int mode, int rows, int cols, int& gridRows, int& gridCols) const
    {
        gridRows = 1;
        gridCols = 1;
        if (inputCount <= 0) {
            return;
        }
        if (mode == 0) {
            gridRows = 1;
            gridCols = inputCount;
        } else if (mode == 1) {
            gridRows = inputCount;
            gridCols = 1;
        } else {
            gridRows = std::max(1, rows);
            gridCols = std::max(1, cols);
            const int needRows = (inputCount + gridCols - 1) / gridCols;
            if (needRows > gridRows) {
                gridRows = needRows;
            }
        }
    }

    std::vector<ImageLayoutGpu::LayoutCell> buildLayoutCells() const
    {
        std::vector<ImageFrame> frames;
        frames.reserve(m_inImages.size());
        for (int i = 0; i < InPortCount; ++i) {
            auto it = m_inImages.find(i);
            if (it == m_inImages.end() || !it->second) {
                continue;
            }
            ImageFrame frame;
            if (!ImageConstHelpers::resolveLatestGpuFrame(it->second, frame)) {
                continue;
            }
            frames.push_back(frame);
        }

        const int n = static_cast<int>(frames.size());
        const int mode = widget->layoutCombo->currentIndex();
        const int rows = widget->rowsSpin->value();
        const int cols = widget->colsSpin->value();
        const int spacing = widget->spacingSpin->value();

        int gridRows = 1;
        int gridCols = 1;
        computeGrid(n, mode, rows, cols, gridRows, gridCols);

        const int safeSpacing = std::max(0, spacing);
        const int cellW = std::max(1, (m_width - safeSpacing * (gridCols - 1)) / gridCols);
        const int cellH = std::max(1, (m_height - safeSpacing * (gridRows - 1)) / gridRows);

        std::vector<ImageLayoutGpu::LayoutCell> cells;
        cells.reserve(n);
        for (int i = 0; i < n && i < gridRows * gridCols; ++i) {
            const int r = i / gridCols;
            const int c = i % gridCols;
            const int x = c * (cellW + safeSpacing);
            const int y = r * (cellH + safeSpacing);
            if (x < 0 || y < 0 || x + cellW > m_width || y + cellH > m_height) {
                continue;
            }
            ImageLayoutGpu::LayoutCell cell;
            cell.texture = frames[static_cast<size_t>(i)].texture;
            cell.x = x;
            cell.y = y;
            cell.w = cellW;
            cell.h = cellH;
            cells.push_back(cell);
        }
        return cells;
    }

    bool composeCpuFallback()
    {
        cv::Mat canvas(m_height, m_width, CV_8UC4,
                       cv::Scalar(m_color.blue(), m_color.green(), m_color.red(), m_color.alpha()));

        std::vector<cv::Mat> mats;
        for (int i = 0; i < InPortCount; ++i) {
            auto it = m_inImages.find(i);
            if (it == m_inImages.end() || !it->second) {
                continue;
            }
            ImageFrame frame;
            if (!getLatestImageFrame(it->second, frame) || frame.empty()) {
                continue;
            }
            cv::Mat src = ImageReadback::matFromFrame(frame);
            if (src.empty()) {
                continue;
            }
            cv::Mat bgra;
            if (src.channels() == 4) {
                bgra = src;
            } else if (src.channels() == 3) {
                cv::cvtColor(src, bgra, cv::COLOR_BGR2BGRA);
            } else if (src.channels() == 1) {
                cv::cvtColor(src, bgra, cv::COLOR_GRAY2BGRA);
            } else {
                continue;
            }
            mats.push_back(bgra);
        }

        const int n = static_cast<int>(mats.size());
        int gridRows = 1;
        int gridCols = 1;
        computeGrid(n, widget->layoutCombo->currentIndex(), widget->rowsSpin->value(),
                    widget->colsSpin->value(), gridRows, gridCols);

        const int safeSpacing = std::max(0, widget->spacingSpin->value());
        const int cellW = std::max(1, (m_width - safeSpacing * (gridCols - 1)) / gridCols);
        const int cellH = std::max(1, (m_height - safeSpacing * (gridRows - 1)) / gridRows);

        for (int i = 0; i < n && i < gridRows * gridCols; ++i) {
            const int r = i / gridCols;
            const int c = i % gridCols;
            const int x = c * (cellW + safeSpacing);
            const int y = r * (cellH + safeSpacing);
            if (x < 0 || y < 0 || x + cellW > m_width || y + cellH > m_height) {
                continue;
            }
            cv::Mat resized;
            cv::resize(mats[static_cast<size_t>(i)], resized, cv::Size(cellW, cellH), 0, 0, cv::INTER_LINEAR);
            resized.copyTo(canvas(cv::Rect(x, y, cellW, cellH)));
        }

        ImageConstHelpers::ensureOutputBuffer(m_outImageData, m_outBuffer);
        ImageConstHelpers::pushBgraFrame(m_outBuffer, std::move(canvas), m_lastPushedTimestamp);
        return true;
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

        qint64 latestInputTimestamp = -1;
        for (const auto& [portIndex, input] : m_inImages) {
            Q_UNUSED(portIndex);
            ImageFrame peek;
            if (input && getLatestImageFrame(input, peek) && !peek.empty()) {
                latestInputTimestamp = std::max(latestInputTimestamp, peek.timestamp);
            }
        }

        const bool inputUpdated =
            latestInputTimestamp >= 0 && latestInputTimestamp > m_lastSeenInputTimestamp;
        if (!m_paramsDirty && !inputUpdated && m_lastPushedTimestamp >= 0) {
            return;
        }

        const std::vector<ImageLayoutGpu::LayoutCell> cells = buildLayoutCells();
        GpuTextureHandle composed =
            ImageLayoutGpu::compose(m_width, m_height, m_color, cells);

        if (composed.valid()) {
            ImageConstHelpers::ensureOutputBuffer(m_outImageData, m_outBuffer);
            ImageConstHelpers::pushTextureFrame(m_outBuffer, std::move(composed), m_lastPushedTimestamp);
            m_lastSeenInputTimestamp = latestInputTimestamp;
            m_paramsDirty = false;
            return;
        }

        if (composeCpuFallback()) {
            m_lastSeenInputTimestamp = latestInputTimestamp;
            m_paramsDirty = false;
        }
    }

    ImageLayoutInterface* widget = new ImageLayoutInterface();
    std::unordered_map<int, std::shared_ptr<ImageData>> m_inImages;
    std::shared_ptr<ImageData> m_outImageData;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    qint64 m_lastPushedTimestamp = -1;
    qint64 m_lastSeenInputTimestamp = -1;
    int m_width = 100;
    int m_height = 100;
    ColorEditorWidget* colorEditorWidget = new ColorEditorWidget();
    QColor m_color = QColor(0, 0, 0, 255);
    bool m_paramsDirty = true;
    std::atomic<bool> m_shuttingDown{false};
};
} // namespace Nodes
