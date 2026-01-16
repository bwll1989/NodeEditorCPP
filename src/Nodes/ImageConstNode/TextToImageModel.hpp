#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QIcon>
#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>

#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"
#include "TextToImageInterface.hpp"
#include "DataTypes/NodeDataList.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "ConstantDefines.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using namespace NodeDataTypes;
using namespace Nodes;
namespace Nodes
{
    class TextToImageModel final : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数：初始化端口和界面，并连接参数变化信号
         */
        TextToImageModel(){
            InPortCount = 9;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Text To Image";
            WidgetEmbeddable = false;
            Resizable = false;

            connect(widget->widthEdit, &QLineEdit::textChanged, this, &TextToImageModel::onInputChanged);
            connect(widget->heightEdit, &QLineEdit::textChanged, this, &TextToImageModel::onInputChanged);
            connect(widget->textEdit, &QTextEdit::textChanged, this, &TextToImageModel::onInputChanged);
            connect(widget->fontCombo, &QFontComboBox::currentFontChanged, this, &TextToImageModel::onInputChanged);
            connect(widget->fontSizeSpin, qOverload<int>(&QSpinBox::valueChanged), this, &TextToImageModel::onInputChanged);
            connect(widget->boldCheck, &QCheckBox::toggled, this, &TextToImageModel::onInputChanged);
            connect(widget->italicCheck, &QCheckBox::toggled, this, &TextToImageModel::onInputChanged);
            connect(widget->underlineCheck, &QCheckBox::toggled, this, &TextToImageModel::onInputChanged);
            connect(widget->alignHCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TextToImageModel::onInputChanged);
            connect(widget->alignVCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &TextToImageModel::onInputChanged);
            connect(widget->paddingSpin, qOverload<int>(&QSpinBox::valueChanged), this, &TextToImageModel::onInputChanged);
            connect(widget->wrapCheck, &QCheckBox::toggled, this, &TextToImageModel::onInputChanged);

            connect(widget->textColorButton, &QPushButton::clicked, this, &TextToImageModel::toggleTextColorEditor);
            connect(widget->bgColorButton, &QPushButton::clicked, this, &TextToImageModel::toggleBackgroundColorEditor);

            m_textColorEditorWidget->setAlphaEnabled(true);
            m_bgColorEditorWidget->setAlphaEnabled(true);
            m_textColorEditorWidget->setColor(m_textColor);
            m_bgColorEditorWidget->setColor(m_bgColor);
            connect(m_textColorEditorWidget, &ColorEditorWidget::colorChanged, this, &TextToImageModel::onTextColorChanged);
            connect(m_bgColorEditorWidget, &ColorEditorWidget::colorChanged, this, &TextToImageModel::onBackgroundColorChanged);

            updateImage();
        };

        /**
         * @brief 析构函数：释放资源
         */
        ~TextToImageModel() override{
            if (m_textColorEditorWidget) {
                m_textColorEditorWidget->setParent(nullptr);
                m_textColorEditorWidget->deleteLater();
            }
            if (m_bgColorEditorWidget) {
                m_bgColorEditorWidget->setParent(nullptr);
                m_bgColorEditorWidget->deleteLater();
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
                    case 0: return "WIDTH";
                    case 1: return "HEIGHT";
                    case 2: return "TEXT";
                    case 3: return "FONT";
                    case 4: return "FONTSIZE";
                    case 5: return "TEXT_COLOR";
                    case 6: return "BG_COLOR";
                    case 7: return "HALIGN";
                    case 8: return "VALIGN";
                    default: return "";
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
            case 0:
                m_width = val.toInt();
                widget->widthEdit->setText(QString::number(m_width));
                break;
            case 1:
                m_height = val.toInt();
                widget->heightEdit->setText(QString::number(m_height));
                break;
            case 2:
                widget->textEdit->setPlainText(val.toString());
                break;
            case 3:
                widget->fontCombo->setCurrentText(val.toString());
                break;
            case 4:
                widget->fontSizeSpin->setValue(val.toInt());
                break;
            case 5:
                m_textColor = QColor(val.toString());
                m_textColorEditorWidget->setColor(m_textColor);
                break;
            case 6:
                m_bgColor = QColor(val.toString());
                m_bgColorEditorWidget->setColor(m_bgColor);
                break;
            case 7:
                m_hAlign = val.toInt();
                if (m_hAlign < 0) m_hAlign = 0;
                if (m_hAlign > 2) m_hAlign = 2;
                widget->alignHCombo->setCurrentIndex(m_hAlign);
                break;
            case 8:
                m_vAlign = val.toInt();
                if (m_vAlign < 0) m_vAlign = 0;
                if (m_vAlign > 2) m_vAlign = 2;
                widget->alignVCombo->setCurrentIndex(m_vAlign);
                break;
            default:
                break;
            }

            updateImage();
            Q_EMIT dataUpdated(0);
        }

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override{
            QJsonObject modelJson1;
            modelJson1["width"] = widget->widthEdit->text();
            modelJson1["height"] = widget->heightEdit->text();
            modelJson1["text"] = widget->textEdit->toPlainText();
            modelJson1["font"] = widget->fontCombo->currentFont().family();
            modelJson1["fontSize"] = widget->fontSizeSpin->value();
            modelJson1["textColor"] = m_textColor.name(QColor::HexArgb);
            modelJson1["bgColor"] = m_bgColor.name(QColor::HexArgb);
            modelJson1["hAlign"] = m_hAlign;
            modelJson1["vAlign"] = m_vAlign;
            modelJson1["padding"] = widget->paddingSpin->value();
            modelJson1["bold"] = widget->boldCheck->isChecked();
            modelJson1["italic"] = widget->italicCheck->isChecked();
            modelJson1["underline"] = widget->underlineCheck->isChecked();
            modelJson1["wrap"] = widget->wrapCheck->isChecked();
            modelJson1["antialia"]=widget->antialiasCheck->isChecked();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override{
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->widthEdit->setText(v["width"].toString());
                widget->heightEdit->setText(v["height"].toString());
                widget->textEdit->setPlainText(v["text"].toString());
                widget->fontCombo->setCurrentText(v["font"].toString());
                widget->fontSizeSpin->setValue(v["fontSize"].toInt());
                m_textColor = QColor(v["textColor"].toString());
                m_bgColor = QColor(v["bgColor"].toString());
                widget->alignHCombo->setCurrentIndex( v["hAlign"].toInt());
                widget->alignVCombo->setCurrentIndex( v["vAlign"].toInt());
                widget->paddingSpin->setValue(v["padding"].toInt());
                widget->boldCheck->setChecked(v["bold"].toBool());
                widget->italicCheck->setChecked(v["italic"].toBool());
                widget->underlineCheck->setChecked(v["underline"].toBool());
                widget->wrapCheck->setChecked(v["wrap"].toBool());
                widget->antialiasCheck->setChecked(v["antialia"].toBool());
                updateImage();
            }
        }

