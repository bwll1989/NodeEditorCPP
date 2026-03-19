// 函数级注释：创建EP竖向分割线控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPVDividerWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      bgColor: '#ffffff',
      fontSize: '14',
      lineColor: '#e5e7eb',
      lineWidth: 2,
      borderStyle: 'solid'
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '竖向分割线',
      templatePath: 'widgets/VDivider/widget.html',
      initialProps,
      opts,
      defaultW: 1,
      defaultH: 10,
      defaults,
      appFactory(template) {
        return {
          template,
          data() {
            return {
              lineColor: initialProps.lineColor ?? defaults.lineColor,
              lineWidth: Number(initialProps.lineWidth ?? defaults.lineWidth),
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle
            };
          }
        };
      }
    });
  };
})();
