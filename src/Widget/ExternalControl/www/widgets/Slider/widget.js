// 函数级注释：创建EP滑块控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPSliderWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '滑块';
    const { node, mountNode, header } = createContainer(grid, defaultTitle, 12, 2, opts);
    
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: 'transparent',
      fontSize: '14',
      min: 0,
      max: 100,
      value: 0,
      barColor: '#e5e7eb',
      fillColor: '#2b6cb0',
      valueColor: '#111827',
      borderColor: '#e5e7eb',
      borderStyle: 'none'
    };
    const coercers = {
      fontSize: (v) => String(v ?? '14'),
      min: (v) => Number(v),
      max: (v) => Number(v),
      value: (v) => Number(v)
    };

    // 1. 同步注册基础 API
    registerNode(node, '滑块', window.EPWidgets.createPropsApi({
      node,
      initialProps,
      vm: null,
      defaults,
      coercers,
      valueMapper(value) { return { value: Number(value) }; }
    }));

    if (!ready) {
      mountNode.innerHTML = '<div style="color:#b91c1c;font-size:12px;">Vue/ElementPlus 未加载或路径错误</div>';
      return node;
    }

    loadTemplate('widgets/Slider/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
              min: initialProps.min ?? 0, 
              max: initialProps.max ?? 100, 
              value: initialProps.value ?? 0,
              commandId: initialProps.commandId || '/cmd/demo',
              barColor: initialProps.barColor || '#e5e7eb',
              fillColor: initialProps.fillColor || '#2b6cb0',
              valueColor: initialProps.valueColor || '#111827',
              borderColor: initialProps.borderColor || '#e5e7eb',
              borderStyle: initialProps.borderStyle || 'solid',
              fontSize: initialProps.fontSize || '14'
          };
        },
        computed: {
          // 函数级注释：计算滑块条样式
          barStyle() {
            return {
              position: 'relative',
              width: '100%',
              height: '100%',
              backgroundColor: this.barColor,
              borderRadius: '4px',
              cursor: 'pointer',
              borderColor: this.borderColor,
              borderStyle: this.borderStyle,
              touchAction: 'none'
            };
          },
          fillStyle() {
            return {
              position: 'absolute',
              left: '0',
              top: '0',
              bottom: '0',
              width: '0',
              backgroundColor: this.fillColor,
              borderRadius: '4px'
            };
          },
          valueStyle() {
            return {
              position: 'absolute',
              left: '50%',
              top: '50%',
              transform: 'translate(-50%,-50%)',
              pointerEvents: 'none',
              color: this.valueColor,
              fontWeight: 600,
              fontSize: (this.fontSize) + 'px'
            };
          }
        },
        mounted() {
          // 函数级注释：初始化柱状条渲染与交互（统一 Pointer 事件，优化移动端）
          const bar = this.$el.querySelector('#bar');
          const fill = this.$el.querySelector('#bar-fill');
          const valEl = this.$el.querySelector('#bar-value');
          const draw = () => {
            const range = Math.max(0.0001, (this.max - this.min));
            const r = Math.max(0, Math.min(1, (this.value - this.min) / range));
            fill.style.width = (r * 100).toFixed(2) + '%';
            if (valEl) valEl.textContent = String(Math.round(this.value));
          };
          draw();
          let dragging = false;
          const toVal = (evt) => {
            const rect = bar.getBoundingClientRect();
            const x = evt.clientX - rect.left;
            const w = rect.width;
            const r = Math.max(0, Math.min(1, x / Math.max(1, w)));
            const v = this.min + r * (this.max - this.min);
            return Math.round(v); // 整数滑块
          };
          // 函数级注释：Pointer 事件绑定，避免页面滚动与误拖拽
          const onPointerDown = (e) => { dragging = true; try { bar.setPointerCapture(e.pointerId); } catch {} };
          const onPointerUp = (e) => { dragging = false; try { bar.releasePointerCapture(e.pointerId); } catch {} };
          const onPointerMove = (e) => { if (!dragging) return; e.preventDefault(); this.value = toVal(e); draw(); };
          bar.addEventListener('pointerdown', onPointerDown);
          bar.addEventListener('pointerup', onPointerUp);
          bar.addEventListener('pointercancel', onPointerUp);
          bar.addEventListener('pointermove', onPointerMove);
          this.$watch('value', draw);
          this.$watch('min', draw);
          this.$watch('max', draw);
        },
        watch: {
          // 函数级注释：滑块值变化即发送命令
          value(nv) {
            const addr = this.commandId || '/cmd/demo';
            window.EPWidgets.sendCommand(addr, String(nv));
          }
        }
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      
      window.EPWidgets.applyCommonStyle(node, initialProps || {});

      // 2. Vue 就绪后，覆盖注册
      registerNode(node, '滑块', window.EPWidgets.createPropsApi({
        node,
        initialProps,
        vm,
        defaults,
        coercers,
        valueMapper(value) { return { value: Number(value) }; }
      }));
    }).catch(err => {
      mountNode.innerHTML = `<div style="color:red;font-size:12px;">加载模板失败: ${err}</div>`;
    });

    return node;
  };
})();
