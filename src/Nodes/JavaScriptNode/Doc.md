# JS Script

## 1. 节点说明

在节点内编写 **JavaScript** 脚本，处理多路输入并写出多路输出。脚本可通过全局对象 `Node` 读写端口、用 Qt 控件 API 动态生成界面；支持 `initInterface` 初始化 UI、`inputEventHandler` 响应输入变化。

脚本始终在 GUI 线程中执行。点击「保存」会重建引擎并重新调用 `initInterface`。

## 2. 端口说明

### 输入

| 端口 | 类型 | 说明 |
|------|------|------|
| IN 0 … IN n | VariableData | 上游数据（默认 4 个，可编辑增删） |

### 输出

| 端口 | 类型 | 说明 |
|------|------|------|
| OUT 0 … OUT n | VariableData | 脚本通过 `Node.setOutputValue` 写出的数据（默认 1 个，可编辑） |

## 3. 界面说明

- **代码编辑器**：编写脚本；**保存** 重新加载代码并重建界面。
- **上方布局区**：脚本用 `Node.addToLayout` 添加的控件（按钮、滑块等）。

全局可用：`Node` 对象、`SpinBox` / `Button` / `Label` / `HSlider` / `VSlider` / `CheckBox` / `LineEdit` / `ComboBox` / `DoubleSpinBox`，以及 `console`、`Math`、`JSON` 等 QJSEngine 扩展。

## 4. 使用说明

1. 在编辑器中编写 `initInterface` 与 `inputEventHandler`（及自定义函数）。
2. 点击保存加载脚本并刷新界面。
3. 将上游节点接到 IN 端口；下游接 OUT 端口读取结果。
4. 输入变化时会调用 `inputEventHandler(索引)`。
5. 节点可缩放；端口数量会随工程保存。

## 5. 脚本生命周期回调

脚本顶层需定义下列函数（可选；未定义则对应阶段不执行）。由引擎在适当时机调用，**不是** `Node` 上的方法。

### `initInterface()`

- **调用时机**：脚本加载 / 点击保存后，引擎 `evaluate` 成功后立即调用。
- **用途**：创建控件、连接信号、用 `Node.addToLayout` 放入节点上方布局区。
- **注意**：每次保存都会先 `clearLayout` 再重建引擎，因此界面逻辑应写在本函数内，不要依赖上次运行残留的控件引用。

```javascript
function initInterface() {
  var btn = new Button();
  btn.setText("发送");
  btn.clicked.connect(function () {
    Node.setOutputValue(0, { default: 1 });
  });
  Node.addToLayout(btn, 0, 0);
}
```

### `inputEventHandler(index)`

- **参数**：`index`（number）— 刚刚更新的输入端口索引（0 起）。
- **调用时机**：任一 IN 端口收到上游 `VariableData` 时。
- **用途**：读取该路输入并计算输出。同一时刻不会重入（处理中忽略新的触发）。

```javascript
function inputEventHandler(index) {
  var v = Node.getInputValue(index)["default"];
  Node.setOutputValue(0, { default: v });
}
```

## 6. Node 对象 API

全局对象 `Node` 绑定当前节点模型，所有方法均在 GUI 线程可用。

### 端口数量与索引

| 方法 | 返回 | 说明 |
|------|------|------|
| `Node.getInputCount()` | number | 当前输入端口数量（与节点属性中的 In 口数一致） |
| `Node.getOutputCount()` | number | 当前输出端口数量 |
| `Node.inputIndex()` | number | 最近一次触发 `inputEventHandler` 的输入端口索引 |

```javascript
var nIn = Node.getInputCount();
var nOut = Node.getOutputCount();
var lastIn = Node.inputIndex();
```

### 读写端口数据

端口数据为 **键值对象**（对应 `VariableData`）。常见约定使用键 `"default"` 存放主值；也可使用任意其它字符串键。索引越界或尚无数据时，读写侧会得到空对象 `{}`。

