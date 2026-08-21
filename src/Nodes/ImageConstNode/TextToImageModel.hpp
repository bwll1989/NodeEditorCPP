#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QIcon>
#include <QPainter>
#include <QPointer>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <atomic>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "Elements/IntDragValueWidget/IntDragValueWidget.hpp"
#include "TextToImageInterface.hpp"
#include "ImageConstCommon.hpp"
#include "NodeDataList.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

using namespace NodeDataTypes;
using namespace Nodes;

namespace Nodes
{
class TextToImageModel final : public AbstractDelegateModel
{
    Q_OBJECT

public:
    TextToImageModel()
    {
        InPortCount = 9;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Text To Image";
        WidgetEmbeddable = false;
        Resizable = false;
        ImageConstHelpers::ensureOutputBuffer(m_outImageData, m_outBuffer);

        connect(widget->widthEdit, &IntDragValueWidget::valueChanged, this, &TextToImageModel::onInputChanged);
        connect(widget->heightEdit, &IntDragValueWidget::valueChanged, this, &TextToImageModel::onInputChanged);
        connect(widget->textEdit, &QTextEdit::textChanged, this, &TextToImageModel::onInputChanged);
        connect(widget->fontCombo, &QFontComboBox::currentFontChanged, this, &TextToImageModel::onInputChanged);
        connect(widget->fontSizeSpin, &IntDragValueWidget::valueChanged, this, &TextToImageModel::onInputChanged);
        connect(widget->boldCheck, &QCheckBox::toggled, this, &TextToImageModel::onInputChanged);
        connect(widget->italicCheck, &QCheckBox::toggled, this, &TextToImageModel::onInputChanged);
        connect(widget->underlineCheck, &QCheckBox::toggled, this, &TextToImageModel::onInputChanged);
        connect(widget->alignHCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TextToImageModel::onInputChanged);
        connect(widget->alignVCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TextToImageModel::onInputChanged);
        connect(widget->paddingSpin, &IntDragValueWidget::valueChanged, this, &TextToImageModel::onInputChanged);
        connect(widget->wrapCheck, &QCheckBox::toggled, this, &TextToImageModel::onInputChanged);

        connect(widget->textColorButton, &QPushButton::clicked, this, &TextToImageModel::toggleTextColorEditor);
        connect(widget->bgColorButton, &QPushButton::clicked, this, &TextToImageModel::toggleBackgroundColorEditor);
        m_textColorEditorWidget->setAlphaEnabled(true);
        m_bgColorEditorWidget->setAlphaEnabled(true);
        ImageConstHelpers::applyRgbaToEditor(m_textColorEditorWidget, m_textRgba);
        ImageConstHelpers::applyRgbaToEditor(m_bgColorEditorWidget, m_bgRgba);
        connect(m_textColorEditorWidget, &ColorEditorWidget::colorChanged, this, &TextToImageModel::onTextColorChanged);
        connect(m_bgColorEditorWidget, &ColorEditorWidget::colorChanged, this, &TextToImageModel::onBackgroundColorChanged);
    }

    ~TextToImageModel() override
    {
        m_shuttingDown.store(true);
        disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);
        if (m_textColorEditorWidget) {
            m_textColorEditorWidget->setParent(nullptr);
            m_textColorEditorWidget->deleteLater();
        }
        if (m_bgColorEditorWidget) {
            m_bgColorEditorWidget->setParent(nullptr);
            m_bgColorEditorWidget->deleteLater();
        }
    }

