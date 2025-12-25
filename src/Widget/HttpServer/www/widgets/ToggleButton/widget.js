// 函数级注释：创建EP切换按钮控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPToggleButtonWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '切换按钮';
    const { node, mountNode } = createContainer(grid, defaultTitle, 8, 2, opts);
    
    // 1. 同步注册基础 API
    registerNode(node, '切换按钮', {
      getProps(){ return { 
          commandId: initialProps.commandId || '/cmd/demo', 
          bgColor: initialProps.bgColor || '#ffffff',
          fontSize: initialProps.fontSize || '14',
          labelOn: initialProps.labelOn || '开启',
          labelOff: initialProps.labelOff || '关闭',
          active: initialProps.active ?? false,
          // 新增样式属性
          buttonColor: initialProps.buttonColor || '#409EFF',
          activeColor: initialProps.activeColor || '#3a8ee6',
          pressColor: initialProps.pressColor || '#2a7bd8',
          textColor: initialProps.textColor || '#ffffff',
          borderColor: initialProps.borderColor || '#409EFF',
          borderStyle: initialProps.borderStyle || 'solid'
      }; },
      setProps(p){ 
        // 兼容 value 属性更新状态
        if (p.value !== undefined) {
            const v = p.value;
            p.active = (v === true || String(v).toLowerCase() === 'true' || v == 1);
        }
        if (p.commandId!==undefined) initialProps.commandId = p.commandId;
        if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
        if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
        if (p.labelOn!==undefined) initialProps.labelOn = p.labelOn;
        if (p.labelOff!==undefined) initialProps.labelOff = p.labelOff;
        if (p.active!==undefined) initialProps.active = !!p.active;
        if (p.buttonColor!==undefined) initialProps.buttonColor = p.buttonColor;
        if (p.activeColor!==undefined) initialProps.activeColor = p.activeColor;
        if (p.pressColor!==undefined) initialProps.pressColor = p.pressColor;
        if (p.textColor!==undefined) initialProps.textColor = p.textColor;
        if (p.borderColor!==undefined) initialProps.borderColor = p.borderColor;
        if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
        if (node.vm) {
          if (p.commandId!==undefined) node.vm.commandId = p.commandId;
          if (p.labelOn!==undefined) node.vm.labelOn = p.labelOn;
          if (p.labelOff!==undefined) node.vm.labelOff = p.labelOff;
          if (p.active!==undefined) node.vm.active = !!p.active;
          if (p.buttonColor!==undefined) node.vm.buttonColor = p.buttonColor;
          if (p.activeColor!==undefined) node.vm.activeColor = p.activeColor;
          if (p.pressColor!==undefined) node.vm.pressColor = p.pressColor;
          if (p.textColor!==undefined) node.vm.textColor = p.textColor;
          if (p.borderColor!==undefined) node.vm.borderColor = p.borderColor;
          if (p.borderStyle!==undefined) node.vm.borderStyle = p.borderStyle;
          if (p.fontSize!==undefined) node.vm.fontSize = p.fontSize;
        }
      }
    });

    if (!ready) {
      mountNode.innerHTML = '<div style="color:#b91c1c;font-size:12px;">Vue/ElementPlus 未加载或路径错误</div>';
      return node;
    }

    loadTemplate('widgets/ToggleButton/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
              labelOn: initialProps.labelOn || '开启',
              labelOff: initialProps.labelOff || '关闭',
              active: initialProps.active ?? false,
              commandId: initialProps.commandId || '/cmd/demo',
              buttonColor: initialProps.buttonColor || '#409EFF',
              activeColor: initialProps.activeColor || '#3a8ee6',
              pressColor: initialProps.pressColor || '#2a7bd8',
              textColor: initialProps.textColor || '#ffffff',
              borderColor: initialProps.borderColor || '#409EFF',
              borderStyle: initialProps.borderStyle || 'solid',
              fontSize: initialProps.fontSize || '14',
              isPressed: false
          };
        },
        computed: {
          // 函数级注释：计算按钮样式（支持切换状态与按下颜色）
          buttonStyle() {
            const bg = this.isPressed ? this.pressColor : (this.active ? this.activeColor : this.buttonColor);
            return {
              width: '100%',
              height: '100%',
              backgroundColor: bg,
              color: this.textColor,
              borderColor: this.borderColor,
              borderStyle: this.borderStyle,
              fontSize: (this.fontSize) + 'px'
            };
          }
        },
        methods: {
          // 函数级注释：点击切换状态并发送命令
          toggle() {
            this.active = !this.active;
            const addr = this.commandId || '/cmd/demo';
            window.EPWidgets.sendCommand(addr, this.active ? '1' : '0');
          },
          // 函数级注释：按下/抬起事件改变按下状态颜色
          onDown() {
            this.isPressed = true;
          },
          onUp() {
            this.isPressed = false;
          }
        }
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      node.vm = vm;
      
      // 应用样式
      if(initialProps.bgColor) node.querySelector('.grid-stack-item-content').style.backgroundColor = initialProps.bgColor;
      if(initialProps.fontSize) node.style.fontSize = initialProps.fontSize + 'px';

    // 2. Vue 就绪后，覆盖注册
    registerNode(node, '切换按钮', {
      getProps(){ return { 
          commandId: vm.commandId, 
          bgColor: initialProps.bgColor, 
          fontSize: initialProps.fontSize,
          labelOn: vm.labelOn,
          labelOff: vm.labelOff,
          active: !!vm.active,
          buttonColor: vm.buttonColor,
          activeColor: vm.activeColor,
          pressColor: vm.pressColor,
          textColor: vm.textColor,
          borderColor: vm.borderColor,
          borderStyle: vm.borderStyle
      }; },
      setProps(p){ 
        // 兼容 value 属性更新状态
        if (p.value !== undefined) {
            const v = p.value;
            vm.active = (v === true || String(v).toLowerCase() === 'true' || v == 1);
        }
        if (p.commandId!==undefined) vm.commandId = p.commandId; 
        if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
        if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
        if (p.labelOn!==undefined) vm.labelOn = p.labelOn; 
        if (p.labelOff!==undefined) vm.labelOff = p.labelOff; 
        if (p.active!==undefined) vm.active = !!p.active;
        if (p.buttonColor!==undefined) vm.buttonColor = p.buttonColor;
        if (p.activeColor!==undefined) vm.activeColor = p.activeColor;
        if (p.pressColor!==undefined) vm.pressColor = p.pressColor;
        if (p.textColor!==undefined) vm.textColor = p.textColor;
        if (p.borderColor!==undefined) vm.borderColor = p.borderColor;
        if (p.borderStyle!==undefined) vm.borderStyle = p.borderStyle;
      }
    });
    }).catch(err => {
      mountNode.innerHTML = `<div style="color:red;font-size:12px;">加载模板失败: ${err}</div>`;
    });

    return node;
  };
})();
