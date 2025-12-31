// 函数级注释：创建EP竖向分割线控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPVDividerWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '竖向分割线';
    const { node, mountNode } = createContainer(grid, defaultTitle, 1, 10, opts);
    
    // 1. 同步注册基础 API
    registerNode(node, '竖向分割线', {
      getProps(){ return { 
          bgColor: initialProps.bgColor || '#ffffff',
          fontSize: initialProps.fontSize || '14',
          // 样式属性
          lineColor: initialProps.lineColor || '#e5e7eb',
          lineWidth: initialProps.lineWidth || 2,
          borderStyle: initialProps.borderStyle || 'solid'
      }; },
      setProps(p){ 
        if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
        if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
        if (p.lineColor!==undefined) initialProps.lineColor = p.lineColor;
        if (p.lineWidth!==undefined) initialProps.lineWidth = Number(p.lineWidth);
        if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
      }
    });

    if (!ready) {
      mountNode.innerHTML = '<div style="color:#b91c1c;font-size:12px;">Vue/ElementPlus 未加载或路径错误</div>';
      return node;
    }

    loadTemplate('widgets/VDivider/widget.html').then(template => {
      const app = vue.createApp({
        template,
        data() {
          return {
            lineColor: initialProps.lineColor || '#e5e7eb',
            lineWidth: initialProps.lineWidth || 2,
            borderStyle: initialProps.borderStyle || 'solid'
          };
        }
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      
      // 应用样式
      if(initialProps.bgColor) node.querySelector('.grid-stack-item-content').style.backgroundColor = initialProps.bgColor;
      if(initialProps.fontSize) node.style.fontSize = initialProps.fontSize + 'px';

      // 2. Vue 就绪后，覆盖注册
      registerNode(node, '竖向分割线', {
        getProps(){ return { 
            bgColor: initialProps.bgColor, 
            fontSize: initialProps.fontSize,
            lineColor: vm.lineColor,
            lineWidth: vm.lineWidth,
            borderStyle: vm.borderStyle
        }; },
        setProps(p){ 
          if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
          if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
          if (p.lineColor!==undefined) vm.lineColor = p.lineColor;
          if (p.lineWidth!==undefined) vm.lineWidth = Number(p.lineWidth);
          if (p.borderStyle!==undefined) vm.borderStyle = p.borderStyle;
        }
      });
    }).catch(err => {
      mountNode.innerHTML = `<div style="color:red;font-size:12px;">加载模板失败: ${err}</div>`;
    });

    return node;
  };
})();
