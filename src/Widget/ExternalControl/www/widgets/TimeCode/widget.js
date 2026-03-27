// 函数级注释：创建EP时间码（Timecode）控件；将帧数 value 按 fps 格式化为 hh:mm:ss:ff，并在本地修改后发送帧数
(function () {
  window.EPWidgets = window.EPWidgets || {};

  // 函数级注释：创建时间码控件（默认 25fps），支持 +/- 调整 h/m/s/f 并发送总帧数
  window.EPWidgets.createEPTimecodeWidget = function (grid, initialProps = {}, opts = {}) {
    const defaults = {
      commandId: '/cmd/demo',
      bgColor: 'transparent',
      fontSize: '30',
      textColor: '#111827',
      borderColor: '#e5e7eb',
      borderStyle: 'none',
      fps: 25,
      value: 0,
      readOnly: false,
      btnBgColor: '#f8fafc',
      btnBorderColor: '#cbd5e1',
      btnTextColor: '#111827'
    };

    const coercers = {
      // 函数级注释：确保字号以字符串保存（兼容现有属性体系）
      fontSize: (v) => String(v ?? defaults.fontSize),
      // 函数级注释：将 fps/value 强制为数值，避免远程更新字符串导致显示异常
      fps: (v) => Number(v),
      value: (v) => Number(v),
      readOnly: (v) => window.EPWidgets.toBool(v)
    };

    return window.EPWidgets.createVueWidget(grid, {
      type: '时间码',
      templatePath: 'widgets/Timecode/widget.html',
      initialProps,
      opts,
      defaultW: 28,
      defaultH: 10,
      defaults,
      coercers,
      // 函数级注释：远程 value 映射到本控件帧数
      valueMapper(value) {
        return { value: Number(value) };
      },
      // 函数级注释：创建 Vue App，负责 timecode 渲染与交互
      appFactory(template) {
        return {
          template,
          data() {
            return {
              value: Number(initialProps.value ?? defaults.value),
              fps: Number(initialProps.fps ?? defaults.fps),
              commandId: initialProps.commandId ?? defaults.commandId,
              fontSize: initialProps.fontSize ?? defaults.fontSize,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              readOnly: initialProps.readOnly ?? defaults.readOnly,
              btnBgColor: initialProps.btnBgColor ?? defaults.btnBgColor,
              btnBorderColor: initialProps.btnBorderColor ?? defaults.btnBorderColor,
              btnTextColor: initialProps.btnTextColor ?? defaults.btnTextColor,
              ui: {
                pad: 10,
                gap: 10,
                btnH: 46,
                btnFont: 22,
                btnRadius: 6,
                displayMargin: 6
              },
              __ro: null,
              __onWinResize: null
            };
          },
          computed: {
            // 函数级注释：对 fps 做安全归一化（最小 1）
            normFps() {
              const f = Number(this.fps);
              if (!Number.isFinite(f)) return 25;
              return Math.max(1, Math.floor(f));
            },
            // 函数级注释：将总帧数拆分为 hh/mm/ss/ff
            parts() {
              const fps = this.normFps;
              const total = Math.max(0, Math.floor(Number(this.value) || 0));

              const perHour = fps * 3600;
              const perMin = fps * 60;

              const h = Math.floor(total / perHour);
              const remH = total % perHour;

              const m = Math.floor(remH / perMin);
              const remM = remH % perMin;

              const s = Math.floor(remM / fps);
              const f = remM % fps;

              return { h, m, s, f };
            },
            // 函数级注释：渲染 timecode 字符串（hh:mm:ss:ff；小时至少两位但不截断）
            timecodeText() {
              const pad2 = (n) => String(Math.max(0, Math.floor(n))).padStart(2, '0');
              const { h, m, s, f } = this.parts;
              const hh = String(Math.max(0, Math.floor(h))).padStart(2, '0');
              return `${hh}:${pad2(m)}:${pad2(s)}:${pad2(f)}`;
            },
            // 函数级注释：整体容器样式（接近示例图：上下按钮 + 中间大字）
            containerStyle() {
              const pad = (this.ui && typeof this.ui.pad === 'number') ? this.ui.pad : 10;
              const radius = Math.max(6, Math.min(10, pad));
              return {
                width: '100%',
                height: '100%',
                display: 'flex',
                flexDirection: 'column',
                alignItems: 'stretch',
                justifyContent: 'space-between',
                padding: pad + 'px',
                borderRadius: radius + 'px',
                borderColor: this.borderColor,
                borderStyle: this.borderStyle,
                borderWidth: '1px',
                boxSizing: 'border-box',
                background: 'transparent'
              };
            },
            // 函数级注释：按钮行样式（4 列）
            rowStyle() {
              const gap = (this.ui && typeof this.ui.gap === 'number') ? this.ui.gap : 10;
              return {
                display: 'grid',
                gridTemplateColumns: 'repeat(4, 1fr)',
                gap: gap + 'px'
              };
            },
            // 函数级注释：按钮样式
            buttonStyle() {
              const btnH = (this.ui && typeof this.ui.btnH === 'number') ? this.ui.btnH : 46;
              const btnFont = (this.ui && typeof this.ui.btnFont === 'number') ? this.ui.btnFont : 22;
              const btnRadius = (this.ui && typeof this.ui.btnRadius === 'number') ? this.ui.btnRadius : 6;
              return {
                height: btnH + 'px',
                borderRadius: btnRadius + 'px',
                border: `1px solid ${this.btnBorderColor}`,
                background: this.btnBgColor,
                color: this.btnTextColor,
                fontSize: btnFont + 'px',
                fontWeight: 700,
                lineHeight: '1',
                cursor: 'pointer',
                userSelect: 'none',
                padding: '0',
                touchAction: 'manipulation'
              };
            },
            // 函数级注释：中间显示样式（大号 timecode）
            displayStyle() {
              const fs = Number(this.fontSize);
              const fontPx = Number.isFinite(fs) ? fs : 64;
              const m = (this.ui && typeof this.ui.displayMargin === 'number') ? this.ui.displayMargin : 6;
              return {
                flex: '1',
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                color: this.textColor,
                fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace',
                fontSize: `${fontPx}px`,
                fontWeight: 700,
                letterSpacing: '1px',
                lineHeight: '1.05',
                margin: m + 'px 0'
              };
            }
          },
          mounted() {
            // 函数级注释：根据控件实际尺寸自适应缩放按钮（支持缩小以适配更小的控件尺寸）
            this.recalcUi();
            this.__onWinResize = () => { try { this.recalcUi(); } catch {} };
            try { window.addEventListener('resize', this.__onWinResize); } catch {}

            try {
              if (typeof ResizeObserver !== 'undefined') {
                this.__ro = new ResizeObserver(() => { try { this.recalcUi(); } catch {} });
                if (this.$el) this.__ro.observe(this.$el);
              }
            } catch {}
          },
          beforeUnmount() {
            // 函数级注释：清理自适应监听，避免内存泄漏
            try {
              if (this.__ro) {
                try { this.__ro.disconnect(); } catch {}
                this.__ro = null;
              }
            } catch {}
            try {
              if (this.__onWinResize) {
                window.removeEventListener('resize', this.__onWinResize);
                this.__onWinResize = null;
              }
            } catch {}
          },
          methods: {
            // 函数级注释：根据当前控件尺寸重新计算 UI 尺寸参数（padding/gap/按钮高度/按钮字号/圆角）
            recalcUi() {
              const el = this.$el;
              if (!el) return;
              const h = Math.max(0, Number(el.clientHeight) || 0);
              const clamp = (min, v, max) => Math.max(min, Math.min(max, v));

              const pad = Math.round(clamp(6, h * 0.06, 10));
              const btnH = Math.round(clamp(20, h * 0.16, 36));
              const gap = Math.round(clamp(4, h * 0.045, 10));
              const btnFont = Math.round(clamp(12, btnH * 0.55, 20));
              const btnRadius = Math.round(clamp(4, btnH * 0.16, 8));
              const displayMargin = Math.round(clamp(4, h * 0.03, 8));

              this.ui = Object.assign({}, this.ui || {}, { pad, gap, btnH, btnFont, btnRadius, displayMargin });
            },
            // 函数级注释：将帧数写回并做整数/非负钳制
            setFrames(v) {
              const nv = Math.max(0, Math.floor(Number(v) || 0));
              this.value = nv;
            },
            // 函数级注释：根据字段计算步进帧数（h/m/s/f）
            stepFrames(unit) {
              const fps = this.normFps;
              if (unit === 'h') return fps * 3600;
              if (unit === 'm') return fps * 60;
              if (unit === 's') return fps;
              return 1;
            },
            // 函数级注释：递增指定字段（自动进位）
            inc(unit) {
              if (this.readOnly) return;
              const delta = this.stepFrames(unit);
              this.setFrames((Number(this.value) || 0) + delta);
            },
            // 函数级注释：递减指定字段（自动借位；下限为 0）
            dec(unit) {
              if (this.readOnly) return;
              const delta = this.stepFrames(unit);
              this.setFrames((Number(this.value) || 0) - delta);
            }
          },
          watch: {
            // 函数级注释：帧数变化即发送命令（远程更新会被 EPWidgets.sendCommand 自动抑制）
            value(nv) {
              const addr = this.commandId || defaults.commandId;
              const frames = Math.max(0, Math.floor(Number(nv) || 0));
              window.EPWidgets.sendCommand(addr, String(frames));
            },
            // 函数级注释：fps 变化时，确保当前 value 仍为合法整数（避免 NaN）
            fps() {
              this.setFrames(this.value);
            }
          }
        };
      }
    });
  };
})();