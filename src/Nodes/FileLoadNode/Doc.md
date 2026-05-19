# FileLoadNode 使用说明

FileLoadNode 插件提供若干“文件数据源”节点：不需要输入端口，选择文件后直接从输出端口输出内容，供下游节点使用。

## JSON File
### 用途
加载 JSON 文件并输出为 VariableData。

### 端口
- 输入：无
- 输出（1）
  - VariableData

### 输出内容
- JSON 根为对象：直接输出对象键值
- JSON 根为数组：输出到 `default`（列表）

### 使用步骤
1. 添加 “JSON File” 节点。
2. 在节点里选择媒体库中的 JSON 文件。
3. 将输出连接到脚本/数据处理/显示节点。

### 外部控制（可选）
- `/file`：设置文件路径（string）

## INI File
### 用途
加载 INI 配置文件并输出为 VariableData。

### 端口
- 输入：无
- 输出（1）
  - VariableData

### 使用步骤
1. 添加 “INI File” 节点并选择 INI 文件。
2. 将输出连接到需要读取配置的节点。

### 外部控制（可选）
- `/file`：设置文件路径（string）

## Image File
### 用途
加载图片并输出为 ImageData。

### 端口
- 输入：无
- 输出（1）
  - ImageData

### 使用步骤
1. 添加 “Image File” 节点并选择图片文件。
2. 将输出连接到图像处理或显示节点。

### 外部控制（可选）
- `/file`：设置文件路径（string）

## CSV File（如启用）
如果你的版本启用了 CSV 节点，可用于加载 CSV 并输出表头与数据行（VariableData）。

## 常见问题
- 无输出：确认文件路径正确且文件存在。
- JSON/INI 解析失败：尝试用更简单的示例文件验证格式是否正确。
