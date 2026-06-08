// 函数级注释：创建EP旋钮控件
(function(){
  window.EPWidgets = window.EPWidgets || {};
  window.EPWidgets.createEPKnobWidget = function(grid, initialProps = {}, opts = {}) {
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

    return window.EPWidgets.createVueWidget(grid, {
      type: '旋钮',
      templatePath: 'widgets/Knob/widget.html',
      initialProps,
      opts,
      defaultW: 4,
      defaultH: 4,
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
              value: initialProps.value ?? defaults.value,
              step: initialProps.step ?? defaults.step,
              commandId: initialProps.commandId ?? defaults.commandId,
              valueColor: initialProps.valueColor ?? defaults.valueColor,
              rangeColor: initialProps.rangeColor ?? defaults.rangeColor,
              strokeWidth: initialProps.strokeWidth ?? defaults.strokeWidth,
              showValue: initialProps.showValue ?? defaults.showValue,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle
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
              const angleInRadians = (angleInDegrees - 90) * Math.PI / 180.0;
              return {
                x: centerX + (radius * Math.cos(angleInRadians)),
                y: centerY + (radius * Math.sin(angleInRadians))
              };
            },
            describeArc(x, y, radius, startAngle, endAngle) {
              const start = this.polarToCartesian(x, y, radius, endAngle);
              const end = this.polarToCartesian(x, y, radius, startAngle);
              const largeArcFlag = endAngle - startAngle <= 180 ? '0' : '1';
              return ['M', start.x, start.y, 'A', radius, radius, 0, largeArcFlag, 0, end.x, end.y].join(' ');
            },
            mapRange(x, inMin, inMax, outMin, outMax) {
              return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
            },
            updateValue(event, touch) {
              const element = this.$el.querySelector('svg');
              if (!element) return;
              const rect = element.getBoundingClientRect();
              const clientX = touch ? touch.clientX : event.clientX;
              const clientY = touch ? touch.clientY : event.clientY;
              const x = clientX - (rect.left + rect.width / 2);
              const y = clientY - (rect.top + rect.height / 2);
              let deg = Math.atan2(y, x) * 180 / Math.PI + 90;
              if (deg > 180) deg -= 360;
              if (deg < -145) deg = -145;
              if (deg > 145) deg = 145;
              const val = this.mapRange(deg, -145, 145, this.min, this.max);
              const stepped = Math.round(val / this.step) * this.step;
              this.value = Math.min(this.max, Math.max(this.min, stepped));
              window.EPWidgets.sendCommand(this.commandId || defaults.commandId, String(this.value));
            },
            onMouseDown(event) {
              this.updateValue(event);
              const onMouseMove = (e) => { e.preventDefault(); this.updateValue(e); };
              const onMouseUp = () => {
                window.removeEventListener('mousemove', onMouseMove);
                window.removeEventListener('mouseup', onMouseUp);
              };
              window.addEventListener('mousemove', onMouseMove);
              window.addEventListener('mouseup', onMouseUp);
            },
            onTouchStart(event) {
              if (event.touches.length !== 1) return;
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
        };
      }
    });
  };
})();
