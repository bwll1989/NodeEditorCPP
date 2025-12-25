// 函数级注释：创建EP分割线控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPDividerWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '分割线';
    const { node, mountNode, header } = createContainer(grid, defaultTitle, 24, 1, opts);
    
    // 1. 同步注册基础 API
    registerNode(node, '分割线', {
      getProps(){ return { 
          bgColor: initialProps.bgColor || '#ffffff',
          fontSize: initialProps.fontSize || '14',
          text: initialProps.text || '分割线',
          // 样式属性
          lineColor: initialProps.lineColor || '#e5e7eb',
          lineWidth: initialProps.lineWidth || 2,
          borderStyle: initialProps.borderStyle || 'solid',
          textColor: initialProps.textColor || '#64748b'
      }; },
      setProps(p){ 
        if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
        if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
        if (p.text!==undefined) initialProps.text = p.text;
        if (p.lineColor!==undefined) initialProps.lineColor = p.lineColor;
        if (p.lineWidth!==undefined) initialProps.lineWidth = Number(p.lineWidth);
        if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
        if (p.textColor!==undefined) initialProps.textColor = p.textColor;
      }
    });

    if (!ready) {
      mountNode.innerHTML = '<div style="color:#b91c1c;font-size:12px;">Vue/ElementPlus 未加载或路径错误</div>';
      return node;
    }

    loadTemplate('widgets/Divider/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
            text: initialProps.text || '分割线',
            lineColor: initialProps.lineColor || '#e5e7eb',
            lineWidth: initialProps.lineWidth || 2,
            borderStyle: initialProps.borderStyle || 'solid',
            textColor: initialProps.textColor || '#64748b'
          };
        }
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      
      // 应用样式
      if(initialProps.bgColor) node.querySelector('.grid-stack-item-content').style.backgroundColor = initialProps.bgColor;
      if(initialProps.fontSize) node.style.fontSize = initialProps.fontSize + 'px';

      // 2. Vue 就绪后，覆盖注册
      registerNode(node, '分割线', {
        getProps(){ return { 
            bgColor: initialProps.bgColor, 
            fontSize: initialProps.fontSize,
            text: vm.text,
            lineColor: vm.lineColor,
            lineWidth: vm.lineWidth,
            borderStyle: vm.borderStyle,
            textColor: vm.textColor
        }; },
        setProps(p){ 
          if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
          if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
          if (p.text!==undefined) vm.text = p.text; 
          if (p.lineColor!==undefined) vm.lineColor = p.lineColor;
          if (p.lineWidth!==undefined) vm.lineWidth = Number(p.lineWidth);
          if (p.borderStyle!==undefined) vm.borderStyle = p.borderStyle;
          if (p.textColor!==undefined) vm.textColor = p.textColor;
        }
      });
    }).catch(err => {
      mountNode.innerHTML = `<div style="color:red;font-size:12px;">加载模板失败: ${err}</div>`;
    });

    return node;
  };
})();
