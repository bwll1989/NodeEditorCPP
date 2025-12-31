// 函数级注释：创建EP开关控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPSwitchWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '开关';
    const { node, mountNode } = createContainer(grid, defaultTitle, 8, 2, opts);
    
    // 1. 同步注册基础 API
    registerNode(node, '开关', {
      getProps(){ return { 
          commandId: initialProps.commandId || '/cmd/demo', 
          bgColor: initialProps.bgColor || '#ffffff',
          fontSize: initialProps.fontSize || '14',
          checked: initialProps.checked ?? false,
          // 样式属性
          onColor: initialProps.onColor || '#13ce66',
          offColor: initialProps.offColor || '#ff4949',
          borderColor: initialProps.borderColor || '#e5e7eb',
          borderStyle: initialProps.borderStyle || 'solid'
      }; },
      setProps(p){ 
        // 兼容 value 属性更新状态
        if (p.value !== undefined) {
            const v = p.value;
            p.checked = (v === true || String(v).toLowerCase() === 'true' || v == 1);
        }
        if (p.commandId!==undefined) initialProps.commandId = p.commandId;
        if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
        if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
        if (p.checked!==undefined) initialProps.checked = !!p.checked;
        if (p.onColor!==undefined) initialProps.onColor = p.onColor;
        if (p.offColor!==undefined) initialProps.offColor = p.offColor;
        if (p.borderColor!==undefined) initialProps.borderColor = p.borderColor;
        if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
      }
    });

    if (!ready) {
      mountNode.innerHTML = '<div style="color:#b91c1c;font-size:12px;">Vue/ElementPlus 未加载或路径错误</div>';
      return node;
    }

    loadTemplate('widgets/Switch/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
              checked: initialProps.checked ?? false,
              commandId: initialProps.commandId || '/cmd/demo',
              onColor: initialProps.onColor || '#13ce66',
              offColor: initialProps.offColor || '#ff4949',
              borderColor: initialProps.borderColor || '#e5e7eb',
              borderStyle: initialProps.borderStyle || 'solid'
          };
        },
        watch: {
          // 函数级注释：开关状态变化即发送命令
          checked(nv) {
            const addr = this.commandId || '/cmd/demo';
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
          }
        }
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      
      // 应用样式
      if(initialProps.bgColor) node.querySelector('.grid-stack-item-content').style.backgroundColor = initialProps.bgColor;
      if(initialProps.fontSize) node.style.fontSize = initialProps.fontSize + 'px';

      // 2. Vue 就绪后，覆盖注册
      registerNode(node, '开关', {
        getProps(){ return { 
            commandId: vm.commandId, 
            bgColor: initialProps.bgColor, 
            fontSize: initialProps.fontSize,
            checked: !!vm.checked,
            onColor: vm.onColor,
            offColor: vm.offColor,
            borderColor: vm.borderColor,
            borderStyle: vm.borderStyle
        }; },
        setProps(p){ 
          // 兼容 value 属性更新状态
          if (p.value !== undefined) {
              const v = p.value;
              vm.checked = (v === true || String(v).toLowerCase() === 'true' || v == 1);
          }
          if (p.commandId!==undefined) vm.commandId = p.commandId; 
          if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
          if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
          if (p.checked!==undefined) vm.checked = !!p.checked; 
          if (p.onColor!==undefined) vm.onColor = p.onColor;
          if (p.offColor!==undefined) vm.offColor = p.offColor;
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
