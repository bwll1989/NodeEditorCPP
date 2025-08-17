/*! @plugin {
    "name": "calcultor",
    "description": "计算器",
    "version": "1.0.0",
    "author": "吴斌",
    "category": "Controls",
    "inputs":1,
    "outputs":1
} */

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
  
    Node.setOutputValue(0, newResult);
}