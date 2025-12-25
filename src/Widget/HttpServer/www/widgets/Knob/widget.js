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
    
    // 1. 同步注册基础 API
    registerNode(node, '旋钮', {
      getProps(){ return { 
          commandId: initialProps.commandId || '/cmd/demo', 
          min: initialProps.min ?? 0, 
          max: initialProps.max ?? 100, 
          value: initialProps.value ?? 0,
          step: initialProps.step ?? 1,
          valueColor: initialProps.valueColor || '#3B82F6',
          rangeColor: initialProps.rangeColor || '#dfe7ef',
          strokeWidth: initialProps.strokeWidth || 14,
          showValue: initialProps.showValue ?? true,
          // 样式属性
          textColor: initialProps.textColor || '#495057',
          borderColor: initialProps.borderColor || '#dfe7ef',
          borderStyle: initialProps.borderStyle || 'solid'
      }; },
      setProps(p){ 
        if (p.commandId!==undefined) initialProps.commandId = p.commandId;
        if (p.min!==undefined) initialProps.min = Number(p.min);
        if (p.max!==undefined) initialProps.max = Number(p.max);
        if (p.value!==undefined) initialProps.value = Number(p.value);
        if (p.step!==undefined) initialProps.step = Number(p.step);
        if (p.valueColor!==undefined) initialProps.valueColor = p.valueColor;
        if (p.rangeColor!==undefined) initialProps.rangeColor = p.rangeColor;
        if (p.strokeWidth!==undefined) initialProps.strokeWidth = Number(p.strokeWidth);
        if (p.showValue!==undefined) initialProps.showValue = !!p.showValue;
        if (p.textColor!==undefined) initialProps.textColor = p.textColor;
        if (p.borderColor!==undefined) initialProps.borderColor = p.borderColor;
        if (p.borderStyle!==undefined) initialProps.borderStyle = p.borderStyle;
        
        // Update Vue instance if it exists
        if (node.vm) {
            if (p.min!==undefined) node.vm.min = Number(p.min);
            if (p.max!==undefined) node.vm.max = Number(p.max);
            if (p.value!==undefined) node.vm.value = Number(p.value);
            if (p.step!==undefined) node.vm.step = Number(p.step);
            if (p.valueColor!==undefined) node.vm.valueColor = p.valueColor;
            if (p.rangeColor!==undefined) node.vm.rangeColor = p.rangeColor;
            if (p.strokeWidth!==undefined) node.vm.strokeWidth = Number(p.strokeWidth);
            if (p.showValue!==undefined) node.vm.showValue = !!p.showValue;
            if (p.commandId!==undefined) node.vm.commandId = p.commandId;
            if (p.textColor!==undefined) node.vm.textColor = p.textColor;
            if (p.borderColor!==undefined) node.vm.borderColor = p.borderColor;
            if (p.borderStyle!==undefined) node.vm.borderStyle = p.borderStyle;
        }
      }
    });

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
      const vm = app.mount(mountNode);
      node.vm = vm; // store view model instance
    });

    return node;
  };
})();
