#ifndef ABSTRACTTIMELINESCREEN_HPP
#define ABSTRACTTIMELINESCREEN_HPP

#include <QWidget>
#include <QImage>
#include <QJsonObject>
#include <QJsonValue>
#include <QSpinBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QDialog>
class AbstractTimelineScreen : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int screenWidth READ screenWidth WRITE setScreenWidth NOTIFY screenWidthChanged)
    Q_PROPERTY(int screenHeight READ screenHeight WRITE setScreenHeight NOTIFY screenHeightChanged)
    Q_PROPERTY(bool isSelected READ isSelected WRITE setIsSelected NOTIFY isSelectedChanged)
    Q_PROPERTY(qreal posX READ posX WRITE setPosX NOTIFY posXChanged)
    Q_PROPERTY(qreal posY READ posY WRITE setPosY NOTIFY posYChanged)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation NOTIFY rotationChanged)
    Q_PROPERTY(QString resolution READ resolution NOTIFY resolutionChanged)
    Q_PROPERTY(QString computerName READ computerName WRITE setComputerName NOTIFY computerNameChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int redIntensity READ redIntensity WRITE setRedIntensity NOTIFY redIntensityChanged)
    Q_PROPERTY(int greenIntensity READ greenIntensity WRITE setGreenIntensity NOTIFY greenIntensityChanged)
    Q_PROPERTY(int blueIntensity READ blueIntensity WRITE setBlueIntensity NOTIFY blueIntensityChanged)
    Q_PROPERTY(double gamma READ gamma WRITE setGamma NOTIFY gammaChanged)