| 方法 | 参数 | 返回 | 说明 |
|------|------|------|------|
| `Node.getInputValue(index)` | `index`: number | object | 读取 IN `index` 的当前值 |
| `Node.getOutputValue(index)` | `index`: number | object | 读取本节点已写出的 OUT `index` 缓存 |
| `Node.setOutputValue(index, value)` | `index`: number；`value`: object 或标量 | void | 写入 OUT 并通知下游更新 |

**`setOutputValue` 的 `value` 规则：**

- 传入 **对象**：各属性原样写入（函数属性会被忽略）。
- 传入 **非对象**（数字、字符串、布尔、数组等）：自动包装为 `{ default: value }`。

```javascript
// 推荐：显式对象
Node.setOutputValue(0, { default: 42 });
Node.setOutputValue(0, { default: "hello", unit: "m" });

// 等价：标量会写入 default
Node.setOutputValue(0, 42);

// 读取
var map = Node.getInputValue(0);
var main = map["default"];
```

### 布局（嵌入控件）

| 方法 | 参数 | 返回 | 说明 |
|------|------|------|------|
| `Node.addToLayout(widget)` | `widget`: 控件实例 | number | 追加到网格布局；成功返回控件 id，失败返回 `-1` |
| `Node.addToLayout(widget, row, column)` | `row` / `column`: 网格行列（0 起） | number | 放到指定格 |
| `Node.addToLayout(widget, row, column, rowSpan, columnSpan)` | 另含跨行 / 跨列 | number | 占多格 |
| `Node.clearLayout()` | — | void | 移除并销毁已添加的全部控件 |

- 控件须为支持的全局类型实例（如 `new Button()`），且必须在 GUI 线程中添加。
- `row` / `column` 省略或为默认 `-1` 时，行为等同“自动追加”。
- 控件加入布局后由 C++ 持有；脚本侧勿再手动 `delete`。
- 保存脚本时会自动清空布局；也可在 `initInterface` 开头主动 `Node.clearLayout()`。

```javascript
function initInterface() {
  var label = new Label();
  label.setText("增益");
  var slider = new HSlider();
  slider.setRange(0, 100);
  slider.setValue(50);

  Node.addToLayout(label, 0, 0);
  Node.addToLayout(slider, 0, 1);

  slider.valueChanged.connect(function (v) {
    Node.setOutputValue(0, { default: v });
  });
}
```

## 7. 可用控件类型（简述）

在 `initInterface` 中用 `new` 创建，再 `Node.addToLayout`：

| 类型 | 常用方法 / 信号 |
|------|-----------------|
| `SpinBox` | `value()` / `setValue` / `setRange`；`valueChanged` |
| `DoubleSpinBox` | 同上（浮点） |
| `HSlider` / `VSlider` | `value()` / `setValue` / `setRange`；`valueChanged` |
| `Button` | `setText`；`clicked` |
| `Label` | `setText` / `text()` / `setAlignment` |
| `LineEdit` | `text()` / `setText`；`textChanged` / `returnPressed` |
| `CheckBox` | `isChecked` / `setChecked`；`toggled` |
| `ComboBox` | `addItem` / `addItems` / `currentIndex` / `currentText`；`currentIndexChanged` |

信号连接示例：`btn.clicked.connect(function () { ... });`

## 8. 示例

### 输入加倍

```javascript
function initInterface() {}

function inputEventHandler(index) {
  var v = Node.getInputValue(index)["default"];
  Node.setOutputValue(0, { default: v * 2 });
}
```

上游数值 → IN 0 → 本节点 → OUT 0 输出加倍结果。

### 多路输入合并 + 界面预览

```javascript
var preview;

function initInterface() {
  preview = new Label();
  preview.setText("-");
  Node.addToLayout(preview, 0, 0);
}

function inputEventHandler(index) {
  var a = Node.getInputValue(0)["default"] || 0;
  var b = Node.getInputValue(1)["default"] || 0;
  var sum = Number(a) + Number(b);
  preview.setText(String(sum));
  Node.setOutputValue(0, { default: sum });
}
```
