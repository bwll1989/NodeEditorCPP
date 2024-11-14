//
// Created by Administrator on 2023/12/13.
//
#pragma once
#pragma once
#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QJsonObject>
#include <QJsonArray>

class MergeInterface : public QWidget {
    Q_OBJECT

public:
    MergeInterface(QWidget *parent = nullptr)
        : QWidget(parent), tableView(new QTableView(this)), model(new QStandardItemModel(0, 3, this)) {  // 3列模型
        setupUI();
        // 连接 itemChanged 信号
        connect(model, &QStandardItemModel::itemChanged, this, &MergeInterface::onItemChanged);
    }

    // 导出数据到 QJsonObject
    QJsonObject exportToJson() {
        QJsonArray rowsArray;

        // 遍历每一行
        for (int row = 0; row < model->rowCount(); ++row) {
            QJsonObject rowObject;
            rowObject["ID"] = model->item(row, 0)->text();
            rowObject["Name"] = model->item(row, 1)->text();
            rowObject["Rename"] = model->item(row, 2)->text();

            // 将当前行的数据添加到 JSON 数组
            rowsArray.append(rowObject);
        }

        // 将 JSON 数组存储到 JSON 对象
        QJsonObject jsonObject;
        jsonObject["rows"] = rowsArray;

        return jsonObject;
    }

    // 从 QJsonObject 导入数据
    void importFromJson(const QJsonObject &jsonObject) {
        QJsonArray rowsArray = jsonObject["rows"].toArray();

        // 清空现有的模型数据
        model->removeRows(0, model->rowCount());

        // 遍历 JSON 数组并将数据插入到模型中
        for (int row = 0; row < rowsArray.size(); ++row) {
            QJsonObject rowObject = rowsArray[row].toObject();
            addRow();  // 添加新行
            model->setItem(row, 0, new QStandardItem(rowObject["ID"].toString()));
            model->setItem(row, 1, new QStandardItem(rowObject["Name"].toString()));
            model->setItem(row, 2, new QStandardItem(rowObject["Rename"].toString()));
        }
    }

    int rowCount() const {
        return model->rowCount();
    }

    // 根据行号返回该行的所有值
    QStringList getRowValues(int row) const {
        QStringList rowValues;

        // 检查行号有效性
        if (row >= 0 && row < model->rowCount()) {
            // 获取该行所有单元格的值
            for (int col = 0; col < model->columnCount(); ++col) {
                QStandardItem *item = model->item(row, col);
                rowValues.append(item ? item->text() : "");
            }
        }

        return rowValues;
    }

signals:
    // 自定义信号，当表格发生变化时发出
    void tableChanged();

    // 自定义信号，发出被修改的行号
    void rowChanged(int row);

private slots:
    // 槽函数：删除指定行
    void deleteRow(int row) {
        model->removeRow(row);
        emit tableChanged();  // 发出表格内容发生变化的信号
    }

    // 槽函数：新增一行
    void addRow() {
        int rowCount = model->rowCount();

        // 插入新行
        model->insertRow(rowCount);

        // 设置新行的默认值
        model->setItem(rowCount, 0, new QStandardItem("0"));  // ID 列示例默认值
        model->setItem(rowCount, 1, new QStandardItem("default"));
        model->setItem(rowCount, 2, new QStandardItem("Rename"));

        // 创建删除按钮
        QPushButton *deleteButton = new QPushButton("Delete");
        connect(deleteButton, &QPushButton::clicked, this, [this, row = rowCount]() {
            deleteRow(row);
        });
        tableView->setIndexWidget(model->index(rowCount, 3), deleteButton);

        emit tableChanged();  // 发出表格内容发生变化的信号
    }

    void onItemChanged(QStandardItem *item) {
        int row = item->row();  // 获取被修改单元格的行号
        emit rowChanged(row);    // 发出行号信号
    }

private:
    QTableView *tableView;
    QStandardItemModel *model;
    QPushButton *addRowButton;

    void setupUI() {
        // 设置表格初始表头
        model->setHorizontalHeaderLabels({"ID", "Name", "Rename", "Action"});
        tableView->setModel(model);
        tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tableView->verticalHeader()->setVisible(false);  // 隐藏行号

        // 创建新增行按钮
        addRowButton = new QPushButton("Add Row");
        connect(addRowButton, &QPushButton::clicked, this, &MergeInterface::addRow);

        // 设置布局
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(tableView);

        // 将按钮添加到布局中
        mainLayout->addWidget(addRowButton);
        setLayout(mainLayout);
    }
};
