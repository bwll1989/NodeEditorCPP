//
// Created by Administrator on 2023/12/13.
//

#include <QLineEdit>

#include "QWidget"
#include "QLabel"
#include "QLayout"
#include "QGridLayout"
#include <QGroupBox>
#include <QVariantMap>
#include <QToolTip>

class ArtnetInInterface: public QGroupBox {
    Q_OBJECT
public:
    explicit ArtnetInInterface(QWidget *parent = nullptr){
        this->setCheckable(true);
        this->setChecked(false);
        this->setTitle("Artnet过滤器");
        
        // 设置提示信息
        universeEdit->setToolTip("输入Universe编号进行过滤");
        channelsEdit->setToolTip("输入通道编号列表进行过滤，格式：1,2,5-10,15");
        
        // 连接信号槽
        connect(channelsEdit, &QLineEdit::textChanged, this, &ArtnetInInterface::onChannelsFilterChanged);
        
        main_layout->addWidget(universeLabel, 0, 0);
        main_layout->addWidget(universeEdit, 0, 1);
        main_layout->addWidget(channelsFilter, 1, 0);
        main_layout->addWidget(channelsEdit, 1, 1);
        this->setLayout(main_layout);
    }
    
    /**
     * 解析通道过滤器文本
     * @return 包含所有需要过滤的通道编号的列表
     */
    QList<int> parseChannelsFilter() const {
        QList<int> channels;
        QString text = channelsEdit->text().trimmed();
        
        if (text.isEmpty()) {
            return channels;
        }
        
        // 按逗号分割
        QStringList parts = text.split(',', Qt::SkipEmptyParts);
        
        for (const QString &part : parts) {
            // 检查是否是范围 (例如 "5-10")
            if (part.contains('-')) {
                QStringList range = part.split('-');
                if (range.size() == 2) {
                    bool okStart = false, okEnd = false;
                    int start = range[0].trimmed().toInt(&okStart);
                    int end = range[1].trimmed().toInt(&okEnd);
                    
                    if (okStart && okEnd && start <= end) {
                        for (int i = start; i <= end; ++i) {
                            if (i >= 0 && i <= 512 && !channels.contains(i)) {
                                channels.append(i);
                            }
                        }
                    }
                }
            } else {
                // 单个通道
                bool ok = false;
                int channel = part.trimmed().toInt(&ok);
                if (ok && channel >= 0 && channel <= 512 && !channels.contains(channel)) {
                    channels.append(channel);
                }
            }
        }
        
        // 排序通道列表
        std::sort(channels.begin(), channels.end());
        return channels;
    }

signals:
    void UniverseChanged(const int &port);
    void ChannelsFilterChanged(const QList<int> &channels);

public slots:
    void valueChanged(const QString& propertyName, const QVariant& value) {
        if (propertyName=="Universe") {
            emit UniverseChanged(value.toInt());
        }
    }
    
    /**
     * 通道过滤器文本变化时触发
     */
    void onChannelsFilterChanged() {
        emit ChannelsFilterChanged(parseChannelsFilter());
    }

public:
    QLineEdit* universeEdit = new QLineEdit();
    QLineEdit* channelsEdit = new QLineEdit();

private:
    QGridLayout *main_layout = new QGridLayout(this);
    QLabel *universeLabel = new QLabel("universe: ");
    QLabel *channelsFilter = new QLabel("channels: ");
};

