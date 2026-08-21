/*! @plugin {
    "author": "作者名称",
    "category": "DMX",
    "description": "插件描述",
    "embeddable": true,
    "inputs": 3,
    "name": "Smoke 2CH",
    "outputs": 1,
    "portEditable": false,
    "resizable": false,
    "version": "1.0.0"
}
 */


var channelCount=2;

var currentDMX = {default:{ addressMap: {} }};  // 初始化输出对象

var channels=[];

var startChannel

var channelNames=["Smoke","Fan"];
function initInterface() {

    Node.clearLayout();

    var nameDisplay = new Label();

    nameDisplay.setText("Start Channel");

    Node.addToLayout(nameDisplay, 0, 0, 1, 1);

	startChannel=new SpinBox();

	startChannel.setRange(1,512-channelCount);

	Node.addToLayout(startChannel, 0, 1, 1, 1);

    for (var i = 1; i <= channelCount; i++) {

        var index = new Label();

        index.setText(channelNames[i-1]);

        Node.addToLayout(index, i, 0, 1, 1);

        var channelValue = new HSlider();

		channels.push(channelValue);

		channelValue.setRange(0,255)

        // 关键修复：此时 valueChanged(i) 返回的是一个函数对象（回调）

        channelValue.valueChanged.connect(valueChanged(i));

        channelValue.setValue(0)

        Node.addToLayout(channelValue, i, 1, 1, 1);

    }

}

function inputEventHandler(index){

    console.log(index)

	if(index==0){

		startChannel.setValue(Node.getInputValue(index)["default"]);

	}

	else{

		channels[index-1].setValue(Node.getInputValue(index)["default"]);

}

}



function valueChanged(channelIndex) {

    return function (val) {

        //console.log("Channel", channelIndex, "->", val);

		currentDMX["default"]["addressMap"][channelIndex+startChannel.value()-1]=val;

        Node.setOutputValue(0,currentDMX)

    };

}

