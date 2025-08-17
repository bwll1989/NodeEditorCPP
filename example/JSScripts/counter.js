/*! @plugin {
    "name": "Empty template",
    "description": "空模板，指示线了界面",
    "version": "1.0.0",
    "author": "吴斌",
    "category": "Controls",
    "inputs": 1,
    "outputs": 1
} */

var countDisplay;
var clearButton;
var count = 0;
var lastInputValue = null;

/**
 * @brief 初始化用户界面
 * 创建计数显示和清空按钮
 */
function initInterface() {
    // 清除所有控件后重新创建
    Node.clearLayout();
    
    // 创建计数显示标签
    var countLabel = new Label();
    countLabel.setText("计数:");
    Node.addToLayout(countLabel, 0, 0);
    
    // 创建计数显示框
    countDisplay = new LineEdit();
    countDisplay.setText("0");
    Node.addToLayout(countDisplay, 0, 1);
    
    // 创建清空按钮
    clearButton = new Button();
    clearButton.setText("清空计数");
    clearButton.clicked.connect(clearCount);
    Node.addToLayout(clearButton, 1, 0, 1, 2);
    
    // 创建条件设置标签
    var conditionLabel = new Label();
    conditionLabel.setText("条件: 输入值 > 0");
    Node.addToLayout(conditionLabel, 2, 0, 1, 2);
}



function clearCount() {
    console.log("清空计数");
}
