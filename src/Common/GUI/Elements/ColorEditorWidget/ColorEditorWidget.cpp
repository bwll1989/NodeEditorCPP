#include "ColorEditorWidget.hpp"
#include <QDialogButtonBox>
#include <QScreen>
#include <QApplication>
#include <QWindow>
/* 函数级注释：构造，初始化 UI 与信号 */
// 类 ColorEditorWidget 构造函数
ColorEditorWidget::ColorEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    /* 函数级注释：不调用 Q_INIT_RESOURCE(color_widgets)
     * 原因：当前工程未编译 color_widgets.qrc，调用会导致 LNK2019。
     * 预览背景采用纯色画刷，不再依赖资源。 */

    // 函数级注释：构建 UI、连接信号，然后设置初始颜色
    buildUI();
    connectSignals();
    setColorInternal(QColor(64, 172, 143, 128));
}

/* 函数级注释：析构 */
ColorEditorWidget::~ColorEditorWidget() = default;

/* 函数级注释：获取当前颜色，若禁用 alpha 则返回不透明 */
QColor ColorEditorWidget::color() const
{
    QColor col = m_color;
    if (!m_alphaEnabled)
        col.setAlpha(255);
    return col;
}

/* 函数级注释：外部设置颜色，更新内部并发出 colorChanged */
void ColorEditorWidget::setColor(const QColor& c)
{
    editHex->setModified(false);
    setColorInternal(c);
}

/* 函数级注释：设置是否启用 alpha 编辑（显示/隐藏 alpha 行） */
void ColorEditorWidget::setAlphaEnabled(bool enabled)
{
    if (enabled == m_alphaEnabled) return;
    m_alphaEnabled = enabled;

    editHex->setShowAlpha(enabled);
    slideAlpha->setVisible(enabled);
    spinAlpha->setVisible(enabled);

    emit alphaEnabledChanged(enabled);

    // 若关闭 alpha，保证预览/十六进制显示一致
    QColor col = m_color;
    if (!enabled) col.setAlpha(255);
    setColorInternal(col);
}

/* 函数级注释：返回是否启用 alpha */
bool ColorEditorWidget::alphaEnabled() const
{
    return m_alphaEnabled;
}

/* 函数级注释：设置色轮选择器形状（转发到 ColorWheel） */
void ColorEditorWidget::setWheelShape(color_widgets::ColorWheel::ShapeEnum shape)
{
    wheel->setSelectorShape(shape);
}

/* 函数级注释：获取色轮选择器形状 */
color_widgets::ColorWheel::ShapeEnum ColorEditorWidget::wheelShape() const
{
    return wheel->selectorShape();
}

/* 函数级注释：设置颜色空间（转发到 ColorWheel） */
void ColorEditorWidget::setColorSpace(color_widgets::ColorWheel::ColorSpaceEnum space)
{
    wheel->setColorSpace(space);
}

/* 函数级注释：获取颜色空间 */
color_widgets::ColorWheel::ColorSpaceEnum ColorEditorWidget::colorSpace() const
{
    return wheel->colorSpace();
}

/* 函数级注释：设置是否旋转内部选择器 */
void ColorEditorWidget::setWheelRotating(bool rotating)
{
    wheel->setRotatingSelector(rotating);
}

/* 函数级注释：获取是否旋转内部选择器 */
bool ColorEditorWidget::wheelRotating() const
{
    return wheel->rotatingSelector();
}

/* 函数级注释：设置是否镜像内部选择器 */
void ColorEditorWidget::setWheelMirrored(bool mirrored)
{
    wheel->setMirroredSelector(mirrored);
}

/* 函数级注释：获取是否镜像内部选择器 */
bool ColorEditorWidget::wheelMirrored() const
{
    return wheel->mirroredSelector();
}

/* 函数级注释：HSV 三要素变化 -> 计算新颜色并更新 */
void ColorEditorWidget::set_hsv()
{
    /* 函数级注释：HSV 变化统一走内部更新，内部会更新色轮与其他控件 */
    if (signalsBlocked()) return;
    QColor col = QColor::fromHsv(
        slideHue->value(),
        slideSaturation->value(),
        slideValue->value(),
        slideAlpha->value()
    );
    setColorInternal(col, /*updateWheel*/true);
}

void ColorEditorWidget::set_alpha()
{
    /* 函数级注释：Alpha 变化统一走内部更新 */
    if (signalsBlocked()) return;
    QColor col = m_color;
    col.setAlpha(slideAlpha->value());
    setColorInternal(col, /*updateWheel*/true);
}

