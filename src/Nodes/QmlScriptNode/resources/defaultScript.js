// 在 initInterface() 中声明 QML 面板控件；在 inputEventHandler() 中处理输入。

function initInterface() {
    Node.setUiSchema([
        Slider({ key: "gain", label: "增益", min: 0, max: 2, default: 1.0, onChanged: "onGain" }),
        SpinBox({ key: "offset", label: "偏移", min: -100, max: 100, step: 1, default: 0, onChanged: "onOffset" }),
        CheckBox({ key: "enabled", label: "启用处理", default: true, onChanged: "onEnabled" }),
        LineEdit({ key: "tag", label: "标签", default: "out", placeholder: "输出 tag 字段" }),
        Button({ text: "处理当前输入", onClicked: "processCurrentInput" })
    ]);
}

function readNumber(input, fallback) {
    if (input === undefined || input === null) return fallback;
    if (typeof input === "number") return input;
    if (input.default !== undefined) return Number(input.default);
    return fallback;
}

function buildOutput(portIndex) {
    var input = Node.getInputValue(portIndex);
    var gain = Number(Node.getSetting("gain"));
    var offset = Number(Node.getSetting("offset"));
    var enabled = Boolean(Node.getSetting("enabled"));
    var tag = String(Node.getSetting("tag"));

    if (isNaN(gain)) gain = 1.0;
    if (isNaN(offset)) offset = 0;

    var x = readNumber(input, 0);
    var y = enabled ? (x * gain + offset) : x;

    return {
        default: y,
        sourcePort: portIndex,
        gain: gain,
        offset: offset,
        enabled: enabled,
        tag: tag
    };
}

function inputEventHandler(portIndex) {
    Node.setOutputValue(0, buildOutput(portIndex));
}

function processCurrentInput() {
    inputEventHandler(Node.inputIndex());
}

function onSettingChanged(key, value) {
    processCurrentInput();
}

function onGain(key, value) { processCurrentInput(); }
function onOffset(key, value) { processCurrentInput(); }
function onEnabled(key, value) { processCurrentInput(); }
