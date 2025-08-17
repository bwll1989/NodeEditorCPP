# CustomScript 帮助文档

## 1. 系统概述
CustomScript 插件系统允许您创建独立的 JavaScript 插件，每个插件都会在节点编辑器中注册为独立的节点类型。系统支持通过元数据配置节点属性，提供了丰富的API和UI控件，可以创建交互式界面，实现复杂的数据处理和控制流程。

## 2. 插件架构
### 2.1 工作流程
1. **启动时扫描**：系统启动时自动扫描插件目录/JSPlugins中的 `.js` 文件
2. **插件扫描**：系统会扫描插件目录/JSPlugins/中的所有 `.js` 文件，提取每个文件的元数据
3. **插件注册**：为每个 JS 文件创建独立的节点类型并注册到节点编辑器
4. **节点创建**：用户创建节点时，工厂函数自动设置对应的插件信息
5. **脚本执行**：节点运行时执行特定的插件脚本

### 2.2 插件元数据
每个插件文件需要在开头定义元数据，用于配置节点属性：

```js
/*
{
    "name": "我的插件",
    "description": "插件描述",
    "version": "1.0.0",
    "author": "作者名",
    "category": "工具",
    "inputs": 2,
    "outputs": 1,
    "embeddable": true,
    "resizable": true,
    "portEditable": false
}
*/
```

#### 元数据字段说明
- `name`: 插件名称（必需）
- `description`: 插件描述
- `version`: 版本号
- `author`: 作者
- `category`: 分类
- `inputs`: 输入端口数量（默认：1）
- `outputs`: 输出端口数量（默认：1）
- `embeddable`: 是否可嵌入UI控件（默认：true）
- `resizable`: 是否可调整大小（默认：false）
- `portEditable`: 端口是否可编辑（默认：false）

## 3. 基本用法
1. 在插件目录中创建 `.js` 文件
2. 在文件开头添加元数据注释
3. 编写插件逻辑代码
4. 重启应用程序以加载新插件
5. 在节点编辑器中找到并使用您的插件节点

## 4. 节点API参考
### 4.1 Node对象
Node对象是JavaScript节点在JS引擎中的实例，提供了以下方法：

#### 输入/输出操作
节点间数据的输入和输出通过输入端口和输出端口进行。数据以QVariantMap为基础类型进行传递，在调用时需以js对象的方式进行处理。以下是Node对象的输入/输出操作方法：

``` js
/*获取输入端口的值
* 参数:输入端口索引
* 返回:输入端口的值
*/
Node.getInputValue(portIndex);
```
获取的数值需以js object的方式进行处理，例如：
``` js
var value=Node.getInputValue(0)["default"];
var num=value.toInt();
```

``` js
/*获取输出端口的值
* 参数:输出端口索引
* 返回:输出端口的值
*/
Node.getOutputValue(portIndex);
```
获取的数值需以js object的方式进行处理，例如：
``` js
var value=Node.getOutputValue(0)["default"];
var num=value.toInt();
```
``` js
/*设置输出端口的值
* 参数:输出端口索引,输出值
* 返回:无
*/
Node.setOutputValue(portIndex,value);
```
设置输出端口的值时，程序会自动将js object转换为QVariantMap,不需要额外操作。例如：
```js
Node.setOutputValue(0,100);
Node.setOutputValue(0,"100");
```
``` js
/*获取当前触发的输入端口索引
* 参数:无
* 返回:当前触发的输入端口索引
*/
Node.inputIndex();
```
``` js
/*获取输入端口数量
* 参数:无
* 返回:输入端口数量
*/
Node.getInputCount();
```
``` js
/*获取输出端口数量
* 参数:无
* 返回:输出端口数量
*/
Node.getOutputCount();
```
#### UI控件操作
``` js
/*清除节点的UI布局
* 注意：该方法会清除所有UI控件
* 参数:无
* 返回:无
*/
Node.clearLayout();
```
``` js
//添加控件到布局
//参数:控件对象,行位置,列位置,行跨度,列跨度
//返回:控件ID，用于后续引用
Node.addToLayout(widgetObj,x,y,rowSpan,columnSpan);
``` 
### 3.2 支持的UI控件
节点支持以下UI控件类型，可以通过JavaScript创建和操作：

