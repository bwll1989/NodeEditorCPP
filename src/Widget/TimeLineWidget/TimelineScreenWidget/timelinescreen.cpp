#include "timelinescreen.hpp"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QSlider>
#include <QComboBox>
#include <QGridLayout>
#include <QPainter>
#include <QResizeEvent>
#include <QJsonObject>

TimelineScreen::TimelineScreen(QWidget *parent)
    : AbstractTimelineScreen(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
}

TimelineScreen::~TimelineScreen()
{
    // 基类会处理 m_propertiesWidget 的删除
}

void TimelineScreen::registerType()
{
    qmlRegisterType<TimelineScreen>("TimelineWidget", 1, 0, "TimelineScreen");
}

void TimelineScreen::createPropertiesWidget()
{
    if (!m_propertiesWidget) {
        m_propertiesWidget = new QDialog();
        m_propertiesWidget->setWindowTitle(tr(this->m_name.toStdString().c_str()));
        m_propertiesWidget->setMinimumWidth(300);
        
        m_propertiesWidget->setWindowFlags(
            Qt::Dialog |
            Qt::WindowStaysOnTopHint |
            Qt::CustomizeWindowHint |
            Qt::WindowTitleHint |
            Qt::WindowCloseButtonHint
        );

        auto mainLayout = new QVBoxLayout(m_propertiesWidget);
        auto tabWidget = new QTabWidget();

        // 通用标签页
        auto generalTab = new QWidget();
        auto generalLayout = new QFormLayout(generalTab);

        // 名称
        auto nameEdit = new QLineEdit();
        nameEdit->setText(m_name);
        connect(nameEdit, &QLineEdit::textChanged, this, &TimelineScreen::setName);
        generalLayout->addRow(tr("名称:"), nameEdit);

        // Computer
        auto computerEdit = new QLineEdit(m_computerName);
        connect(computerEdit, &QLineEdit::textChanged, this, &TimelineScreen::setComputerName);
        generalLayout->addRow(tr("Computer:"), computerEdit);

        // 启用显示复选框
        auto enabledCheckBox = new QCheckBox(tr("Use this display"));
        enabledCheckBox->setChecked(m_enabled);
        connect(enabledCheckBox, &QCheckBox::toggled, this, &TimelineScreen::setEnabled);
        generalLayout->addRow("", enabledCheckBox);

        // 连接状态
        auto statusLayout = new QHBoxLayout();
        auto statusLabel = new QLabel(tr("连接:"));
        auto statusValue = new QLabel(tr("离线"));
        statusValue->setStyleSheet("QLabel { color: red; }");
        auto testButton = new QPushButton(tr("连接测试"));
        statusLayout->addWidget(statusLabel);
        statusLayout->addWidget(statusValue);
        statusLayout->addWidget(testButton);
        generalLayout->addRow("", statusLayout);      

        // 显示分辨率组
        auto resGroup = new QGroupBox(tr("显示分辨率:"));
        auto resLayout = new QFormLayout(resGroup);

        // 宽度和高度
        auto widthSpinBox = new QSpinBox();
        widthSpinBox->setRange(1, 9999);
        widthSpinBox->setValue(m_screenWidth);
        connect(widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                this, &TimelineScreen::setScreenWidth);

        auto heightSpinBox = new QSpinBox();
        heightSpinBox->setRange(1, 9999);
        heightSpinBox->setValue(m_screenHeight);
        connect(heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &TimelineScreen::setScreenHeight);

        resLayout->addRow(tr("宽度:"), widthSpinBox);
        resLayout->addRow(tr("高度:"), heightSpinBox);
        generalLayout->addRow(resGroup);

        // 舞台中的位置组
        auto posGroup = new QGroupBox(tr("显示在舞台的位置:"));
        auto posLayout = new QFormLayout(posGroup);

        // X和Y坐标
        auto xPosSpinBox = new QSpinBox();
        m_propertyWidgets.xPos = xPosSpinBox;
        xPosSpinBox->setRange(-9999, 9999);
        xPosSpinBox->setValue(m_posX);
        connect(xPosSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int value) { setPosX(value); });

        auto yPosSpinBox = new QSpinBox();
        m_propertyWidgets.yPos = yPosSpinBox;
        yPosSpinBox->setRange(-9999, 9999);
        yPosSpinBox->setValue(m_posY);
        connect(yPosSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int value) { setPosY(value); });

        auto rotationSpinBox = new QSpinBox();
        m_propertyWidgets.rotation = rotationSpinBox;
        rotationSpinBox->setRange(-180, 180);
        rotationSpinBox->setValue(m_rotation);
        connect(rotationSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, [this](int value) { setRotation(value); });

        posLayout->addRow(tr("X轴:"), xPosSpinBox);
        posLayout->addRow(tr("Y轴:"), yPosSpinBox);
        posLayout->addRow(tr("旋转:"), rotationSpinBox);
        generalLayout->addRow(posGroup);

        tabWidget->addTab(generalTab, tr("通用"));
        
        // 修改几何结构标签页
        auto geometryTab = new QWidget();
        auto geometryLayout = new QVBoxLayout(geometryTab);


        // 添加标签页
        tabWidget->addTab(geometryTab, tr("几何结构"));

        // 修改高级标签页
        auto advancedTab = new QWidget();
        auto advancedLayout = new QVBoxLayout(advancedTab);

        // 颜色标题
        auto colorTitle = new QLabel(tr("这个显示器通道的颜色:"));
        advancedLayout->addWidget(colorTitle);

        // 红色通道
        auto redLayout = new QHBoxLayout();
        auto redLabel = new QLabel(tr("红:"));
        redLabel->setMinimumWidth(30);
        auto redSlider = new QSlider(Qt::Horizontal);
        redSlider->setRange(0, 100);
        redSlider->setValue(m_redIntensity);
        auto redSpinBox = new QSpinBox();
        redSpinBox->setRange(0, 100);
        redSpinBox->setValue(m_redIntensity);
        redSpinBox->setSuffix("%");
        
        // 修复红色通道的双向绑定
        connect(redSlider, &QSlider::valueChanged, this, [this, redSpinBox](int value) {
            redSpinBox->setValue(value);
            setRedIntensity(value);
        });
        connect(redSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                this, [this, redSlider](int value) {
            redSlider->setValue(value);
            setRedIntensity(value);
        });

        redLayout->addWidget(redLabel);
        redLayout->addWidget(redSlider);
        redLayout->addWidget(redSpinBox);
        advancedLayout->addLayout(redLayout);

        // 绿色通道
        auto greenLayout = new QHBoxLayout();
        auto greenLabel = new QLabel(tr("绿:"));
        greenLabel->setMinimumWidth(30);
        auto greenSlider = new QSlider(Qt::Horizontal);
        greenSlider->setRange(0, 100);
        greenSlider->setValue(m_greenIntensity);
        auto greenSpinBox = new QSpinBox();
        greenSpinBox->setRange(0, 100);
        greenSpinBox->setValue(m_greenIntensity);
        greenSpinBox->setSuffix("%");
        
        // 修复绿色通道的双向绑定
        connect(greenSlider, &QSlider::valueChanged, this, [this, greenSpinBox](int value) {
            greenSpinBox->setValue(value);
            setGreenIntensity(value);
        });
        connect(greenSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                this, [this, greenSlider](int value) {
            greenSlider->setValue(value);
            setGreenIntensity(value);
        });

        greenLayout->addWidget(greenLabel);
        greenLayout->addWidget(greenSlider);
        greenLayout->addWidget(greenSpinBox);
        advancedLayout->addLayout(greenLayout);

        // 蓝色通道
        auto blueLayout = new QHBoxLayout();
        auto blueLabel = new QLabel(tr("蓝:"));
        blueLabel->setMinimumWidth(30);
        auto blueSlider = new QSlider(Qt::Horizontal);
        blueSlider->setRange(0, 100);
        blueSlider->setValue(m_blueIntensity);
        auto blueSpinBox = new QSpinBox();
        blueSpinBox->setRange(0, 100);
        blueSpinBox->setValue(m_blueIntensity);
        blueSpinBox->setSuffix("%");
        
        // 修复蓝色通道的双向绑定
        connect(blueSlider, &QSlider::valueChanged, this, [this, blueSpinBox](int value) {
            blueSpinBox->setValue(value);
            setBlueIntensity(value);
        });
        connect(blueSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
                this, [this, blueSlider](int value) {
            blueSlider->setValue(value);
            setBlueIntensity(value);
        });

        blueLayout->addWidget(blueLabel);
        blueLayout->addWidget(blueSlider);
        blueLayout->addWidget(blueSpinBox);
        advancedLayout->addLayout(blueLayout);

        // Gamma 设置
        auto gammaLayout = new QHBoxLayout();
        auto gammaLabel = new QLabel(tr("Gamma:"));
        gammaLabel->setMinimumWidth(30);
        
        auto gammaSlider = new QSlider(Qt::Horizontal);
        gammaSlider->setRange(100, 400);  // 1.00 - 4.00
        gammaSlider->setValue(qRound(m_gamma * 100));
        
        auto gammaSpinBox = new QDoubleSpinBox();
        gammaSpinBox->setRange(1.00, 4.00);
        gammaSpinBox->setValue(m_gamma);
        gammaSpinBox->setDecimals(2);
        gammaSpinBox->setSingleStep(0.05);
        
        // 修复 Gamma 的双向绑定
        connect(gammaSlider, &QSlider::valueChanged, this, [this, gammaSpinBox](int value) {
            double gamma = value / 100.0;
            if (!qFuzzyCompare(gammaSpinBox->value(), gamma)) {
                gammaSpinBox->setValue(gamma);
                setGamma(gamma);
            }
        });
        
        connect(gammaSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                this, [this, gammaSlider](double value) {
            int sliderValue = qRound(value * 100);
            if (gammaSlider->value() != sliderValue) {
                gammaSlider->setValue(sliderValue);
                setGamma(value);
            }
        });

        gammaLayout->addWidget(gammaLabel);
        gammaLayout->addWidget(gammaSlider);
        gammaLayout->addWidget(gammaSpinBox);
        advancedLayout->addLayout(gammaLayout);

        // 添加弹簧
        advancedLayout->addStretch();

        // 添加标签页到tabWidget
        tabWidget->addTab(advancedTab, tr("高级"));

        mainLayout->addWidget(tabWidget);

        // 按钮行
        auto buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
            Qt::Horizontal,
            m_propertiesWidget
        );
        
        connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
            m_propertiesWidget->accept();
        });
        
        connect(buttonBox, &QDialogButtonBox::rejected, this, [this]() {
            m_propertiesWidget->reject();
        });
        
        mainLayout->addWidget(buttonBox);

        // 连接属性变化信号到更新槽
        connect(this, &TimelineScreen::posXChanged, this, &TimelineScreen::updatePropertyWidgets);
        connect(this, &TimelineScreen::posYChanged, this, &TimelineScreen::updatePropertyWidgets);
        connect(this, &TimelineScreen::rotationChanged, this, &TimelineScreen::updatePropertyWidgets);
        connect(this, &TimelineScreen::redIntensityChanged, this, &TimelineScreen::updatePropertyWidgets);
        connect(this, &TimelineScreen::greenIntensityChanged, this, &TimelineScreen::updatePropertyWidgets);
        connect(this, &TimelineScreen::blueIntensityChanged, this, &TimelineScreen::updatePropertyWidgets);
        connect(this, &TimelineScreen::gammaChanged, this, &TimelineScreen::updatePropertyWidgets);
    }
}

