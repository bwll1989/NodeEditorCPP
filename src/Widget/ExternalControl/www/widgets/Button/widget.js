// 函数级注释：创建EP按钮控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPButtonWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: '#ffffff',
      fontSize: '14',
      label: '执行',
      buttonColor: '#409EFF',
      activeColor: '#0e5d45',
      textColor: '#ffffff',
      borderColor: '#409EFF',
      borderStyle: 'solid',
      isActive: false
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '按钮',
      templatePath: 'widgets/Button/widget.html',
      initialProps,
      opts,
      defaultW: 8,
      defaultH: 2,
      defaults,
      valueMapper(value) {
        return { isActive: window.EPWidgets.toBool(value) };
      },
      appFactory(template) {
        return {
          template,
          data() {
            return {
              label: initialProps.label ?? defaults.label,
              commandId: initialProps.commandId ?? defaults.commandId,
              buttonColor: initialProps.buttonColor ?? defaults.buttonColor,
              activeColor: initialProps.activeColor ?? defaults.activeColor,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              fontSize: initialProps.fontSize ?? defaults.fontSize,
              isActive: false
            };
          },
          computed: {
            // 函数级注释：计算按钮样式（使用 ElementPlus CSS 变量，确保自定义颜色生效）
            buttonStyle() {
              const bg = this.isActive ? this.activeColor : this.buttonColor;
              return {
                width: '100%',
                height: '100%',
                '--el-button-bg-color': bg,
                '--el-button-hover-bg-color': bg,
                '--el-button-active-bg-color': this.activeColor,
                '--el-button-text-color': this.textColor,
                '--el-button-border-color': this.borderColor,
                borderStyle: this.borderStyle,
                fontSize: this.fontSize + 'px'
              };
            }
          },
          methods: {
            // 函数级注释：发送一次按钮触发指令
            exec() {
              const addr = this.commandId || defaults.commandId;
              window.EPWidgets.sendCommand(addr, 1);
            },
            // 函数级注释：按下/抬起事件改变按下状态
            onDown() { this.isActive = true; },
            onUp() { this.isActive = false; }
          }
        };
      }
    });
  };
})();