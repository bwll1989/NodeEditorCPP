#pragma once

#include <QWidget>
#include <QFont>
#include <QFontMetrics>
#include <QList>

#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

class QVBoxLayout;

/**
 * 端口对齐列：只负责固定行高槽位，不规定行内控件。
 *
 * 行高 = DefaultHorizontalNodeGeometry 的 port step：
 *   QFontMetrics(QFont()).height() + 10
 *
 * 用法：往 addRow/insertRow 传入任意内容控件；列负责套固定高度壳并对齐端口。
 */
class GUI_ELEMENTS_EXPORT PortAlignedColumn : public QWidget
{
    Q_OBJECT

public:
    static int rowPitch()
    {
        return QFontMetrics(QFont()).height() + 10;
    }

    explicit PortAlignedColumn(QWidget *parent = nullptr);
    ~PortAlignedColumn() override;

    int rowHeight() const { return m_rowHeight; }
    int rowCount() const;

    /** 行内容控件（不含外壳）；越界返回 nullptr */
    QWidget *rowContent(int index) const;

    /**
     * 在末尾追加一行。接管 content 所有权，装入固定高度壳。
     * @return 新行索引
     */
    int appendRow(QWidget *content);

    /** 在 index 处插入一行，接管 content 所有权 */
    void insertRow(int index, QWidget *content);

    /**
     * 删除一行（外壳与内容一并销毁）。
     * @return 是否成功
     */
    bool removeRow(int index);

    void clear();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void rowCountChanged(int count);

private:
    QWidget *makeShell(QWidget *content);
    void insertShell(int index, QWidget *shell, QWidget *content);

    QVBoxLayout *m_rowsLayout = nullptr;
    QList<QWidget *> m_shells;
    QList<QWidget *> m_contents;
    int m_rowHeight = 23;
};
