// 函数级注释：注册“超链接”控件，提供属性读写与样式应用
(function(){
  const EP = window.EPWidgets || {};
  // 函数级注释：创建超链接控件
  EP.createEPLinkWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      title: '超链接',
      commandId: '/cmd/demo',
      label: '打开链接',
      href: 'https://example.com',
      targetBlank: true,
      buttonColor: '#409EFF',
      activeColor: '#0e5d45',
      textColor: '#ffffff',
      borderColor: '#409EFF',
      borderStyle: 'none',
      fontSize: '14',
      bgColor: 'transparent'
    };

    // 使用与“按钮”一致的标准渲染路径（依赖 Vue/ElementPlus）

    // 标准 Vue 渲染路径
    return EP.createVueWidget(grid, {
      type: '超链接',
      templatePath: 'widgets/Hyperlink/widget.html',
      initialProps,
      opts,
      defaultW: 8,
      defaultH: 2,
      defaults,
      valueMapper(value) {
        const s = String(value ?? '').trim();
        if (/^https?:\\/\\//i.test(s)) return { href: s };
        return { text: s };
      },
      appFactory(template) {
        return {
          template,
          data() {
            return {
              title: initialProps.title ?? defaults.title,
              label: initialProps.label ?? defaults.label,
              href: initialProps.href ?? defaults.href,
              targetBlank: initialProps.targetBlank ?? defaults.targetBlank,
              buttonColor: initialProps.buttonColor ?? defaults.buttonColor,
              activeColor: initialProps.activeColor ?? defaults.activeColor,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              fontSize: initialProps.fontSize ?? defaults.fontSize,
              bgColor: initialProps.bgColor ?? defaults.bgColor,
              isActive: false
            };
          },
          computed: {
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
            openLink(){
              try { window.open(this.href, this.targetBlank ? '_blank' : '_self', 'noopener'); } catch {}
            },
            onDown() { this.isActive = true; },
            onUp() { this.isActive = false; }
          }
        };
      }
    });
  };
  window.EPWidgets = EP;
})();