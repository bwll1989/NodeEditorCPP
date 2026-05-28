# File Load 插件

从媒体库或磁盘加载文件并输出数据。当前注册节点：**Image File**、**JSON File**、**INI File**（CSV 节点在代码中可选注册）。

---

## Image File

### 1. 节点说明

加载一张图片文件并持续输出为 ImageData。

### 2. 端口说明

#### 输入

无。

#### 输出

- **输出 0**（ImageData）：加载的图像。

### 3. 界面说明

文件选择下拉框（媒体库），变更后自动重新加载。

### 4. 使用说明

选择图片路径；`/file` 可外部切换文件。

### 5. 示例

Image File（logo.png）→ Image Layout，作为固定角标。

---

## JSON File

### 1. 节点说明

读取 JSON 文件，以 VariableData 输出解析结果（对象/表结构）。

### 2. 端口说明

#### 输入

无。

#### 输出

- **输出 0**（VariableData）：JSON 内容。

### 3. 界面说明

文件选择框。

### 4. 使用说明

选定 JSON 后供 Extract、脚本等节点读取配置。

### 5. 示例

JSON File（show.json）→ Extract → Inject，按配置触发场景。

---

## INI File

### 1. 节点说明

读取 INI 配置文件，输出为 VariableData（键值结构）。

### 2. 端口说明

#### 输入

无。

#### 输出

- **输出 0**（VariableData）：INI 解析结果。

### 3. 界面说明

文件选择框。

### 4. 使用说明

用于加载设备参数、路径等静态配置。

### 5. 示例

INI File → Extract `/device/ip` → TCP Client 主机地址。
