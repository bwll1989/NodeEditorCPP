# ImageSwitchNode 使用说明

## 用途
ImageSwitchNode 用于在多路图像输入中选择一路输出。常用于多路摄像头/素材切换、A/B 画面切换等。

## 端口
- 输入（可编辑）
  - IMAGE 0..IMAGE N：图像输入（ImageData）
  - INDEX：索引输入（VariableData，int），始终是最后一个输入端口
- 输出（1）
  - IMAGE 0：ImageData（被选中的那一路图像）

说明：
- INDEX=0 对应 IMAGE 0，INDEX=1 对应 IMAGE 1，以此类推。
- 如果 INDEX 指向的输入没有连接或为空，输出为空。

## 外部控制（可选）
- `/index`：设置当前输出索引（int）

## 使用步骤
1. 在“端口编辑”里增加需要的图像输入数量。
2. 连接多路图像到 IMAGE 输入端口。
3. 用数值源节点连接 INDEX（或用外部控制设置 /index）。
4. 将输出 IMAGE 0 连接到下游节点。

