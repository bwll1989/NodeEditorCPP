// 函数级注释：创建EP按钮控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPButtonWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '按钮';
    const { node, mountNode, header } = createContainer(grid, defaultTitle, 8, 2, opts);
    
    // 1. 同步注册基础 API，确保属性面板立即可用
    registerNode(node, '按钮', {
      getProps(){ return { 
          commandId: initialProps.commandId || '/cmd/demo', 
          bgColor: initialProps.bgColor || '#ffffff',
          fontSize: initialProps.fontSize || '14',
          label: initialProps.label || '执行',
          // 新增样式属性
          buttonColor: initialProps.buttonColor || '#409EFF',
          activeColor: initialProps.activeColor || '#3a8ee6',
          textColor: initialProps.textColor || '#ffffff',
          borderColor: initialProps.borderColor || '#409EFF',
          borderStyle: initialProps.borderStyle || 'solid'
      }; },
      setProps(p){ 
        // 兼容 value 属性更新状态
        if (p.value !== undefined) {
            const v = p.value;
            const isActive = (v === true || String(v).toLowerCase() === 'true' || v == 1);
            if (node.vm) node.vm.isActive = isActive;
        }
         if (p.commandId!==undefined) initialProps.commandId = p.commandId;
         if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
         if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
         if (p.label!==undefined) initialProps.label = p.label;
         if (p.buttonColor!==undefined) initialProps.buttonColor = p.buttonColor;
         if (p.activeColor!==undefined) initialProps.activeColor = p.activeColor;
         if (p.textColor!==undefined) initialProps.textColor = p.textColor;
         if (p.borderColor!==undefined) initialProps.borderColor = p.borderColor;
         if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
         
         // Update Vue instance if it exists
         if (node.vm) {
             if (p.commandId!==undefined) node.vm.commandId = p.commandId;
             if (p.label!==undefined) node.vm.label = p.label;
             if (p.buttonColor!==undefined) node.vm.buttonColor = p.buttonColor;
             if (p.activeColor!==undefined) node.vm.activeColor = p.activeColor;
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

    loadTemplate('widgets/Button/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
             label: initialProps.label || '执行',
             commandId: initialProps.commandId || '/cmd/demo',
             buttonColor: initialProps.buttonColor || '#409EFF',
             activeColor: initialProps.activeColor || '#3a8ee6',
             textColor: initialProps.textColor || '#ffffff',
             borderColor: initialProps.borderColor || '#409EFF',
             borderStyle: initialProps.borderStyle || 'solid',
             fontSize: initialProps.fontSize || '14',
             isActive: false
          };
        },
        computed: {
            buttonStyle() {
                const style = {
                    width: '100%',
                    height: '100%',
                    backgroundColor: this.isActive ? this.activeColor : this.buttonColor,
                    color: this.textColor,
                    borderColor: this.borderColor,
                    borderStyle: this.borderStyle,
                    fontSize: (this.fontSize) + 'px'
                };
                return style;
            }
        },
        methods: {
          exec() {
            // 使用 JSON 发送命令（addr/value）
            const addr = this.commandId || '/cmd/demo';
            const val = 1;
            window.EPWidgets.sendCommand(addr, val);
          },
          onDown() {
              this.isActive = true;
          },
          onUp() {
              this.isActive = false;
          }
        }
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      node.vm = vm; // Store vm for external access
      
      // 应用容器样式 (背景色和字体大小仍可作用于容器，但按钮自身样式优先)
      if(initialProps.bgColor) node.querySelector('.grid-stack-item-content').style.backgroundColor = initialProps.bgColor;
      if(initialProps.fontSize) node.style.fontSize = initialProps.fontSize + 'px';

    }).catch(err => {
      mountNode.innerHTML = `<div style="color:red;font-size:12px;">加载模板失败: ${err}</div>`;
    });

    return node;
  };
})();