void ColorEditorWidget::set_rgb()
{
    /* 函数级注释：RGB 变化统一走内部更新；饱和度为 0 时保持当前 hue */
    if (signalsBlocked()) return;
    QColor col(
        slideRed->value(),
        slideGreen->value(),
        slideBlue->value(),
        slideAlpha->value()
    );
    if (col.saturation() == 0)
        col = QColor::fromHsv(slideHue->value(), 0, col.value(), slideAlpha->value());

    setColorInternal(col, /*updateWheel*/true);
}

/* 函数级注释：Hex 输入框颜色即时变化 */
void ColorEditorWidget::on_edit_hex_colorChanged(const QColor& c)
{
    setColorInternal(c);
}

/* 函数级注释：Hex 输入完成 */
void ColorEditorWidget::on_edit_hex_colorEditingFinished(const QColor& c)
{
    editHex->setModified(false);
    setColorInternal(c);
    emit colorSelected(color());
}

/* 函数级注释：点击“拾取屏幕颜色”开始拾取 */
void ColorEditorWidget::on_pick_color()
{
    m_picking = true;
    grabMouse(Qt::CrossCursor);
}

/* 函数级注释：拾色模式下鼠标释放 -> 结束并更新颜色 */
void ColorEditorWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_picking)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        setColorInternal(color_widgets::utils::get_screen_color(event->globalPosition().toPoint()));
#else
        setColorInternal(color_widgets::utils::get_screen_color(event->globalPos()));
#endif
        m_picking = false;
        releaseMouse();
        emit colorSelected(color());
    }
    QWidget::mouseReleaseEvent(event);
}

/* 函数级注释：拾色模式下鼠标移动 -> 实时取色 */
void ColorEditorWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_picking)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        setColorInternal(color_widgets::utils::get_screen_color(event->globalPosition().toPoint()));
#else
        setColorInternal(color_widgets::utils::get_screen_color(event->globalPos()));
#endif
    }
    QWidget::mouseMoveEvent(event);
}

/* 函数级注释：内部统一更新颜色（阻塞信号，保持控件一致） */
void ColorEditorWidget::setColorInternal(const QColor& col, bool updateWheel)
{
    // 先记录颜色
    m_color = col;

    // 立即阻塞本控件与所有子控件的信号，避免回环
    bool blocked = signalsBlocked();
    blockSignals(true);
    for (QObject* obj : this->children())
        if (auto* w = qobject_cast<QWidget*>(obj))
            w->blockSignals(true);

    // 根据需要更新色轮（来自滑条/Hex/外部时更新；来自色轮时不更新）
    if (updateWheel && wheel)
        wheel->setColor(col);

    // 计算用于 HSV/渐变的参数（保持饱和度为0时沿用现有 hue）
    const double hueF = (col.saturation() == 0 && wheel) ? wheel->hue() : col.hsvHueF();
    const double satF = col.saturationF();
    const double valF = col.valueF();

    // RGB 滑块与渐变
    slideRed->setValue(col.red());
    spinRed->setValue(slideRed->value());
    slideRed->setFirstColor(QColor(0, col.green(), col.blue(), col.alpha()));
    slideRed->setLastColor(QColor(255, col.green(), col.blue(), col.alpha()));

    slideGreen->setValue(col.green());
    spinGreen->setValue(slideGreen->value());
    slideGreen->setFirstColor(QColor(col.red(), 0, col.blue(), col.alpha()));
    slideGreen->setLastColor(QColor(col.red(), 255, col.blue(), col.alpha()));

    slideBlue->setValue(col.blue());
    spinBlue->setValue(slideBlue->value());
    slideBlue->setFirstColor(QColor(col.red(), col.green(), 0, col.alpha()));
    slideBlue->setLastColor(QColor(col.red(), col.green(), 255, col.alpha()));

    // Hue / Saturation / Value（HueSlider 需要当前 sat/value 绘制轨迹）
    slideHue->setValue(qRound(hueF * 360.0));
    slideHue->setColorSaturation(satF);
    slideHue->setColorValue(valF);
    spinHue->setValue(slideHue->value());

    slideSaturation->setValue(qRound(satF * 255.0));
    spinSaturation->setValue(slideSaturation->value());
    slideSaturation->setFirstColor(QColor::fromHsvF(hueF, 0, valF, col.alphaF()));
    slideSaturation->setLastColor(QColor::fromHsvF(hueF, 1, valF, col.alphaF()));

    slideValue->setValue(qRound(valF * 255.0));
    spinValue->setValue(slideValue->value());
    slideValue->setFirstColor(QColor::fromHsvF(hueF, satF, 0, col.alphaF()));
    slideValue->setLastColor(QColor::fromHsvF(hueF, satF, 1, col.alphaF()));

    // Alpha
    slideAlpha->setValue(col.alpha());
    spinAlpha->setValue(slideAlpha->value());
    QColor alphaColor = col; alphaColor.setAlpha(0);
    slideAlpha->setFirstColor(alphaColor);
    alphaColor.setAlpha(255);
    slideAlpha->setLastColor(alphaColor);

    // 预览与十六进制
    if (!editHex->isModified())
        editHex->setColor(col);
    preview->setColor(col);

    // 恢复信号阻塞状态
    blockSignals(blocked);
    for (QObject* obj : this->children())
        if (auto* w = qobject_cast<QWidget*>(obj))
            w->blockSignals(false);

    emit colorChanged(col);
}

