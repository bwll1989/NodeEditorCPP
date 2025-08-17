/*! @plugin {
    "name": "data process",
    "description": "数据切换",
    "version": "1.0.0",
    "author": "吴斌",
    "category": "Controls",
    "inputs":1,
    "outputs":1
} */

var slider1;
function initInterface() {
    // 在这里编写初始化界面的代码
    // 例如：创建按钮、文本框等
}
var value=Node.getInputValue(0)["default"];


console.log(value)
Node.setOutputValue(0,value)