    void afterModelReady() override
    {
        AbstractDelegateModel::afterModelReady();
        ImageGpuUpload::instance().warmup();

        connect(TimestampGenerator::getInstance(),
                &TimestampGenerator::frameCountUpdated,
                this,
                [self = QPointer<TextToImageModel>(this)](qint64) {
                    if (!self || self->m_shuttingDown.load()) {
                        return;
                    }
                    self->publishFrame();
                },
                Qt::QueuedConnection);

        m_paramsDirty = true;
        renderImage();
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
                return "TEXT";
            case 3:
                return "FONT";
            case 4:
                return "FONTSIZE";
            case 5:
                return "TEXT_RGBA";
            case 6:
                return "BG_RGBA";
            case 7:
                return "HALIGN";
            case 8:
                return "VALIGN";
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
            widget->textEdit->setPlainText(v->asString());
            break;
        case 3:
            widget->fontCombo->setCurrentText(v->asString());
            break;
        case 4:
            widget->fontSizeSpin->setValue(v->asInt());
            break;
        case 5:
            m_textRgba = rgbaVectorFromVariant(v->value());
            ImageConstHelpers::applyRgbaToEditor(m_textColorEditorWidget, m_textRgba);
            break;
        case 6:
            m_bgRgba = rgbaVectorFromVariant(v->value());
            ImageConstHelpers::applyRgbaToEditor(m_bgColorEditorWidget, m_bgRgba);
            break;
        case 7:
            m_hAlign = v->asInt();
            if (m_hAlign < 0) {
                m_hAlign = 0;
            }
            if (m_hAlign > 2) {
                m_hAlign = 2;
            }
            widget->alignHCombo->setCurrentIndex(m_hAlign);
            break;
        case 8:
            m_vAlign = v->asInt();
            if (m_vAlign < 0) {
                m_vAlign = 0;
            }
            if (m_vAlign > 2) {
                m_vAlign = 2;
            }
            widget->alignVCombo->setCurrentIndex(m_vAlign);
            break;
        default:
            break;
        }

        m_paramsDirty = true;
        renderImage();
    }

    QWidget* embeddedWidget() override { return widget; }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["width"] = widget->widthEdit->value();
        modelJson1["height"] = widget->heightEdit->value();
        modelJson1["text"] = widget->textEdit->toPlainText();
        modelJson1["font"] = widget->fontCombo->currentFont().family();
        modelJson1["fontSize"] = widget->fontSizeSpin->value();
        modelJson1["textRgba"] = QJsonArray::fromVariantList(floatVectorToList(m_textRgba));
        modelJson1["bgRgba"] = QJsonArray::fromVariantList(floatVectorToList(m_bgRgba));
        modelJson1["hAlign"] = m_hAlign;
        modelJson1["vAlign"] = m_vAlign;
        modelJson1["padding"] = widget->paddingSpin->value();
        modelJson1["bold"] = widget->boldCheck->isChecked();
        modelJson1["italic"] = widget->italicCheck->isChecked();
        modelJson1["underline"] = widget->underlineCheck->isChecked();
        modelJson1["wrap"] = widget->wrapCheck->isChecked();
        modelJson1["antialia"] = widget->antialiasCheck->isChecked();
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        return modelJson;
    }

    void load(const QJsonObject& p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            widget->widthEdit->setValue(v["width"].toInt(512));
            widget->heightEdit->setValue(v["height"].toInt(256));
            m_width = widget->widthEdit->value();
            m_height = widget->heightEdit->value();
            widget->textEdit->setPlainText(v["text"].toString());
            widget->fontCombo->setCurrentText(v["font"].toString());
            widget->fontSizeSpin->setValue(v["fontSize"].toInt());
            m_textRgba = ImageConstHelpers::loadRgbaValue(v.toObject(),
                QStringLiteral("textRgba"), QStringLiteral("textColor"), m_textRgba);
            m_bgRgba = ImageConstHelpers::loadRgbaValue(v.toObject(),
                QStringLiteral("bgRgba"), QStringLiteral("bgColor"), m_bgRgba);
            ImageConstHelpers::applyRgbaToEditor(m_textColorEditorWidget, m_textRgba);
            ImageConstHelpers::applyRgbaToEditor(m_bgColorEditorWidget, m_bgRgba);
            widget->alignHCombo->setCurrentIndex(v["hAlign"].toInt());
            widget->alignVCombo->setCurrentIndex(v["vAlign"].toInt());
            widget->paddingSpin->setValue(v["padding"].toInt());
            widget->boldCheck->setChecked(v["bold"].toBool());
            widget->italicCheck->setChecked(v["italic"].toBool());
            widget->underlineCheck->setChecked(v["underline"].toBool());
            widget->wrapCheck->setChecked(v["wrap"].toBool());
            widget->antialiasCheck->setChecked(v["antialia"].toBool());
            m_paramsDirty = true;
            renderImage();
        }
    }

