//
// Created by bwll1 on 2024/9/26.
//

#include "PropertyWidget.hpp"
#include "Widget/PortEditWidget/PortEditAddRemoveWidget.hpp"
#include <QTabBar>

namespace
{
class DynamicStackedWidget final : public QStackedWidget
{
public:
    explicit DynamicStackedWidget(QWidget* parent = nullptr)
        : QStackedWidget(parent)
    {}

    QSize sizeHint() const override
    {
        if (auto* w = currentWidget()) {
            return w->sizeHint();
        }
        return QStackedWidget::sizeHint();
    }

    QSize minimumSizeHint() const override
    {
        if (auto* w = currentWidget()) {
            return w->minimumSizeHint();
        }
        return QStackedWidget::minimumSizeHint();
    }
};
}

PropertyWidget::PropertyWidget(CustomDataFlowGraphModel *model,QWidget *parent)
    : QWidget(parent)
    , _model(model)
{
    initLayout();
}

PropertyWidget::~PropertyWidget() {

}

void PropertyWidget::setPropertyTabWidget(QWidget *widget)
{
    if (!_propertyStack) { return; }
    if (!widget) {
        _propertyStack->setCurrentWidget(emptyProperty);
        _currentPropertyWidget = nullptr;
        _propertyStack->updateGeometry();
        _propertyPage->updateGeometry();
        return;
    }

    if (_propertyStack->indexOf(widget) == -1) {
        widget->setParent(_propertyStack);
        _propertyStack->addWidget(widget);
    }

    _propertyStack->setCurrentWidget(widget);
    _currentPropertyWidget = widget;
    _propertyStack->updateGeometry();
    _propertyPage->updateGeometry();
}

void PropertyWidget::setPortTabWidget(QWidget *widget)
{
    if (!_portStack) { return; }
    if (!widget) {
        _portStack->setCurrentWidget(emptyPortEdit);
        _portStack->updateGeometry();
        _portPage->updateGeometry();
        return;
    }

    if (_portStack->indexOf(widget) == -1) {
        widget->setParent(_portStack);
        _portStack->addWidget(widget);
    }

    _portStack->setCurrentWidget(widget);
    _portStack->updateGeometry();
    _portPage->updateGeometry();
}

void PropertyWidget::initLayout() {

    emptyProperty=new QLabel(QStringLiteral("无可编辑属性"));
    emptyProperty->setAlignment(Qt::AlignCenter);

    emptyPortEdit=new QLabel(QStringLiteral("端口不可编辑"));
    emptyPortEdit->setAlignment(Qt::AlignCenter);

    lay=new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);

    _tabWidget = new QTabWidget(this);
    _propertyPage = new QWidget(_tabWidget);
    auto *propertyLayout = new QVBoxLayout(_propertyPage);
    propertyLayout->setContentsMargins(0, 0, 0, 0);
    propertyLayout->setSpacing(0);
    _propertyStack = new DynamicStackedWidget(_propertyPage);
    _propertyStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    propertyLayout->addWidget(_propertyStack, 1);
    _propertyStack->addWidget(emptyProperty);
    _propertyStack->setCurrentWidget(emptyProperty);

    _portPage = new QWidget(_tabWidget);
    auto *portLayout = new QVBoxLayout(_portPage);
    portLayout->setContentsMargins(0, 0, 0, 0);
    portLayout->setSpacing(0);
    _portStack = new DynamicStackedWidget(_portPage);
    _portStack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    portLayout->addWidget(_portStack, 1);
    _portStack->addWidget(emptyPortEdit);
    _portStack->setCurrentWidget(emptyPortEdit);

    _tabWidget->addTab(_propertyPage, QStringLiteral("属性面板"));
    _tabWidget->addTab(_portPage, QStringLiteral("端口编辑"));
    _tabWidget->tabBar()->setVisible(_tabWidget->count() > 1);
    // _tabWidget->setDocumentMode(true);
    lay->addWidget(_tabWidget, 1);
}

void PropertyWidget::setModel(CustomDataFlowGraphModel* model)
{
    _model = model;
    update(QtNodes::InvalidNodeId);
}

void PropertyWidget::update(const QtNodes::NodeId nodeId) {

    if(!_model || !_model->nodeExists(nodeId)){
        setPropertyTabWidget(nullptr);

        if (_portEditWidget) {
            if (_portStack && _portStack->indexOf(_portEditWidget) != -1) {
                _portStack->removeWidget(_portEditWidget);
            }
            delete _portEditWidget;
            _portEditWidget = nullptr;
        }
        setPortTabWidget(nullptr);

        const int portTab = _tabWidget ? _tabWidget->indexOf(_portPage) : -1;
        if (portTab != -1) {
            _tabWidget->removeTab(portTab);
        }
        if (_tabWidget) {
            _tabWidget->tabBar()->setVisible(_tabWidget->count() > 1);
        }

        _currentNodeId = QtNodes::InvalidNodeId;
        return;
    }

    QWidget *propertyWidget = nullptr;
    const bool widgetEmbeddable = _model->nodeData(nodeId, QtNodes::NodeRole::WidgetEmbeddable).value<bool>();
    if (!widgetEmbeddable) {
        propertyWidget = _model->nodeData(nodeId, QtNodes::NodeRole::Widget).value<QWidget *>();
    }

    const bool portEditable = _model->nodeData(nodeId, QtNodes::NodeRole::PortEditable).value<bool>();
    if (portEditable) {
        if (!_tabWidget->indexOf(_portPage) != -1) {
        }
        if (_tabWidget->indexOf(_portPage) == -1) {
            _tabWidget->addTab(_portPage, QStringLiteral("端口编辑"));
        }

        if (!_portEditWidget || _currentNodeId != nodeId) {
            if (_portEditWidget) {
                if (_portStack && _portStack->indexOf(_portEditWidget) != -1) {
                    _portStack->removeWidget(_portEditWidget);
                }
                delete _portEditWidget;
            }
            _portEditWidget = new PortEditAddRemoveWidget(nodeId, *_model, this);
            _currentNodeId = nodeId;
        }
        const auto inCount = _model->nodeData(nodeId, QtNodes::NodeRole::InPortCount).toUInt();
        const auto outCount = _model->nodeData(nodeId, QtNodes::NodeRole::OutPortCount).toUInt();
        _portEditWidget->populateButtons(QtNodes::PortType::In, static_cast<unsigned int>(inCount));
        _portEditWidget->populateButtons(QtNodes::PortType::Out, static_cast<unsigned int>(outCount));
    } else {
        if (_portEditWidget) {
            if (_portStack && _portStack->indexOf(_portEditWidget) != -1) {
                _portStack->removeWidget(_portEditWidget);
            }
            delete _portEditWidget;
            _portEditWidget = nullptr;
        }
        const int portTab = _tabWidget->indexOf(_portPage);
        if (portTab != -1) {
            _tabWidget->removeTab(portTab);
        }
    }

    _tabWidget->tabBar()->setVisible(_tabWidget->count() > 1);

    setPropertyTabWidget(propertyWidget);
    setPortTabWidget(_portEditWidget);

    const int propertyTabIndex = _tabWidget->indexOf(_propertyPage);
    const int portTabIndex = _tabWidget->indexOf(_portPage);

    if (!propertyWidget && _portEditWidget && portTabIndex != -1) {
        _tabWidget->setCurrentIndex(portTabIndex);
    } else if (propertyWidget && propertyTabIndex != -1) {
        _tabWidget->setCurrentIndex(propertyTabIndex);
    } else if (propertyTabIndex != -1) {
        _tabWidget->setCurrentIndex(propertyTabIndex);
    }

}
