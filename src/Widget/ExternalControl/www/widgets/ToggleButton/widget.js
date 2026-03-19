// 函数级注释：创建EP切换按钮控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPToggleButtonWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: '#ffffff',
      fontSize: '14',
      labelOn: '开启',
      labelOff: '关闭',
      active: false,
      buttonColor: '#409EFF',
      activeColor: '#0e5d45',
      pressColor: '#0e5d45',
      textColor: '#ffffff',
      borderColor: '#409EFF',
      borderStyle: 'solid',
      isPressed: false
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '切换按钮',
      templatePath: 'widgets/ToggleButton/widget.html',
      initialProps,
      opts,
      defaultW: 8,
      defaultH: 2,
      defaults,
      valueMapper(value) {
        return { active: window.EPWidgets.toBool(value) };
      },
      appFactory(template) {
        return {
          template,
          data() {
            return {
              labelOn: initialProps.labelOn ?? defaults.labelOn,
              labelOff: initialProps.labelOff ?? defaults.labelOff,
              active: initialProps.active ?? defaults.active,
              commandId: initialProps.commandId ?? defaults.commandId,
              buttonColor: initialProps.buttonColor ?? defaults.buttonColor,
              activeColor: initialProps.activeColor ?? defaults.activeColor,
              pressColor: initialProps.pressColor ?? defaults.pressColor,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              fontSize: initialProps.fontSize ?? defaults.fontSize,
              isPressed: false
            };
          },
          computed: {
            // 函数级注释：计算按钮样式（使用 ElementPlus CSS 变量，确保自定义颜色生效）
            buttonStyle() {
              const bg = this.isPressed ? this.pressColor : (this.active ? this.activeColor : this.buttonColor);
              return {
                width: '100%',
                height: '100%',
                '--el-button-bg-color': bg,
                '--el-button-hover-bg-color': bg,
                '--el-button-active-bg-color': this.pressColor,
                '--el-button-text-color': this.textColor,
                '--el-button-border-color': this.borderColor,
                borderStyle: this.borderStyle,
                fontSize: this.fontSize + 'px'
              };
            }
          },
          methods: {
            // 函数级注释：点击切换状态并发送命令
            toggle() {
              this.active = !this.active;
              const addr = this.commandId || defaults.commandId;
              window.EPWidgets.sendCommand(addr, this.active ? '1' : '0');
            },
            // 函数级注释：按下/抬起事件改变按下状态颜色
            onDown() { this.isPressed = true; },
            onUp() { this.isPressed = false; }
          }
        };
      }
    });
  };
})();
