// 函数级注释：创建EP输入框控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPInputWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: 'transparent',
      fontSize: '14',
      placeholder: '请输入',
      value: '',
      textColor: '#111827',
      borderColor: '#e5e7eb',
      borderStyle: 'none'
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '输入框',
      templatePath: 'widgets/Input/widget.html',
      initialProps,
      opts,
      defaultW: 12,
      defaultH: 2,
      defaults,
      valueMapper(value) {
        return { value: String(value ?? '') };
      },
      appFactory(template) {
        return {
          template,
          data() {
            return {
              value: initialProps.value ?? defaults.value,
              placeholder: initialProps.placeholder ?? defaults.placeholder,
              commandId: initialProps.commandId ?? defaults.commandId,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              bgColor: initialProps.bgColor ?? defaults.bgColor
            };
          },
          watch: {
            // 函数级注释：输入内容变化即发送命令
            value(nv) {
              const addr = this.commandId || defaults.commandId;
              window.EPWidgets.sendCommand(addr, String(nv));
            }
          },
          computed: {
            // 函数级注释：输入框样式（应用文本与边框颜色）
            inputStyle() {
              return {
                width: '100%',
                height: '100%',
                boxSizing: 'border-box',
                color: this.textColor,
                borderColor: this.borderColor,
                borderStyle: this.borderStyle,
                backgroundColor: this.bgColor,
                '--el-input-border-color': this.borderColor,
                '--el-input-hover-border-color': this.borderColor,
                '--el-input-focus-border-color': this.borderColor,
                '--el-fill-color-blank': this.bgColor,
                '--el-input-bg-color': this.bgColor,
                '--el-color-white': this.bgColor
              };
            }
          }
        };
      }
    });
  };
})();
