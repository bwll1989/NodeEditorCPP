// 函数级注释：创建EP分割线控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPDividerWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      bgColor: '#ffffff',
      fontSize: '14',
      text: '分割线',
      lineColor: '#e5e7eb',
      lineWidth: 2,
      borderStyle: 'solid',
      textColor: '#64748b'
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '分割线',
      templatePath: 'widgets/Divider/widget.html',
      initialProps,
      opts,
      defaultW: 24,
      defaultH: 1,
      defaults,
      appFactory(template) {
        return {
          template,
          data() {
            return {
              text: initialProps.text ?? defaults.text,
              lineColor: initialProps.lineColor ?? defaults.lineColor,
              lineWidth: Number(initialProps.lineWidth ?? defaults.lineWidth),
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              textColor: initialProps.textColor ?? defaults.textColor
            };
          }
        };
      }
    });
  };
})();
