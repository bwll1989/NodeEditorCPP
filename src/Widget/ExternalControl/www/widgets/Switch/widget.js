// 函数级注释：创建EP开关控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPSwitchWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: 'transparent',
      fontSize: '14',
      checked: false,
      onColor: '#13ce66',
      offColor: '#ff4949',
      borderColor: '#e5e7eb',
      borderStyle: 'none'
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '开关',
      templatePath: 'widgets/Switch/widget.html',
      initialProps,
      opts,
      defaultW: 8,
      defaultH: 2,
      defaults,
      valueMapper(value) {
        return { checked: window.EPWidgets.toBool(value) };
      },
      appFactory(template) {
        return {
          template,
          data() {
            return {
              checked: initialProps.checked ?? defaults.checked,
              commandId: initialProps.commandId ?? defaults.commandId,
              onColor: initialProps.onColor ?? defaults.onColor,
              offColor: initialProps.offColor ?? defaults.offColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle
            };
          },
          watch: {
            // 函数级注释：开关状态变化即发送命令
            checked(nv) {
              const addr = this.commandId || defaults.commandId;
              window.EPWidgets.sendCommand(addr, nv ? '1' : '0');
            }
          },
          computed: {
            // 函数级注释：计算容器样式（用于边框自定义）
            containerStyle() {
              return {
                width: '100%',
                height: '100%',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                borderColor: this.borderColor,
                borderStyle: this.borderStyle
              };
            },
            // 函数级注释：计算开关样式（通过 ElementPlus CSS 变量强制应用 on/off 颜色）
            switchStyle() {
              return {
                '--el-switch-on-color': this.onColor,
                '--el-switch-off-color': this.offColor
              };
            }
          }
        };
      }
    });
  };
})();
