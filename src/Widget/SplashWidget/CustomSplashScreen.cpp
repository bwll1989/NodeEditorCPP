//
// Created by bwll1 on 2024/8/29.
//

#include "CustomSplashScreen.hpp"
#include <QCoreApplication>
#include <QEventLoop>

CustomSplashScreen::CustomSplashScreen() {
    basePixmap = QPixmap(":/images/images/loading.png");
    splash = new QSplashScreen(basePixmap);
    splash->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    splash->show();
    render();
}

CustomSplashScreen::~CustomSplashScreen() {
    if (splash) {
        splash->deleteLater();
        splash = nullptr;
    }
}

/**
 * 更新状态文本
 * 在底图下方白色区域绘制状态文字，并立即刷新显示。
 */
void CustomSplashScreen::updateStatus(const QString &status) {
    statusText = status;
    render();
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

/**
 * 隐藏并关闭启动界面
 * 通常在主窗口准备好后调用。
 */
void CustomSplashScreen::finish(QMainWindow *mainWindow) {
    if (splash) {
        splash->finish(mainWindow);
    }
}

/**
 * 渲染当前状态到图片
 * 复制底图，在其下方白色区域居中绘制状态文字，再设置到 QSplashScreen。
 */
/**
 * @brief 渲染启动画面（严格按底图白框比例绘制文字）
 * 函数级注释：
 * - 白框高度按底图设计固定为整体高度的 20%（例如 200px 高图的白框为 40px）
 * - 文本矩形严格位于白框内部，并按白框尺寸计算左右/上下内边距
 * - 文本居中绘制，字号按空间自动缩减以避免溢出（支持高 DPI）
 * - 不再额外绘制底带或渐变，完全依赖底图的白框区域
 */
void CustomSplashScreen::render() {
    if (!splash) return;

    QPixmap composed = basePixmap.copy();
    QPainter painter(&composed);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const int w = composed.width();
    const int h = composed.height();

    /**
     * @brief 严格垂直居中文本到白框内
     * 函数级注释：
     * - 先按比例确定白框与文本可绘制区域
     * - 使用 QFontMetrics 计算文本紧致高度
     * - 将紧致高度在文本区域内进行二次垂直居中，避免视觉偏高
     * - 绘制时仅水平居中，垂直位置由我们计算好的 finalRect 控制
     */
    const qreal bandRatio = 0.15;
    const int bandHeight = qRound(h * bandRatio);
    const QRect bandRect(0, h - bandHeight, w, bandHeight);

    const int sidePadding = qMax(10, qRound(w * 0.025));
    const int vPadding    = qMax(8, qRound(bandHeight * 0.18));
    QRect textRect = bandRect.adjusted(sidePadding, vPadding, -sidePadding, -vPadding);

    QFont font = painter.font();
    font.setPointSizeF(qMax<qreal>(10.0, h / 26.0));
    painter.setFont(font);
    painter.setPen(Qt::black);

    // 自适应缩放字号至文本矩形内
    QFontMetrics fm(font);
    QRect tight = fm.boundingRect(textRect, Qt::AlignHCenter | Qt::TextWordWrap, statusText);
    while ((tight.width() > textRect.width() || tight.height() > textRect.height()) && font.pointSizeF() > 9.0) {
        font.setPointSizeF(font.pointSizeF() - 0.5);
        painter.setFont(font);
        fm = QFontMetrics(font);
        tight = fm.boundingRect(textRect, Qt::AlignHCenter | Qt::TextWordWrap, statusText);
    }

    // 用紧致高度在 textRect 内做二次垂直居中，消除“偏高”问题
    const int finalTop = textRect.top() + (textRect.height() - tight.height()) / 2;
    QRect finalRect(textRect.left(), finalTop, textRect.width(), tight.height());

    painter.drawText(finalRect, Qt::AlignHCenter | Qt::TextWordWrap , statusText);
    painter.end();

    splash->setPixmap(composed);
    splash->raise();
}
