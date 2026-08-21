/*! @plugin {
    "author": "WuBin",
    "category": "JS Plugins",
    "description": "数值重映射",
    "embeddable": true,
    "inputs": 1,
    "name": "NumericalRemap",
    "outputs": 1,
    "portEditable": true,
    "resizable": false,
    "version": "1.0.0"
}
 */

var inputMin;
var inputMax;
var outputMin;
var outputMax;
const scale = (number, [inMin, inMax], [outMin, outMax]) => {
    // if you need an integer value use Math.floor or Math.ceil here
    return (number - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
}

function initInterface() {
	var lab=new Label();
	lab.setText("INPUT")
	Node.addToLayout(lab,0,0);
	inputMin=new LineEdit();
	inputMin.setText("0");
	Node.addToLayout(inputMin,0,1)
	inputMax=new LineEdit();
	inputMax.setText("1");
	Node.addToLayout(inputMax,0,3)
	var label = new Label();
	label.setText("To");
	Node.addToLayout(label,1,2)
	var lab=new Label();
	lab.setText("OUTPUT")
	Node.addToLayout(lab,2,0);
	outputMin=new LineEdit();
	outputMin.setText("0");
	Node.addToLayout(outputMin,2,1)
	outputMax=new LineEdit();
	outputMax.setText("255");
	Node.addToLayout(outputMax,2,3);
	
}
function remap(value) {
	var low1=parseFloat(inputMin.text())

	var high1=parseFloat(inputMax.text())

	var low2= parseFloat(outputMin.text())
	var high2=parseFloat(outputMax.text())
	if(value<=low1)
		return low2
	if(value>=high1)
		return high2
	return low2 + (high2 - low2) * (value - low1) / (high1 - low1);
}

function inputEventHandler(index){
	Node.setOutputValue(index,remap(Node.getInputValue(index)["default"]))

}
