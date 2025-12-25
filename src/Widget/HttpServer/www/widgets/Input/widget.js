// 函数级注释：创建EP输入框控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPInputWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '输入框';
    const { node, mountNode, header } = createContainer(grid, defaultTitle, 12, 2, opts);
    
    // 1. 同步注册基础 API
    registerNode(node, '输入框', {
      getProps(){ return { 
          commandId: initialProps.commandId || '/cmd/demo', 
          bgColor: initialProps.bgColor || '#ffffff',
          fontSize: initialProps.fontSize || '14',
          placeholder: initialProps.placeholder || '请输入', 
          value: initialProps.value || '',
          // 样式属性
          textColor: initialProps.textColor || '#111827',
          borderColor: initialProps.borderColor || '#e5e7eb',
          borderStyle: initialProps.borderStyle || 'solid'
      }; },
      setProps(p){ 
        if (p.commandId!==undefined) initialProps.commandId = p.commandId;
        if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
        if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
        if (p.placeholder!==undefined) initialProps.placeholder = p.placeholder;
        if (p.value!==undefined) initialProps.value = p.value;
        if (p.textColor!==undefined) initialProps.textColor = p.textColor;
        if (p.borderColor!==undefined) initialProps.borderColor = p.borderColor;
        if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
      }
    });

    if (!ready) {
      mountNode.innerHTML = '<div style="color:#b91c1c;font-size:12px;">Vue/ElementPlus 未加载或路径错误</div>';
      return node;
    }

    loadTemplate('widgets/Input/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
              value: initialProps.value || '', 
              placeholder: initialProps.placeholder || '请输入',
              commandId: initialProps.commandId || '/cmd/demo',
              textColor: initialProps.textColor || '#111827',
              borderColor: initialProps.borderColor || '#e5e7eb',
              borderStyle: initialProps.borderStyle || 'solid'
          };
        },
        watch: {
          // 函数级注释：输入内容变化即发送命令
          value(nv) {
            const addr = this.commandId || '/cmd/demo';
            window.EPWidgets.sendCommand(addr, String(nv));
          }
        },
        computed: {
          // 函数级注释：输入框样式（应用文本与边框颜色）
          inputStyle() {
            return {
              width: '90%',
              color: this.textColor,
              borderColor: this.borderColor,
              borderStyle: this.borderStyle,
              '--el-input-border-color': this.borderColor,
              '--el-input-hover-border-color': this.borderColor,
              '--el-input-focus-border-color': this.borderColor
            };
          }
        }
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      
      
      // 应用样式
      if(initialProps.bgColor) node.querySelector('.grid-stack-item-content').style.backgroundColor = initialProps.bgColor;
      if(initialProps.fontSize) node.style.fontSize = initialProps.fontSize + 'px';

      // 2. Vue 就绪后，覆盖注册
      registerNode(node, '输入框', {
        getProps(){ return { 
            commandId: vm.commandId, 
            bgColor: initialProps.bgColor, 
            fontSize: initialProps.fontSize,
            placeholder: vm.placeholder, 
            value: vm.value,
            textColor: vm.textColor,
            borderColor: vm.borderColor,
            borderStyle: vm.borderStyle
        }; },
        setProps(p){ 
          if (p.commandId!==undefined) vm.commandId = p.commandId; 
          if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
          if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
          if (p.placeholder!==undefined) vm.placeholder = p.placeholder;
          if (p.value!==undefined) vm.value = p.value;
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
