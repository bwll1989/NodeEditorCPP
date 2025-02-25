#ifndef TIMELINESCREEN_HPP
#define TIMELINESCREEN_HPP

#include <QQuickItem>
#include <QQmlEngine>
#include <QJsonObject>
#include <QObject>
#include <QWidget>
#include <QColor>
#include <QDialog>
#include <QSpinBox>
#include <QSlider>
#include <QDoubleSpinBox>

class TimelineScreen : public QQuickItem
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
    explicit TimelineScreen(QQuickItem *parent = nullptr);
    ~TimelineScreen();

    // Getter methods
    QString name() const { return m_name; }
    int screenWidth() const { return m_screenWidth; }
    int screenHeight() const { return m_screenHeight; }
    bool isSelected() const { return m_isSelected; }
    qreal posX() const { return m_posX; }
    qreal posY() const { return m_posY; }
    qreal rotation() const { return m_rotation; }
    QString resolution() const;
    QString computerName() const { return m_computerName; }
    bool enabled() const { return m_enabled; }
    int redIntensity() const { return m_redIntensity; }
    int greenIntensity() const { return m_greenIntensity; }
    int blueIntensity() const { return m_blueIntensity; }
    double gamma() const { return m_gamma; }

    // Setter methods
    void setName(const QString &name);
    void setScreenWidth(int width);
    void setScreenHeight(int height);
    void setIsSelected(bool selected);
    void setPosX(qreal x);
    void setPosY(qreal y);
    void setRotation(qreal angle);
    void setComputerName(const QString &name);
    void setEnabled(bool enabled);
    void setRedIntensity(int value);
    void setGreenIntensity(int value);
    void setBlueIntensity(int value);
    void setGamma(double value);
 
    QJsonObject save() const;
    void load(const QJsonObject &json);
    // Static method for QML registration
    static void registerType();

    // 添加属性面板相关方法
    Q_INVOKABLE void showProperties();
    Q_INVOKABLE void hideProperties();

signals:
    void nameChanged();
    void screenWidthChanged();
    void screenHeightChanged();

    void isSelectedChanged();
    void posXChanged();
    void posYChanged();
    void rotationChanged();
    void resolutionChanged();
    void propertyChanged();
    void computerNameChanged();
    void enabledChanged();
    void redIntensityChanged();
    void greenIntensityChanged();
    void blueIntensityChanged();
    void gammaChanged();
  

private:
    QString m_name;
    int m_screenWidth;
    int m_screenHeight;
    bool m_isSelected;
    qreal m_posX;
    qreal m_posY;
    qreal m_rotation;
    QString m_computerName = "127.0.0.1";
    bool m_enabled = true;
    QDialog* m_propertiesWidget = nullptr;
    int m_redIntensity = 100;
    int m_greenIntensity = 100;
    int m_blueIntensity = 100;
    double m_gamma = 2.20;
    // 添加属性窗口控件指针
    struct {
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
    void createPropertiesWidget();

private slots:
    void onPropertyChanged();
    void updatePropertyWidgets();  // 添加新的槽函数
};

#endif // TIMELINESCREEN_HPP 