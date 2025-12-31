// 函数级注释：创建EP标签控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPLabelWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '标签';
    const { node, mountNode } = createContainer(grid, defaultTitle, 8, 2, opts);
    
    // 1. 同步注册基础 API
    registerNode(node, '标签', {
      getProps(){ return { 
          commandId: initialProps.commandId || '/cmd/demo',
          bgColor: initialProps.bgColor || '#ffffff',
          fontSize: initialProps.fontSize || '14',
          text: initialProps.text || '标签',
          // 新增样式属性
          textColor: initialProps.textColor || '#111827',
          borderColor: initialProps.borderColor || '#e5e7eb',
          borderStyle: initialProps.borderStyle || 'solid'
      }; },
      setProps(p){ 
        if (p.commandId!==undefined) initialProps.commandId = p.commandId;
        if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
        if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
        if (p.text!==undefined) initialProps.text = p.text;
        if (p.textColor!==undefined) initialProps.textColor = p.textColor;
        if (p.borderColor!==undefined) initialProps.borderColor = p.borderColor;
        if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
        if (node.vm) {
          if (p.commandId!==undefined) node.vm.commandId = p.commandId;
          if (p.text!==undefined) node.vm.text = p.text;
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

    loadTemplate('widgets/Label/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
            commandId: initialProps.commandId || '/cmd/demo',
            text: initialProps.text || '标签',
            textColor: initialProps.textColor || '#111827',
            borderColor: initialProps.borderColor || '#e5e7eb',
            borderStyle: initialProps.borderStyle || 'solid',
            fontSize: initialProps.fontSize || '14'
          };
        },
        mounted() {
            // 监听 WebSocket 消息以更新文本
            window.addEventListener('ws-message', this.handleWsMessage);
        },
        beforeUnmount() {
            window.removeEventListener('ws-message', this.handleWsMessage);
        },
        methods: {
            handleWsMessage(e) {
                const msg = e.detail;
                if (msg && msg.commandId === this.commandId) {
                    if (msg.value !== undefined) {
                        this.text = String(msg.value);
                    } else if (msg.text !== undefined) {
                        this.text = String(msg.text);
                    }
                }
            }
        },
        computed: {
          // 函数级注释：计算容器样式（边框等）和文本样式（颜色）
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
          },
          labelStyle() {
            return {
              color: this.textColor,
              fontSize: (this.fontSize) + 'px'
            };
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
      registerNode(node, '标签', {
        getProps(){ return { 
            commandId: vm.commandId,
            bgColor: initialProps.bgColor, 
            fontSize: initialProps.fontSize,
            text: vm.text,
            textColor: vm.textColor,
            borderColor: vm.borderColor,
            borderStyle: vm.borderStyle
        }; },
        setProps(p){ 
          if (p.commandId!==undefined) vm.commandId = p.commandId;
          if (p.bgColor!==undefined) initialProps.bgColor = p.bgColor;
          if (p.fontSize!==undefined) initialProps.fontSize = p.fontSize;
          if (p.text!==undefined) vm.text = p.text;
          if (p.value!==undefined) vm.text = String(p.value); // 支持 value 映射到 text
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
