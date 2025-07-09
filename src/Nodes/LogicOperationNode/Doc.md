# 逻辑运算节点文档

## 节点功能
提供基本逻辑门运算功能，支持以下操作：
- AND（逻辑与）
- OR（逻辑或）
- NOT（逻辑非）
- XOR（异或）
- NAND（与非）
- NOR（或非）

## 输入端口
| 端口索引 | 类型   | 说明                      |
|----------|--------|-------------------------|
| 0        | bool   | 主输入信号（左侧输入）     |
| 1        | bool   | 辅助输入信号（右侧输入）   |
| 2        | int    | 运算类型选择（0-5对应上述6种运算）|

## 输出端口
| 端口索引 | 类型   | 说明                |
|----------|--------|-------------------|
| 0        | bool   | 运算结果输出        |
| 1        | string | 当前运算类型名称    |

## 使用示例
```cpp
// 创建逻辑与运算节点
auto node = registry.createNode("LogicOperation");
node->setInputData(0, true);   // 输入A
node->setInputData(1, false);  // 输入B
node->setInputData(2, 0);      // 选择AND运算

// 获取输出
bool result = node->getOutputData(0)->value().toBool(); // 返回false
string opName = node->getOutputData(1)->value().toString(); // 返回"AND"
```

