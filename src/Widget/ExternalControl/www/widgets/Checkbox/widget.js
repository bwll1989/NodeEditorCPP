// 函数级注释：创建EP勾选框控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPCheckboxWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '勾选';
    const { node, mountNode, header } = createContainer(grid, defaultTitle, 8, 2, opts);
    
    // 1. 同步注册基础 API
    registerNode(node, '勾选', {
      getProps(){ return { 
          commandId: initialProps.commandId || '/cmd/demo', 
          bgColor: initialProps.bgColor || '#ffffff',
          fontSize: initialProps.fontSize || '14',
          label: initialProps.label || '启用', 
          checked: initialProps.checked ?? false,
          // 样式属性
          activeColor: initialProps.activeColor || '#409EFF',
          textColor: initialProps.textColor || '#111827',
          borderColor: initialProps.borderColor || '#409EFF',
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
        if (p.label!==undefined) initialProps.label = p.label;
        if (p.checked!==undefined) initialProps.checked = !!p.checked;
        if (p.activeColor!==undefined) initialProps.activeColor = p.activeColor;
        if (p.textColor!==undefined) initialProps.textColor = p.textColor;
        if (p.borderColor!==undefined) initialProps.borderColor = p.borderColor;
        if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
      }
    });

    if (!ready) {
      mountNode.innerHTML = '<div style="color:#b91c1c;font-size:12px;">Vue/ElementPlus 未加载或路径错误</div>';
      return node;
    }

    loadTemplate('widgets/Checkbox/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
              label: initialProps.label || '启用', 
              checked: initialProps.checked ?? false,
              commandId: initialProps.commandId || '/cmd/demo',
              activeColor: initialProps.activeColor || '#409EFF',
              textColor: initialProps.textColor || '#111827',
              borderColor: initialProps.borderColor || '#409EFF',
              borderStyle: initialProps.borderStyle || 'solid'
          };
        },
        watch: {
          // 函数级注释：勾选状态变化即发送命令
          checked(nv) {
            const addr = this.commandId || '/cmd/demo';
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
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      
      // 应用样式
      if(initialProps.bgColor) node.querySelector('.grid-stack-item-content').style.backgroundColor = initialProps.bgColor;
      if(initialProps.fontSize) node.style.fontSize = initialProps.fontSize + 'px';

      // 2. Vue 就绪后，覆盖注册
      registerNode(node, '勾选', {
        getProps(){ return { 
            commandId: vm.commandId, 
            bgColor: initialProps.bgColor, 
            fontSize: initialProps.fontSize,
            label: vm.label, 
            checked: !!vm.checked,
            activeColor: vm.activeColor,
            textColor: vm.textColor,
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
          if (p.label!==undefined) vm.label = p.label; 
          if (p.checked!==undefined) vm.checked = !!p.checked; 
          if (p.activeColor!==undefined) vm.activeColor = p.activeColor;
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