public Q_SLOTS:
    void toggleTextColorEditor() { showColorEditor(m_textColorEditorWidget, "文字颜色"); }
    void toggleBackgroundColorEditor() { showColorEditor(m_bgColorEditorWidget, "背景颜色"); }

private Q_SLOTS:
    void onInputChanged()
    {
        const int w = widget->widthEdit->value();
        const int h = widget->heightEdit->value();
        if (w > 0) {
            m_width = w;
        }
        if (h > 0) {
            m_height = h;
        }
        m_paramsDirty = true;
        renderImage();
    }

    void onTextColorChanged(const QColor& c)
    {
        m_textRgba = rgbaFromQColor(c);
        m_paramsDirty = true;
        renderImage();
    }

    void onBackgroundColorChanged(const QColor& c)
    {
        m_bgRgba = rgbaFromQColor(c);
        m_paramsDirty = true;
        renderImage();
    }

private:
    void showColorEditor(ColorEditorWidget* editor, const QString& title)
    {
        if (!editor) {
            return;
        }
        editor->setParent(nullptr);
        editor->setWindowTitle(title);
        editor->setWindowIcon(QIcon(":/icons/icons/curve.png"));
        editor->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
        editor->setAttribute(Qt::WA_DeleteOnClose, false);
        editor->setAttribute(Qt::WA_QuitOnClose, false);
        editor->resize(800, 400);
        editor->show();
        editor->activateWindow();
        editor->raise();
    }

    /** QPainter 渲染文本 — 不适合 GPU shader（依赖字体栅格化与排版） */
    void renderImage()
    {
        m_paramsDirty = true;
        if (m_width <= 0) {
            m_width = 1;
        }
        if (m_height <= 0) {
            m_height = 1;
        }

        const int width = m_width;
        const int height = m_height;
        const int padding = widget->paddingSpin->value();
        const QRect contentRect(
            padding,
            padding,
            std::max(0, width - padding * 2),
            std::max(0, height - padding * 2));

        QFont font = widget->fontCombo->currentFont();
        font.setPointSize(widget->fontSizeSpin->value());
        font.setBold(widget->boldCheck->isChecked());
        font.setItalic(widget->italicCheck->isChecked());
        font.setUnderline(widget->underlineCheck->isChecked());

        const bool wordWrap = widget->wrapCheck->isChecked();
        const bool antialias = widget->antialiasCheck->isChecked();

        QString alignCss = "left";
        m_hAlign = widget->alignHCombo->currentIndex();
        m_vAlign = widget->alignVCombo->currentIndex();
        if (m_hAlign == 1) {
            alignCss = "center";
        } else if (m_hAlign == 2) {
            alignCss = "right";
        }

        auto rgbaCss = [](const QColor& c) -> QString {
            return QString("rgba(%1,%2,%3,%4)")
                .arg(c.red())
                .arg(c.green())
                .arg(c.blue())
                .arg(c.alphaF(), 0, 'f', 3);
        };

        QString text = widget->textEdit->toPlainText();
        text.replace("\r\n", "\n");
        QString escaped = text.toHtmlEscaped();
        escaped.replace("\n", "<br/>");

        const QString html = QString(
                                 "<div style=\""
                                 "color:%1;"
                                 "font-family:'%2';"
                                 "font-size:%3px;"
                                 "font-weight:%4;"
                                 "font-style:%5;"
                                 "text-decoration:%6;"
                                 "text-align:%7;"
                                 "white-space:%8;"
                                 "\">%9</div>")
                                 .arg(rgbaCss(qcolorFromRgba(m_textRgba)))
                                 .arg(font.family().toHtmlEscaped())
                                 .arg(font.pointSize())
                                 .arg(font.bold() ? "bold" : "normal")
                                 .arg(font.italic() ? "italic" : "normal")
                                 .arg(font.underline() ? "underline" : "none")
                                 .arg(alignCss)
                                 .arg(wordWrap ? "pre-wrap" : "pre")
                                 .arg(escaped);

        QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
        image.fill(qcolorFromRgba(m_bgRgba));

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, antialias);
        painter.setRenderHint(QPainter::TextAntialiasing, antialias);

        QTextDocument doc;
        doc.setDocumentMargin(0);
        doc.setDefaultFont(font);
        doc.setHtml(html);
        if (wordWrap) {
            doc.setTextWidth(contentRect.width());
        } else {
            doc.setTextWidth(-1);
        }

        painter.save();
        painter.setClipRect(contentRect);
        qreal yOffset = 0.0;
        const qreal docHeight = doc.documentLayout()->documentSize().height();
        if (m_vAlign == 1) {
            yOffset = (contentRect.height() - docHeight) * 0.5;
        } else if (m_vAlign == 2) {
            yOffset = (contentRect.height() - docHeight);
        }
        if (yOffset < 0.0) {
            yOffset = 0.0;
        }
        painter.translate(QPointF(contentRect.left(), contentRect.top() + yOffset));
        doc.drawContents(&painter, QRectF(0, 0, contentRect.width(), contentRect.height()));
        painter.restore();

        widget->textColorButton->setStyleSheet(
            QString("background-color:%1;").arg(qcolorFromRgba(m_textRgba).name(QColor::HexArgb)));
        widget->bgColorButton->setStyleSheet(
            QString("background-color:%1;").arg(qcolorFromRgba(m_bgRgba).name(QColor::HexArgb)));

        const QSize previewSize = widget->display->size().isEmpty() ? QSize(260, 90) : widget->display->size();
        widget->display->setPixmap(
            QPixmap::fromImage(image.scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));

        QImage argb = image.convertToFormat(QImage::Format_ARGB32);
        cv::Mat mat(argb.height(), argb.width(), CV_8UC4, const_cast<uchar*>(argb.bits()), argb.bytesPerLine());
        m_renderedBgra = mat.clone();
    }

    void publishFrame()
    {
        if (m_shuttingDown.load() || !m_paramsDirty || m_renderedBgra.empty()) {
            return;
        }

        ImageConstHelpers::ensureOutputBuffer(m_outImageData, m_outBuffer);
        ImageConstHelpers::pushBgraFrame(m_outBuffer, m_renderedBgra.clone(), m_lastPushedTimestamp);
        m_paramsDirty = false;
    }

    TextToImageInterface* widget = new TextToImageInterface();
    std::shared_ptr<ImageData> m_outImageData;
    std::shared_ptr<ImageTimestampRingQueue> m_outBuffer;
    qint64 m_lastPushedTimestamp = -1;
    int m_width = 512;
    int m_height = 256;
    QVector<float> m_textRgba{1.0f, 1.0f, 1.0f, 1.0f};
    QVector<float> m_bgRgba{0.0f, 0.0f, 0.0f, 1.0f};
    int m_hAlign = 0;
    int m_vAlign = 0;
    cv::Mat m_renderedBgra;
    ColorEditorWidget* m_textColorEditorWidget = new ColorEditorWidget();
    ColorEditorWidget* m_bgColorEditorWidget = new ColorEditorWidget();
    bool m_paramsDirty = true;
    std::atomic<bool> m_shuttingDown{false};
};
} // namespace Nodes