void TimelineScreen::updatePropertyWidgets()
{
    if (!m_propertiesWidget || !m_propertiesWidget->isVisible()) {
        return;
    }

    // 更新位置控件
    if (m_propertyWidgets.xPos) {
        m_propertyWidgets.xPos->setValue(m_posX);
    }
    if (m_propertyWidgets.yPos) {
        m_propertyWidgets.yPos->setValue(m_posY);
    }
    if (m_propertyWidgets.rotation) {
        m_propertyWidgets.rotation->setValue(m_rotation);
    }

    // 更新颜色控件
    if (m_propertyWidgets.redIntensity && m_propertyWidgets.redSlider) {
        m_propertyWidgets.redIntensity->setValue(m_redIntensity);
        m_propertyWidgets.redSlider->setValue(m_redIntensity);
    }
    if (m_propertyWidgets.greenIntensity && m_propertyWidgets.greenSlider) {
        m_propertyWidgets.greenIntensity->setValue(m_greenIntensity);
        m_propertyWidgets.greenSlider->setValue(m_greenIntensity);
    }
    if (m_propertyWidgets.blueIntensity && m_propertyWidgets.blueSlider) {
        m_propertyWidgets.blueIntensity->setValue(m_blueIntensity);
        m_propertyWidgets.blueSlider->setValue(m_blueIntensity);
    }
    if (m_propertyWidgets.gamma && m_propertyWidgets.gammaSlider) {
        m_propertyWidgets.gamma->setValue(m_gamma);
        m_propertyWidgets.gammaSlider->setValue(qRound(m_gamma * 100));
    }
}

void TimelineScreen::setImage(const QImage& image)
{
    m_currentImage = image;
    m_displayRect = calculateDisplayRect();
    update();
}

void TimelineScreen::clear()
{
    m_currentImage = QImage();
    update();
}

void TimelineScreen::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_displayRect = calculateDisplayRect();
}

void TimelineScreen::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (!m_currentImage.isNull()) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(m_displayRect, m_currentImage);
    }
}

QRect TimelineScreen::getDisplayRect() const
{
    return m_displayRect;
}

QRect TimelineScreen::calculateDisplayRect() const
{
    if (m_currentImage.isNull()) {
        return rect();
    }
    return QRect(QPoint(0,0), m_currentImage.size());
}

QJsonObject TimelineScreen::save() const
{
    return AbstractTimelineScreen::save();
}

void TimelineScreen::load(const QJsonObject &json)
{
    AbstractTimelineScreen::load(json);
    // 更新属性和界面
    onPropertyChanged();
}

void TimelineScreen::onPropertyChanged()
{
    // 更新属性窗口
    updatePropertyWidgets();
    // 发送属性改变信号
    emit propertyChanged();
    // 触发重绘
    update();
}