#### SpinBox (整数输入框)
``` js
var spinBox=new SpinBox();
spinBox.setMinimum(0);
spinBox.setMaximum(100);
spinBox.setValue(50);
Node.addToLayout(spinBox,0,0);
``` 
#### DoubleSpinBox (浮点数输入框)
``` js
var doubleSpinBox = new DoubleSpinBox();
doubleSpinBox.setMinimum(0.0);
doubleSpinBox.setMaximum(1.0);
doubleSpinBox.setValue(0.5);
doubleSpinBox.setDecimals(2);
Node.addToLayout(doubleSpinBox, 0, 1);
```
#### Button (按钮)
``` js
var button = new Button();
button.setText("点击我");
button.clicked.connect(function() {
    // 按钮点击事件处理
    console.log("按钮被点击");
});
Node.addToLayout(button, 1, 0);
``` 
#### VSlider (垂直滑块)
``` js
var vslider = new VSlider();
vslider.setMinimum(0);
vslider.setMaximum(100);
vslider.setValue(50);
Node.addToLayout(vslider, 0, 2);
``` 
#### HSlider (水平滑块)
``` js
var hslider = new HSlider();
hslider.setMinimum(0);
hslider.setMaximum(100);
hslider.setValue(50);
Node.addToLayout(hslider, 2, 0);
``` 
#### Label (标签)
``` js
var label = new Label();
label.setText("这是一个标签");
Node.addToLayout(label, 3, 0);
``` 
#### LineEdit (文本输入框)
``` js
var lineEdit = new LineEdit();
lineEdit.setText("默认文本");
lineEdit.textChanged.connect(function() {
    // 文本变化事件处理
    console.log("文本已更改为: " + lineEdit.text());
});
Node.addToLayout(lineEdit, 4, 0);
``` 
#### ComboBox (下拉框)
``` js
var comboBox = new ComboBox();
comboBox.addItem("选项1");
comboBox.addItem("选项2");
comboBox.addItem("选项3");
comboBox.currentIndexChanged.connect(function(index) {
    // 选项变化事件处理
    console.log("选择了选项: " + comboBox.currentText());
});
Node.addToLayout(comboBox, 5, 0);
``` 
#### CheckBox (复选框)
``` js
var checkBox = new CheckBox();
checkBox.setText("启用选项");
checkBox.setChecked(true);
checkBox.stateChanged.connect(function(state) {
    // 状态变化事件处理
    console.log("复选框状态: " + checkBox.isChecked());
});
Node.addToLayout(checkBox, 6, 0);
```
## 4. 生命周期函数
### 4.1 initInterface
当节点初始化或脚本加载时，系统会自动调用 initInterface 函数（如果存在）。您可以在此函数中创建和设置UI控件。

