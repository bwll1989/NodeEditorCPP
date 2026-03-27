// 函数级注释：创建EP旋钮控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPKnobWidget = function(grid, initialProps = {}, opts = {}) {
    const { getVue, registerNode, createContainer, loadTemplate } = window.EPWidgets;
    const vue = getVue();
    const ready = !!vue && !!window.ElementPlus;
    const defaultTitle = initialProps.title || '旋钮';
    // 默认 4x4 大小 (GridStack 48列模式下)
    const { node, mountNode } = createContainer(grid, defaultTitle, 4, 4, opts);
    
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: 'transparent',
      fontSize: '14',
      min: 0,
      max: 100,
      value: 0,
      step: 1,
      valueColor: '#3B82F6',
      rangeColor: '#dfe7ef',
      strokeWidth: 14,
      showValue: true,
      textColor: '#495057',
      borderColor: '#dfe7ef',
      borderStyle: 'none'
    };
    const coercers = {
      fontSize: (v) => String(v ?? '14'),
      min: (v) => Number(v),
      max: (v) => Number(v),
      value: (v) => Number(v),
      step: (v) => Number(v),
      strokeWidth: (v) => Number(v),
      showValue: (v) => window.EPWidgets.toBool(v)
    };

    // 1. 同步注册基础 API
    registerNode(node, '旋钮', window.EPWidgets.createPropsApi({
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

    loadTemplate('widgets/Knob/widget.html').then(template => {
      const app = vue.createApp({
        template: template,
        data() {
          return { 
              min: initialProps.min ?? 0, 
              max: initialProps.max ?? 100, 
              value: initialProps.value ?? 0,
              step: initialProps.step ?? 1,
              commandId: initialProps.commandId || '/cmd/demo',
              valueColor: initialProps.valueColor || '#3B82F6',
              rangeColor: initialProps.rangeColor || '#dfe7ef',
              strokeWidth: initialProps.strokeWidth || 14,
              showValue: initialProps.showValue ?? true,
              textColor: initialProps.textColor || '#495057',
              borderColor: initialProps.borderColor || '#dfe7ef',
              borderStyle: initialProps.borderStyle || 'solid'
          };
        },
        computed: {
            containerStyle() {
                return {
                    width: '100%',
                    height: '100%',
                    display: 'flex',
                    justifyContent: 'center',
                    alignItems: 'center',
                    borderColor: this.borderColor,
                    borderStyle: this.borderStyle
                };
            },
            rangePath() {
                return this.describeArc(50, 50, 40, -145, 145);
            },
            valuePath() {
                const angle = this.mapRange(this.value, this.min, this.max, -145, 145);
                return this.describeArc(50, 50, 40, -145, angle);
            },
            valueText() {
                return Math.round(this.value);
            }
        },
        methods: {
            polarToCartesian(centerX, centerY, radius, angleInDegrees) {
                var angleInRadians = (angleInDegrees - 90) * Math.PI / 180.0;
                return {
                    x: centerX + (radius * Math.cos(angleInRadians)),
                    y: centerY + (radius * Math.sin(angleInRadians))
                };
            },
            describeArc(x, y, radius, startAngle, endAngle) {
                var start = this.polarToCartesian(x, y, radius, endAngle);
                var end = this.polarToCartesian(x, y, radius, startAngle);
                var largeArcFlag = endAngle - startAngle <= 180 ? "0" : "1";
                var d = [
                    "M", start.x, start.y,
                    "A", radius, radius, 0, largeArcFlag, 0, end.x, end.y
                ].join(" ");
                return d;
            },
            mapRange(x, in_min, in_max, out_min, out_max) {
                return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
            },
            updateValue(event, touch) {
                const element = this.$el.querySelector('svg');
                if (!element) return;
                const rect = element.getBoundingClientRect();
                const clientX = touch ? touch.clientX : event.clientX;
                const clientY = touch ? touch.clientY : event.clientY;
                
                const x = clientX - (rect.left + rect.width / 2);
                const y = clientY - (rect.top + rect.height / 2);
                
                let angle = Math.atan2(y, x) * 180 / Math.PI;
                let deg = angle + 90; 
                
                if (deg > 180) deg -= 360; 
                
                if (deg < -145) deg = -145;
                if (deg > 145) deg = 145;
                
                const val = this.mapRange(deg, -145, 145, this.min, this.max);
                
                const stepped = Math.round(val / this.step) * this.step;
                this.value = Math.min(this.max, Math.max(this.min, stepped));
                
                const addr = this.commandId || '/cmd/demo';
                window.EPWidgets.sendCommand(addr, String(this.value));
            },
            onMouseDown(event) {
                this.updateValue(event);
                const onMouseMove = (e) => {
                    e.preventDefault();
                    this.updateValue(e);
                };
                const onMouseUp = () => {
                    window.removeEventListener('mousemove', onMouseMove);
                    window.removeEventListener('mouseup', onMouseUp);
                };
                window.addEventListener('mousemove', onMouseMove);
                window.addEventListener('mouseup', onMouseUp);
            },
            onTouchStart(event) {
                if (event.touches.length === 1) {
                    this.updateValue(null, event.touches[0]);
                    const onTouchMove = (e) => {
                         if (e.touches.length === 1) {
                            e.preventDefault(); 
                            this.updateValue(null, e.touches[0]);
                         }
                    };
                    const onTouchEnd = () => {
                        window.removeEventListener('touchmove', onTouchMove);
                        window.removeEventListener('touchend', onTouchEnd);
                    };
                    window.addEventListener('touchmove', onTouchMove, { passive: false });
                    window.addEventListener('touchend', onTouchEnd);
                }
            }
        }
      });
      app.use(window.ElementPlus);
      const vm = app.mount(mountNode);
      node.vm = vm; // store view model instance

      window.EPWidgets.applyCommonStyle(node, initialProps || {});
      registerNode(node, '旋钮', window.EPWidgets.createPropsApi({
        node,
        initialProps,
        vm,
        defaults,
        coercers,
        valueMapper(value) { return { value: Number(value) }; }
      }));
    });

    return node;
  };
})();
