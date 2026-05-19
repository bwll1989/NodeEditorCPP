//
// Created by Administrator on 2023/12/13.
//

#ifndef QMLSCRIPTINTERFACE_HPP
#define QMLSCRIPTINTERFACE_HPP

#include "QWidget"
#include "QVBoxLayout"
#include "QGridLayout"
#include "QSplitter"
#include "JsCodeEditor.h"
#include "QToolButton"
#include "QIcon"
#include "QStyle"
#include <QQuickWidget>
#include <QQmlContext>
#include <QUrl>
using namespace std;
namespace Nodes
{
    class QmlScriptInterface: public QWidget{
        Q_OBJECT
        public:
        // 函数级注释：构造界面，使用 QQuickWidget 加载 QML 面板作为节点 UI。
        explicit QmlScriptInterface(QWidget *parent = nullptr){
            this->setLayout(new QVBoxLayout(this));
            this->layout()->setContentsMargins(0,0,0,0);

            qmlWidget = new QQuickWidget(this);
            qmlWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
            qmlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            // 保留原有 top_Widget/top_layout，便于旧脚本兼容或后续逐步迁移。
            top_Widget->setLayout(top_layout);
            top_layout->setContentsMargins(0,0,0,0);

            // 创建底部布局，用于放置右下角的编辑按钮
            QHBoxLayout *bottomLayout = new QHBoxLayout();
            bottomLayout->setContentsMargins(0,0,5,5);
            bottomLayout->setSpacing(0);

            // 创建编辑图标按钮（更小尺寸）
            editButton = new QToolButton();
            editButton->setIcon(QIcon(":/icons/icons/js.png"));
            editButton->setToolTip("编辑脚本");
            editButton->setFixedSize(18, 18);

            // 将按钮放置在右下角
            bottomLayout->addStretch();
            bottomLayout->addWidget(editButton);

            // 创建底部容器
            QWidget *bottomWidget = new QWidget();
            bottomWidget->setLayout(bottomLayout);
            bottomWidget->setFixedHeight(18);

            // 添加到主布局
            this->layout()->addWidget(qmlWidget);
            this->layout()->addWidget(bottomWidget);
            this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            connect(editButton, &QToolButton::clicked, this, &QmlScriptInterface::toggleEditorMode);
        }

        // 函数级注释：将后端对象（通常为 DataModel）注入 QML 上下文，并加载 QML 面板。
        void setBackend(QObject *backend)
        {
            if (!qmlWidget || !backend) {
                return;
            }
            qmlWidget->rootContext()->setContextProperty("node", backend);
            if (qmlWidget->source().isEmpty()) {
                qmlWidget->setSource(QUrl("qrc:/Qml/main.qml"));
            }
        }

        /**
         * @brief 切换编辑器显示模式（嵌入/独立窗口）
         * 将编辑器显示为独立窗口，并确保跟随主程序退出
         */
        void toggleEditorMode() {
            // 移除父子关系，使其成为独立窗口
            codeWidget->setParent(nullptr);
            
            // 设置为独立窗口
            codeWidget->setWindowTitle("JS脚本编辑器");
            
            // 设置窗口图标
            codeWidget->setWindowIcon(QIcon(":/icons/icons/js.png"));
            
            // 设置窗口标志：独立窗口 + 置顶显示 + 关闭按钮
            codeWidget->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
            
            // 设置窗口属性：当关闭时自动删除
            codeWidget->setAttribute(Qt::WA_DeleteOnClose, false); // 不自动删除，我们手动管理
            codeWidget->setAttribute(Qt::WA_QuitOnClose, false);   // 关闭窗口时不退出应用程序
            
            // 设置窗口大小和显示
            codeWidget->resize(800, 600);
            codeWidget->show();
            codeWidget->setReadOnly(false);

            
            // 激活窗口并置于前台
            codeWidget->activateWindow();
            codeWidget->raise();
        }

        /**
         * @brief 析构函数，确保在销毁时强制关闭独立窗口
         */
        ~QmlScriptInterface() {
            // 强制关闭编辑器窗口
            if (codeWidget && codeWidget->isVisible()) {
                codeWidget->close();
            }
            // 如果窗口仍然存在，强制隐藏
            if (codeWidget) {
                codeWidget->hide();
                codeWidget->setParent(this); // 重新设置父子关系，确保随对象销毁
            }
        }

    public:
        QGridLayout *top_layout = new QGridLayout();
        QWidget *top_Widget = new QWidget();
        QQuickWidget *qmlWidget = nullptr;
        QToolButton *editButton; // 编辑按钮
        JsCodeEditor *codeWidget = new JsCodeEditor("",this);
    };
}
#endif // QmlSCRIPTINTERFACE_HPP

