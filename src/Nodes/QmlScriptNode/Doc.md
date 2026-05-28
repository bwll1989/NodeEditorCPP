# QmlScriptNode

## 1. 节点说明

使用 **JavaScript + QML** 声明式 UI 的脚本节点：脚本定义 `initInterface`、设置项（settings）与界面描述（uiSchema），由 QML 渲染控件；支持输入事件与多路输入/输出，适合较复杂的可配置面板。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| IN 0 … IN n | VariableData | 上游数据（默认 4 个，可编辑） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| OUT 0 … OUT n | VariableData | 脚本写出的数据（默认 1 个，可编辑） |

## 3. 界面说明

- **QML 视图区**：根据 `uiSchema` 显示控件。
- **脚本编辑器**（可打开）：编辑 JS；保存后重新加载脚本。
- **刷新 UI**：按当前脚本重建界面。
- **设置项**：脚本内通过 `Node.setSetting` / `getSettings` 管理，变更时触发 `onSettingChanged`。

脚本 API 含：`Node.getInputValue`、`Node.setOutputValue`、`Node.setUiSchema`、`Node.runScript` 等。

## 4. 使用说明

1. 打开编辑器编写或修改默认脚本，保存后自动重载。
2. 在 `initInterface` 中配置 UI 与默认 settings。
3. 上游接 IN 端口；下游接 OUT 端口。
4. 若出现 `scriptError`，在界面或日志中查看脚本错误信息。
5. 加载过程中 `initializing` 为 true，避免此时依赖 UI 的逻辑。

## 5. 示例

脚本声明一个滑块 setting `gain`，`onSettingChanged` 里根据 IN 0 与 gain 计算后 `setOutputValue(0, …)`；IN 0 接音频分析，OUT 0 接下游处理。