public:
    explicit AbstractTimelineScreen(QWidget *parent = nullptr) : QWidget(parent)
    , m_screenWidth(1920)
    , m_screenHeight(1080)
    , m_isSelected(false)
    , m_posX(0.0)
    , m_posY(0.0)
    , m_rotation(0.0)
    , m_computerName("127.0.0.1")
    , m_enabled(true){}
    virtual ~AbstractTimelineScreen() = default;
    /**
     * 显示属性窗口
     */
    Q_INVOKABLE void showPropertiesDialog() {
        showProperties();
    }
    /**
     * 隐藏属性窗口
     */
    Q_INVOKABLE void hidePropertiesDialog() {
        hideProperties();
    }
    /**
     * 设置图像
     * @param QImage 图像
     */
    virtual void setImage(const QImage& image){};
    /** 
     * 获取属性 
     * @return QString 名称
     */     
    QString name() const { return m_name; }
    /** 
     * 获取屏幕宽度 
     * @return int 屏幕宽度
        */
    int screenWidth() const { return m_screenWidth; }
    /** 
     * 获取屏幕高度 
     * @return int 屏幕高度
     */
    int screenHeight() const { return m_screenHeight; }
    /** 
     * 获取是否选中 
     * @return bool 是否选中
     */
    bool isSelected() const { return m_isSelected; }
    /** 
     * 获取位置X 
     * @return qreal 位置X
     */
    qreal posX() const { return m_posX; }
    /** 
     * 获取位置Y 
     * @return qreal 位置Y
     */     
    qreal posY() const { return m_posY; }
    /** 
     * 获取旋转 
     * @return qreal 旋转
     */
    qreal rotation() const { return m_rotation; }   
    /** 
     * 获取分辨率 
     * @return QString 分辨率
     */
    QString resolution() const {
        return QString("%1x%2").arg(m_screenWidth).arg(m_screenHeight);
    }
    /** 
     * 获取计算机名称 
     * @return QString 计算机名称
     */
    QString computerName() const { return m_computerName; }
    /** 
     * 获取是否启用 
     * @return bool 是否启用
     */
    bool enabled() const { return m_enabled; }
    /** 
     * 获取红色亮度 
     * @return int 红色亮度
     */
    int redIntensity() const { return m_redIntensity; }
    /** 
     * 获取绿色亮度 
     * @return int 绿色亮度
     */
    int greenIntensity() const { return m_greenIntensity; }
    /** 
     * 获取蓝色亮度 
     * @return int 蓝色亮度
     */
    int blueIntensity() const { return m_blueIntensity; }
    /** 
     * 获取伽马值 
     * @return double 伽马值
     */
    double gamma() const { return m_gamma; }
    /** 
     * 保存属性 
     * @return QJsonObject 属性
     */
    virtual QJsonObject save() const{
        QJsonObject json;
        json["name"] = name();
        json["width"] = screenWidth();
        json["height"] = screenHeight();
        json["posX"] = posX();
        json["posY"] = posY();
        json["rotation"] = rotation();
        json["computerName"] = computerName();
        json["enabled"] = enabled();
        json["redIntensity"] = redIntensity();
        json["greenIntensity"] = greenIntensity();
        json["blueIntensity"] = blueIntensity();
        json["gamma"] = gamma();
        return json;
    };
    /** 
     * 加载属性 
     * @param QJsonObject json 属性
     */
    virtual void load(const QJsonObject &json) {
        setName(json["name"].toString());
        setScreenWidth(json["width"].toInt());
        setScreenHeight(json["height"].toInt());
        setPosX(json["posX"].toDouble());
        setPosY(json["posY"].toDouble());
        setRotation(json["rotation"].toDouble());
        setComputerName(json["computerName"].toString());
        setEnabled(json["enabled"].toBool());
        setRedIntensity(json["redIntensity"].toInt(100));
        setGreenIntensity(json["greenIntensity"].toInt(100));
        setBlueIntensity(json["blueIntensity"].toInt(100));
        setGamma(json["gamma"].toDouble(2.20));
    };
    /** 
     * 设置名称 
     * @param QString name 名称
     */
    virtual void setName(const QString &name)
    {
        if (m_name != name) {
            m_name = name;
            emit nameChanged();
        }
    }
    /** 
     * 设置屏幕宽度 
     * @param int width 宽度
     */
    virtual void setScreenWidth(int width)
    {
        if (m_screenWidth != width) {
            m_screenWidth = width;
            emit screenWidthChanged();
            emit resolutionChanged();
        }
    }
    /** 
     * 设置屏幕高度 
     * @param int height 高度
     */
    virtual void setScreenHeight(int height)
    {
        if (m_screenHeight != height) {
            m_screenHeight = height;
            emit screenHeightChanged();
            emit resolutionChanged();
        }
    }
    /** 
     * 设置是否选中 
     * @param bool selected 是否选中
     */
    virtual void setIsSelected(bool selected)
    {
        if (m_isSelected != selected) {
            m_isSelected = selected;
            emit isSelectedChanged();
        }
    }
    /** 
     * 设置位置X 
     * @param qreal x 位置X
     */
    virtual void setPosX(qreal x)
{
    if (!qFuzzyCompare(m_posX, x)) {
        m_posX = x;
        emit posXChanged();
    }
}
    /** 
     * 设置位置Y 
     * @param qreal y 位置Y
     */
    virtual void setPosY(qreal y)
{
    if (!qFuzzyCompare(m_posY, y)) {
        m_posY = y;
        emit posYChanged();
    }
}
    /** 
     * 设置旋转 
     * @param qreal angle 旋转
     */
    virtual void setRotation(qreal angle)
    {
    if (!qFuzzyCompare(m_rotation, angle)) 
    {
        m_rotation = angle;
        emit rotationChanged();
    }
    };
    /** 
     * 设置计算机名称 
     * @param QString name 计算机名称
     */
    virtual void setComputerName(const QString &name)
    {
    if (m_computerName != name) {
        m_computerName = name;
        emit computerNameChanged();
    }
    };
    /** 
     * 设置是否启用 
     * @param bool enabled 是否启用
     */
    virtual void setEnabled(bool enabled)
    {
        if (m_enabled != enabled) {
            m_enabled = enabled;
            emit enabledChanged();
        }
    }
    /** 
     * 设置红色亮度 
     * @param int value 红色亮度
     */
    virtual void setRedIntensity(int value)
    {
        if (m_redIntensity != value) {
            
            m_redIntensity = value;
            emit redIntensityChanged();
        }
    }
    /** 
     * 设置绿色亮度 
     * @param int value 绿色亮度
     */
    virtual void setGreenIntensity(int value)
    {
        if (m_greenIntensity != value) {
            m_greenIntensity = value;
            emit greenIntensityChanged();
        }
    }
    /** 
     * 设置蓝色亮度 
     * @param int value 蓝色亮度
     */
    virtual void setBlueIntensity(int value)
    {
        if (m_blueIntensity != value) {

            m_blueIntensity = value;
            emit blueIntensityChanged();
        }
    }
    /** 
     * 设置伽马值 
     * @param double value 伽马值
     */
    virtual void setGamma(double value)
    {
        if (!qFuzzyCompare(m_gamma, value)) {
            m_gamma = value;
            emit gammaChanged();
        }
    }
    /** 
     * 清除 
     */
    virtual void clear() = 0;
    /** 
     * 调整大小事件 
     * @param QResizeEvent* event 调整大小事件
     */
    virtual void resizeEvent(QResizeEvent* event) override = 0;
    /** 
     * 绘制事件 
     * @param QPaintEvent* event 绘制事件
     */
    virtual void paintEvent(QPaintEvent* event) override = 0;
    /** 
     * 获取实际显示区域 
     * @return QRect 实际显示区域
     */
    virtual QRect getDisplayRect() const = 0;
