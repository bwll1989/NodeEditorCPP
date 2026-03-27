// 函数级注释：创建EP数值控件（支持整型/浮点，输入或只显示）
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPNumberWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/number',
      bgColor: 'transparent',
      fontSize: '14',
      value: 0,
      min: 0,
      max: 100,
      step: 1,
      precision: 0,
      readOnly: false,
      textColor: '#111827',
      borderColor: '#e5e7eb',
      borderStyle: 'none'
    };

    // 函数级注释：将任意输入映射为数值相关属性
    function mapValue(v) {
      let num = Number(v);
      if (Number.isNaN(num)) num = 0;
      return { value: num };
    }

    // 函数级注释：字段类型矫正器（保证为数值/布尔）
    const coercers = {
      value(v){ const n = Number(v); return Number.isNaN(n) ? defaults.value : n; },
      min(v){ const n = Number(v); return Number.isNaN(n) ? defaults.min : n; },
      max(v){ const n = Number(v); return Number.isNaN(n) ? defaults.max : n; },
      step(v){ const n = Number(v); return Number.isNaN(n) ? defaults.step : n; },
      precision(v){ const n = Number(v); return Number.isNaN(n) ? defaults.precision : Math.max(0, Math.floor(n)); },
      readOnly(v){ return !!window.EPWidgets.toBool(v); }
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '数值',
      templatePath: 'widgets/Number/widget.html',
      initialProps,
      opts,
      defaultW: 10,
      defaultH: 2,
      defaults,
      valueMapper: mapValue,
      coercers,
      // 函数级注释：Vue应用工厂，定义数据/样式/联动逻辑
      appFactory(template) {
        return {
          template,
          data() {
            return {
              value: initialProps.value ?? defaults.value,
              min: initialProps.min ?? defaults.min,
              max: initialProps.max ?? defaults.max,
              step: initialProps.step ?? defaults.step,
              precision: initialProps.precision ?? defaults.precision,
              readOnly: !!(initialProps.readOnly ?? defaults.readOnly),
              commandId: initialProps.commandId ?? defaults.commandId,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              bgColor: initialProps.bgColor ?? defaults.bgColor
            };
          },
          watch: {
            // 函数级注释：数值变化即发送命令（只读模式不发送；按精度取舍）
            value(nv) {
              if (this.readOnly) return;
              const addr = this.commandId || defaults.commandId;
              let num = Number(nv);
              if (Number.isNaN(num)) num = 0;
              if (this.precision > 0) num = Number(num.toFixed(this.precision));
              else num = Math.round(num);
              // 范围约束
              if (this.min !== undefined) num = Math.max(this.min, num);
              if (this.max !== undefined) num = Math.min(this.max, num);
              window.EPWidgets.sendCommand(addr, String(num));
            },
            // 函数级注释：精度变化时重整当前值
            precision(nv) {
              const p = Math.max(0, Math.floor(Number(nv)||0));
              let num = Number(this.value);
              if (Number.isNaN(num)) num = 0;
              this.value = p > 0 ? Number(num.toFixed(p)) : Math.round(num);
            },
            // 函数级注释：范围变化时约束当前值
            min(nv) {
              const m = Number(nv);
              if (!Number.isNaN(m)) this.value = Math.max(m, Number(this.value)||0);
            },
            max(nv) {
              const m = Number(nv);
              if (!Number.isNaN(m)) this.value = Math.min(m, Number(this.value)||0);
            }
          },
          computed: {
            // 函数级注释：输入框样式（文本/边框/背景颜色统一覆盖 Element Plus 变量）
            inputStyle() {
              return {
                width: '100%',
                height: '100%',
                boxSizing: 'border-box',
                color: this.textColor,
                borderColor: this.borderColor,
                borderStyle: this.borderStyle,
                backgroundColor: this.bgColor,
                '--el-input-border-color': this.borderColor,
                '--el-input-hover-border-color': this.borderColor,
                '--el-input-focus-border-color': this.borderColor,
                '--el-fill-color-blank': this.bgColor,
                '--el-input-bg-color': this.bgColor,
                '--el-color-white': this.bgColor
              };
            }
          }
        };
      }
    });
  };
})();