    public Q_SLOTS:
        /**
         * @brief 打开文字颜色选择器
         */
        void toggleTextColorEditor(){
            showColorEditor(m_textColorEditorWidget, "文字颜色");
        }

        /**
         * @brief 打开背景颜色选择器
         */
        void toggleBackgroundColorEditor(){
            showColorEditor(m_bgColorEditorWidget, "背景颜色");
        }

    private Q_SLOTS:
        /**
         * @brief 根据当前参数生成文本图像
         */
        void updateImage(){
            if (m_width <= 0) m_width = 1;
            if (m_height <= 0) m_height = 1;

            const int width = m_width;
            const int height = m_height;

            const int padding = widget->paddingSpin->value();
            const QRect contentRect(
                padding,
                padding,
                std::max(0, width - padding * 2),
                std::max(0, height - padding * 2)
            );

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
            if (m_hAlign == 1) alignCss = "center";
            else if (m_hAlign == 2) alignCss = "right";

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
                "\">%9</div>"
            )
            .arg(rgbaCss(m_textColor))
            .arg(font.family().toHtmlEscaped())
            .arg(font.pointSize())
            .arg(font.bold() ? "bold" : "normal")
            .arg(font.italic() ? "italic" : "normal")
            .arg(font.underline() ? "underline" : "none")
            .arg(alignCss)
            .arg(wordWrap ? "pre-wrap" : "pre")
            .arg(escaped);

            QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
            image.fill(m_bgColor);

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
            if (yOffset < 0.0) yOffset = 0.0;
            painter.translate(QPointF(contentRect.left(), contentRect.top() + yOffset));
            doc.drawContents(&painter, QRectF(0, 0, contentRect.width(), contentRect.height()));
            painter.restore();

            widget->textColorButton->setStyleSheet(QString("background-color:%1;").arg(m_textColor.name(QColor::HexArgb)));
            widget->bgColorButton->setStyleSheet(QString("background-color:%1;").arg(m_bgColor.name(QColor::HexArgb)));

            const QSize previewSize = widget->display->size().isEmpty() ? QSize(260, 90) : widget->display->size();
            widget->display->setPixmap(QPixmap::fromImage(image.scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)));

            QImage argb = image.convertToFormat(QImage::Format_ARGB32);
            cv::Mat mat(argb.height(), argb.width(), CV_8UC4, const_cast<uchar*>(argb.bits()), argb.bytesPerLine());
            m_image = mat.clone();
        }

        /**
         * @brief 界面参数变更时触发重新渲染
         */
        void onInputChanged(){
            bool okW = false;
            bool okH = false;
            const int w = widget->widthEdit->text().toInt(&okW);
            const int h = widget->heightEdit->text().toInt(&okH);
            if (okW && w > 0) m_width = w;
            if (okH && h > 0) m_height = h;
            updateImage();
            Q_EMIT dataUpdated(0);
        }

        /**
         * @brief 文字颜色变化时同步渲染
         * @param c 当前颜色
         */
        void onTextColorChanged(const QColor& c){
            m_textColor = c;
            updateImage();
            Q_EMIT dataUpdated(0);
        }

        /**
         * @brief 背景颜色变化时同步渲染
         * @param c 当前颜色
         */
        void onBackgroundColorChanged(const QColor& c){
            m_bgColor = c;
            updateImage();
            Q_EMIT dataUpdated(0);
        }

    private:
        /**
         * @brief 打开颜色编辑器窗口（独立窗口 + 置顶）
         * @param editor 颜色编辑器实例
         * @param title 窗口标题
         */
        void showColorEditor(ColorEditorWidget* editor, const QString& title){
            if (!editor) return;

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

        TextToImageInterface *widget = new TextToImageInterface();
        std::weak_ptr<ImageData> m_outData;
        int m_width = 512;
        int m_height = 256;
        QColor m_textColor = Qt::white;
        QColor m_bgColor = Qt::black;
        int m_hAlign = 0;
        int m_vAlign = 0;
        cv::Mat m_image=cv::Mat(256, 512, CV_8UC4, cv::Scalar(0, 0, 0, 255));
        ColorEditorWidget *m_textColorEditorWidget = new ColorEditorWidget();
        ColorEditorWidget *m_bgColorEditorWidget = new ColorEditorWidget();
    };
}