/* 函数级注释：搭建 UI（参考 ColorDialog 的布局风格） */
void ColorEditorWidget::buildUI()
{
    auto* main = new QHBoxLayout(this);
    main->setContentsMargins(6, 6, 6, 6);
    main->setSpacing(8);

    // 左侧：色轮 + 预览 + Hex + 拾色按钮
    auto* left = new QVBoxLayout();
    left->setSpacing(6);

    /* 函数级注释：预览背景使用纯色，避免资源缺失 */
    // 函数级注释：实例化预览/色轮/十六进制输入/拾色按钮，并设置基础属性与背景


    
    wheel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    wheel->setMinimumSize(QSize(180, 180));
    wheel->setColor(QColor(255, 0, 0));
    wheel->setWheelWidth(30);

    preview->setDisplayMode(color_widgets::ColorPreview::SplitAlpha);
    preview->setDrawFrame(true);
    preview->setBackground(QBrush(QColor(60, 60, 60))); // 避免资源缺失时的空背景
    preview->setMinimumHeight(28);
    preview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

   
    pickButton->setToolTip(tr("拾取屏幕颜色"));

    // 左列顺序：色轮 -> 预览 -> 按钮（Hex 不在左列）
    left->addWidget(wheel, /*stretch*/1);
    left->addWidget(preview);
    left->addWidget(pickButton);

    // 右侧：HSV / RGB / Alpha
    auto* right = new QGridLayout();
    right->setHorizontalSpacing(6);
    right->setVerticalSpacing(4);

    // 函数级注释：辅助函数用于构造行（标签右对齐 + 滑条 + 数值）
    auto mkRow = [&](int row, const QString& text, color_widgets::GradientSlider* slider, QSpinBox* spin, int max) {
        auto* label = new QLabel(text, this);
        label->setMinimumWidth(72);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        slider->setOrientation(Qt::Horizontal);
        slider->setRange(0, max);
        spin->setRange(0, max);
        spin->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        right->addWidget(label, row, 0);
        right->addWidget(slider, row, 1);
        right->addWidget(spin,   row, 2);
    };

    {
        auto* label = new QLabel(tr("Hue"), this);
        label->setMinimumWidth(72);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        slideHue->setOrientation(Qt::Horizontal);
        slideHue->setRange(0, 360);
        spinHue->setRange(0, 360);
        right->addWidget(label, 0, 0);
        right->addWidget(slideHue, 0, 1);
        right->addWidget(spinHue, 0, 2);
    }



    mkRow(1, tr("Saturation"), slideSaturation, spinSaturation, 255);

    mkRow(2, tr("Value"), slideValue, spinValue, 255);

    // 分隔线
    QFrame* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    right->addWidget(sep, 3, 0, 1, 3);


    mkRow(4, tr("Red"), slideRed, spinRed, 255);

    mkRow(5, tr("Green"), slideGreen, spinGreen, 255);

    mkRow(6, tr("Blue"), slideBlue, spinBlue, 255);

    mkRow(7, tr("Alpha"), slideAlpha, spinAlpha, 255);

    editHex->setPreviewColor(true);
    editHex->setShowAlpha(true);
    editHex->setMinimumHeight(28);
    editHex->setPlaceholderText("#RRGGBBAA");
    // 函数级注释：Hex 行置于右列最底部，三列布局（标签/空列/输入框），与其他行对齐
    {
        auto* hexLabel = new QLabel(tr("Hex"), this);
        hexLabel->setMinimumWidth(72);
        hexLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        right->addWidget(hexLabel, 8, 0);

        // // 空列占位，保持三列对齐
        // right->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 8, 1);

        right->addWidget(editHex, 8, 1,1,2);
    }

    // 合并布局
    main->addLayout(left, 0);
    main->addLayout(right, 1);
    setLayout(main);
}

