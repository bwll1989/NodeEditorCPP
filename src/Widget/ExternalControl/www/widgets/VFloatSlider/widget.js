// 函数级注释：创建EP竖向浮点滑块控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPVFloatSliderWidget = function(grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: 'transparent',
      fontSize: '14',
      min: 0,
      max: 1,
      step: 0.01,
      value: 0,
      height: '100%',
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
      step: (v) => Number(v),
      value: (v) => Number(v),
      height: (v) => String(v ?? '100%')
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '竖向浮点滑块',
      templatePath: 'widgets/VFloatSlider/widget.html',
      initialProps,
      opts,
      defaultW: 4,
      defaultH: 10,
      defaults,
      coercers,
      valueMapper(value) { return { value: Number(value) }; },
      appFactory(template) {
        return {
          template,
          data() {
            return {
              min: initialProps.min ?? defaults.min,
              max: initialProps.max ?? defaults.max,
              step: initialProps.step ?? defaults.step,
              value: initialProps.value ?? defaults.value,
              height: initialProps.height ?? defaults.height,
              commandId: initialProps.commandId ?? defaults.commandId,
              barColor: initialProps.barColor ?? defaults.barColor,
              fillColor: initialProps.fillColor ?? defaults.fillColor,
              valueColor: initialProps.valueColor ?? defaults.valueColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              fontSize: initialProps.fontSize ?? defaults.fontSize
            };
          },
          computed: {
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
                bottom: '0',
                width: '100%',
                height: '0',
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
                fontSize: this.fontSize + 'px'
              };
            }
          },
          mounted() {
            const bar = this.$el.querySelector('#bar');
            const fill = this.$el.querySelector('#bar-fill');
            const valEl = this.$el.querySelector('#bar-value');
            const quantize = (v) => {
              const step = Math.max(0.000001, this.step || 0.01);
              const q = Math.round((v - this.min) / step) * step + this.min;
              return Number(Math.max(this.min, Math.min(this.max, q)).toFixed(6));
            };
            const applyHeight = () => { bar.style.height = String(this.height || '100%'); };
            const draw = () => {
              const range = Math.max(0.0001, (this.max - this.min));
              const r = Math.max(0, Math.min(1, (this.value - this.min) / range));
              fill.style.height = (r * 100).toFixed(2) + '%';
              if (valEl) {
                const step = Math.max(0.000001, this.step || 0.01);
                const decimals = step < 1 ? 2 : 0;
                valEl.textContent = decimals ? Number(this.value).toFixed(decimals) : String(Math.round(this.value));
              }
            };
            applyHeight();
            draw();
            let dragging = false;
            const toVal = (evt) => {
              const rect = bar.getBoundingClientRect();
              const y = evt.clientY - rect.top;
              const h = rect.height;
              const r = Math.max(0, Math.min(1, 1 - (y / Math.max(1, h))));
              return quantize(this.min + r * (this.max - this.min));
            };
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
            this.$watch('step', draw);
            this.$watch('height', () => { applyHeight(); draw(); });
          },
          watch: {
            value(nv) {
              window.EPWidgets.sendCommand(this.commandId || defaults.commandId, String(nv));
            }
          }
        };
      }
    });
  };
})();