protected:
    /** 
     * 创建属性窗口 
     */
    virtual void createPropertiesWidget() = 0;
    /** 
     * 属性窗口 
     */
    QDialog* m_propertiesWidget = nullptr;
    // 当前图像
    QImage m_currentImage;
    // 名称
    QString m_name;
    // 屏幕宽度
    int m_screenWidth;
    // 屏幕高度
    int m_screenHeight;
    // 是否选中
    bool m_isSelected;
    // 位置X
    qreal m_posX;
    // 位置Y
    qreal m_posY;
    // 旋转
    qreal m_rotation;
    // 计算机名称
    QString m_computerName = "127.0.0.1";
    // 是否启用
    bool m_enabled = true;
    // 红色亮度
    int m_redIntensity = 100;
    // 绿色亮度
    int m_greenIntensity = 100;
    // 蓝色亮度
    int m_blueIntensity = 100;
    // 伽马值
    double m_gamma = 2.20;
    // 实际显示区域
    QRect m_displayRect;
    // 添加属性窗口控件指针
    struct PropertyWidgets {
        QSpinBox* xPos = nullptr;
        QSpinBox* yPos = nullptr;
        QSpinBox* rotation = nullptr;
        QSpinBox* redIntensity = nullptr;
        QSpinBox* greenIntensity = nullptr;
        QSpinBox* blueIntensity = nullptr;
        QDoubleSpinBox* gamma = nullptr;
        QSlider* redSlider = nullptr;
        QSlider* greenSlider = nullptr;
        QSlider* blueSlider = nullptr;
        QSlider* gammaSlider = nullptr;
    } m_propertyWidgets;

protected slots:
    /** 
     * 属性改变 
     */ 
    virtual void onPropertyChanged() = 0;
    /** 
     * 更新属性窗口 
     */
    virtual void updatePropertyWidgets() = 0;
    /** 
     * 显示属性窗口 
     */
    virtual void showProperties() {
        if (!m_propertiesWidget) {
            createPropertiesWidget();
        }
        m_propertiesWidget->show();
        m_propertiesWidget->raise();
        m_propertiesWidget->activateWindow();
    }
    /** 
     * 隐藏属性窗口 
     */
    virtual void hideProperties() {
        if (m_propertiesWidget) {
            m_propertiesWidget->hide();
        }
    }

public:
    /** 
     * 通知属性改变 
     */
    Q_INVOKABLE void notifyPropertyChanged() {
        onPropertyChanged();
    }
signals:
    /** 
     * 名称改变 
     */
    void nameChanged();
    /** 
     * 屏幕宽度改变 
     */
    void screenWidthChanged();
    /** 
     * 屏幕高度改变 
     */
    void screenHeightChanged();
    /** 
     * 是否选中改变 
     */
    void isSelectedChanged();
    /** 
     * 位置X改变 
     */
    void posXChanged();
    /** 
     * 位置Y改变 
     */
    void posYChanged();
    /** 
     * 旋转改变 
     */
    void rotationChanged();
    /** 
     * 分辨率改变 
     */
    void resolutionChanged();
    /** 
     * 计算机名称改变 
     */
    void computerNameChanged();
    /** 
     * 是否启用改变 
     */
    void enabledChanged();
    /** 
     * 红色亮度改变 
     */
    void redIntensityChanged();
    /** 
     * 绿色亮度改变 
     */
    void greenIntensityChanged();
    /** 
     * 蓝色亮度改变 
     */
    void blueIntensityChanged();
    /** 
     * 伽马值改变 
     */
    void gammaChanged();
    /** 
     * 属性改变 
     */
    void propertyChanged();
};

#endif // ABSTRACTTIMELINESCREEN_HPP 