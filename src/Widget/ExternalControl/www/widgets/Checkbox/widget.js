// 函数级注释：创建EP勾选框控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPCheckboxWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: '#ffffff',
      fontSize: '14',
      label: '启用',
      checked: false,
      activeColor: '#409EFF',
      textColor: '#111827',
      borderColor: '#409EFF',
      borderStyle: 'solid'
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '勾选',
      templatePath: 'widgets/Checkbox/widget.html',
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
              label: initialProps.label ?? defaults.label,
              checked: initialProps.checked ?? defaults.checked,
              commandId: initialProps.commandId ?? defaults.commandId,
              activeColor: initialProps.activeColor ?? defaults.activeColor,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle
            };
          },
          watch: {
            // 函数级注释：勾选状态变化即发送命令
            checked(nv) {
              const addr = this.commandId || defaults.commandId;
              window.EPWidgets.sendCommand(addr, nv ? 1 : 0);
            }
          },
          computed: {
            // 函数级注释：计算复选框样式（应用颜色变量）
            checkboxStyle() {
              return {
                color: this.textColor,
                borderColor: this.borderColor,
                borderStyle: this.borderStyle,
                '--el-checkbox-checked-bg-color': this.activeColor,
                '--el-checkbox-checked-border-color': this.borderColor,
                '--el-checkbox-text-color': this.textColor
              };
            }
          }
        };
      }
    });
  };
})();