``` js
function initInterface() {
    // 创建UI控件
    var label = new Label();
    label.setText("控制面板");
    Node.addToLayout(label, 0, 0);
    
    // 创建更多控件...
}
```
## 5. 完整示例
### 5.1 简单计算器
``` js
var display;
var result = 0;
var operation = "";
var firstNumber = true;

function initInterface() {
    //清除所有控件后重新创建
    Node.clearLayout();
    // 创建显示屏
    display = new LineEdit();
    display.setText("0");
    Node.addToLayout(display, 0, 0, 1, 3);
    
    // 创建数字按钮
    for (var i = 1; i <= 9; i++) {
        var btn = new Button();
        btn.setText(i.toString());
        btn.clicked.connect(createNumberClickHandler(i));
        Node.addToLayout(btn, Math.floor((i-1)/3) + 1, (i-1) % 3);
    }
    
    // 创建0按钮
    var zeroBtn = new Button();
    zeroBtn.setText("0");
    zeroBtn.clicked.connect(createNumberClickHandler(0));
    Node.addToLayout(zeroBtn, 4, 1);
    
    // 创建运算符按钮
    var operators = ["+", "-", "*", "/"];
    for (var i = 0; i < operators.length; i++) {
        var btn = new Button();
        btn.setText(operators[i]);
        btn.clicked.connect(createOperatorClickHandler(operators
        [i]));
        Node.addToLayout(btn, i+1, 3);
    }
    
    // 创建等号按钮
    var equalsBtn = new Button();
    equalsBtn.setText("=");
    equalsBtn.clicked.connect(calculate);
    Node.addToLayout(equalsBtn, 5, 3);
    
    // 创建清除按钮
    var clearBtn = new Button();
    clearBtn.setText("C");
    clearBtn.clicked.connect(function() {
        display.setText("0");
        result = 0;
        operation = "";
        firstNumber = true;
    });
    Node.addToLayout(clearBtn, 4, 0);
}

function createNumberClickHandler(num) {
    return function() {
        if (firstNumber || display.text() === "0") {
            display.setText(num.toString());
            firstNumber = false;
        } else {
            display.setText(display.text() + num.toString());
        }
    };
}

function createOperatorClickHandler(op) {
    return function() {
        result = parseFloat(display.text());
        operation = op;
        firstNumber = true;
    };
}

function calculate() {
    var secondNumber = parseFloat(display.text());
    var newResult = 0;
    
    switch (operation) {
        case "+":
            newResult = result + secondNumber;
            break;
        case "-":
            newResult = result - secondNumber;
            break;
        case "*":
            newResult = result * secondNumber;
            break;
        case "/":
            if (secondNumber !== 0) {
                newResult = result / secondNumber;
            } else {
                display.setText("错误");
                return;
            }
            break;
    }
    
    display.setText(newResult.toString());
    result = newResult;
    firstNumber = true;
    
    // 将结果输出到端口0
    var output = {};
    output.value = newResult;
    Node.setOutputValue(0, output);
}
```
### 5.2 数据处理示例
``` js
function initInterface() {
    Node.clearLayout();
    var processBtn = new Button();
    processBtn.setText("处理数据");
    processBtn.clicked.connect(processData);
    Node.addToLayout(processBtn, 0, 0);
}

// 当有输入数据时自动处理
var inputData = Node.getInputValue(Node.inputIndex());
if (inputData && Object.keys(inputData).length > 0) {
    processData();
}

function processData() {
    // 获取输入数据
    var data = Node.getInputValue(0);
    
    // 检查数据是否存在
    if (!data || Object.keys(data).length === 0) {
        console.log("没有输入数据");
        return;
    }
    
    // 处理数据
    var result = {};
    
    // 如果输入是数字数组，计算平均值
    if (Array.isArray(data.values)) {
        var sum = 0;
        for (var i = 0; i < data.values.length; i++) {
            sum += data.values[i];
        }
        result.average = sum / data.values.length;
        result.count = data.values.length;
    } else {
        // 复制输入数据并添加时间戳
        result = Object.assign({}, data);
        result.timestamp = new Date().toISOString();
    }
    
    // 输出处理后的数据
    Node.setOutputValue(0, result);
}
```
## 6. 调试技巧
1. 使用 console.log() 输出调试信息
2. 在脚本中添加错误处理
3. 使用节点的"运行"按钮手动触发脚本执行
4. 检查JavaScript控制台中的错误信息
## 7. 最佳实践
1. 在 initInterface() 函数中创建和设置UI控件,节点创建时会自动调用此函数创建节点界面
2. 使用事件处理函数响应用户交互
3. 使用有意义的变量名和注释
4. 将复杂逻辑拆分为多个函数
5. 在处理输入数据前检查其有效性
6. 使用对象存储和传递数据
## 8. 限制和注意事项
1. JavaScript引擎基于Qt的QJSEngine，支持ECMAScript标准
2. 不支持直接访问文件系统和网络（除非通过节点的输入/输出）
3. UI控件创建后会保持在节点上，直到调用 clearLayout()
4. 大量UI控件可能会影响性能
5. 脚本执行错误会在控制台中显示