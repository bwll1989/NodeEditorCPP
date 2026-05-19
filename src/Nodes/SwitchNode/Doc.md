# SwitchNode 使用说明

## 用途
在多路输入中按索引选择一路输出（通用的 VariableData 多路选择器）。

## 端口
### 输入（可编辑）
- INPUT 0..INPUT N-1：数据输入（VariableData）
- INDEX：索引输入（VariableData / int），始终是最后一个输入端口

### 输出
- OUTPUT 0：被选中的那一路输入（VariableData）

## 外部控制（可选）
- /index（int）：设置当前索引

## 使用步骤
1. 在“端口编辑”里设置需要的输入数量（最后一个端口会自动作为 INDEX）。
2. 连接多路数据到 INPUT 端口。
3. 给 INDEX 一个整数：
   - 0 输出 INPUT 0
   - 1 输出 INPUT 1
   - 以此类推

## 注意事项
- 当 INDEX 指向的输入未连接/没有数据时，输出可能为空。 