/* 函数级注释：连接信号槽（尽量与 ColorDialog 的行为一致） */
void ColorEditorWidget::connectSignals()
{
    // 函数级注释：连接各控件的信号；若任一控件未实例化则安全返回并报警，避免空指针访问导致崩溃
    if (!preview || !wheel || !editHex || !pickButton ||
        !slideHue || !spinHue ||
        !slideSaturation || !spinSaturation ||
        !slideValue || !spinValue ||
        !slideRed || !spinRed ||
        !slideGreen || !spinGreen ||
        !slideBlue || !spinBlue ||
        !slideAlpha || !spinAlpha)
    {
        qWarning() << "[ColorEditorWidget] connectSignals: null widget detected. "
                   << "Ensure buildUI() instantiated all controls before connecting.";
        return;
    }

    // 预览点击 -> 视为“选定”
    connect(preview, &color_widgets::ColorPreview::clicked, [this] {
        emit colorSelected(color());
    });


    connect(wheel, &color_widgets::ColorWheel::colorChanged,
            this, [this](const QColor& c){
                /* 函数级注释：来自色轮的颜色变化，避免回环更新色轮自身 */
                setColorInternal(c, /*updateWheel*/false);
            });

    connect(wheel, &color_widgets::ColorWheel::colorSpaceChanged,
            this, &ColorEditorWidget::colorSpaceChanged);
    connect(wheel, &color_widgets::ColorWheel::selectorShapeChanged,
            this, &ColorEditorWidget::wheelShapeChanged);
    connect(wheel, &color_widgets::ColorWheel::rotatingSelectorChanged,
            this, &ColorEditorWidget::wheelRotatingChanged);
    connect(wheel, &color_widgets::ColorWheel::mirroredSelectorChanged,
            this, &ColorEditorWidget::wheelMirroredChanged);

    connect(editHex, &color_widgets::ColorLineEdit::colorChanged,
            this, &ColorEditorWidget::on_edit_hex_colorChanged);
    connect(editHex, &color_widgets::ColorLineEdit::colorEditingFinished,
            this, &ColorEditorWidget::on_edit_hex_colorEditingFinished);

    connect(pickButton, &QPushButton::clicked,
            this, &ColorEditorWidget::on_pick_color);

    connect(slideHue,  &QSlider::valueChanged, [this](int){ spinHue->setValue(slideHue->value()); set_hsv(); });
    connect(spinHue,   QOverload<int>::of(&QSpinBox::valueChanged), [this](int v){ slideHue->setValue(v); set_hsv(); });

    connect(slideSaturation, &QSlider::valueChanged, [this](int){ spinSaturation->setValue(slideSaturation->value()); set_hsv(); });
    connect(spinSaturation,  QOverload<int>::of(&QSpinBox::valueChanged), [this](int v){ slideSaturation->setValue(v); set_hsv(); });

    connect(slideValue, &QSlider::valueChanged, [this](int){ spinValue->setValue(slideValue->value()); set_hsv(); });
    connect(spinValue,  QOverload<int>::of(&QSpinBox::valueChanged), [this](int v){ slideValue->setValue(v); set_hsv(); });

    connect(slideRed,   &QSlider::valueChanged, [this](int){ spinRed->setValue(slideRed->value()); set_rgb(); });
    connect(spinRed,    QOverload<int>::of(&QSpinBox::valueChanged), [this](int v){ slideRed->setValue(v); set_rgb(); });

    connect(slideGreen, &QSlider::valueChanged, [this](int){ spinGreen->setValue(slideGreen->value()); set_rgb(); });
    connect(spinGreen,  QOverload<int>::of(&QSpinBox::valueChanged), [this](int v){ slideGreen->setValue(v); set_rgb(); });

    connect(slideBlue,  &QSlider::valueChanged, [this](int){ spinBlue->setValue(slideBlue->value()); set_rgb(); });
    connect(spinBlue,   QOverload<int>::of(&QSpinBox::valueChanged), [this](int v){ slideBlue->setValue(v); set_rgb(); });

    connect(slideAlpha, &QSlider::valueChanged, [this](int){ spinAlpha->setValue(slideAlpha->value()); set_alpha(); });
    connect(spinAlpha,  QOverload<int>::of(&QSpinBox::valueChanged), [this](int v){ slideAlpha->setValue(v); set_alpha(); });
}

QColor ColorEditorWidget::getColor() const
{
    return color();
}

