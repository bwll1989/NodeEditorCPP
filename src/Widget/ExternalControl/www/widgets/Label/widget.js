// 函数级注释：创建EP标签控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPLabelWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: '#ffffff',
      fontSize: '14',
      text: '标签',
      textColor: '#111827',
      borderColor: '#e5e7eb',
      borderStyle: 'solid'
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '标签',
      templatePath: 'widgets/Label/widget.html',
      initialProps,
      opts,
      defaultW: 8,
      defaultH: 2,
      defaults,
      valueMapper(value) {
        return { text: String(value ?? '') };
      },
      appFactory(template) {
        return {
          template,
          data() {
            return {
              commandId: initialProps.commandId ?? defaults.commandId,
              text: initialProps.text ?? defaults.text,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              fontSize: initialProps.fontSize ?? defaults.fontSize
            };
          },
          mounted() {
            // 函数级注释：监听 ws-message 事件以更新文本（兼容旧消息通道）
            window.addEventListener('ws-message', this.handleWsMessage);
          },
          beforeUnmount() {
            window.removeEventListener('ws-message', this.handleWsMessage);
          },
          methods: {
            // 函数级注释：处理 ws-message 事件并更新文本
            handleWsMessage(e) {
              const msg = e.detail;
              if (msg && msg.commandId === this.commandId) {
                if (msg.value !== undefined) this.text = String(msg.value);
                else if (msg.text !== undefined) this.text = String(msg.text);
              }
            }
          },
          computed: {
            // 函数级注释：计算容器样式（边框等）和文本样式（颜色）
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
            labelStyle() {
              return {
                color: this.textColor,
                fontSize: this.fontSize + 'px'
              };
            }
          }
        };
      }
    });
  };
})();
