// 函数级注释：创建EP Frame 控件（四边形边框/背景/阴影）；通过更低层级显示在普通控件之下，用于做布局底板
(function () {
  window.EPWidgets = window.EPWidgets || {};

  // 函数级注释：创建 Frame 控件；支持背景颜色、边框样式、圆角、阴影（box-shadow）
  window.EPWidgets.createEPFrameWidget = function (grid, initialProps = {}, opts = {}) {
    const defaults = {
      bgColor: 'transparent',
      borderColor: '#e5e7eb',
      borderStyle: 'solid',
      radius: 10,
      shadow: 'none',
      label: '',
      labelColor: '#111827',
      labelBgColor: '#f8fafc',
      labelFontSize: 14,
      labelFontWeight: 500,
      labelOffsetX: 12,
      labelOffsetY: 0
    };

    const coercers = {
      // 函数级注释：圆角强制为数值并做范围钳制
      radius: (v) => {
        const n = Number(v);
        if (!Number.isFinite(n)) return defaults.radius;
        return Math.max(0, Math.min(60, Math.round(n)));
      },
      // 函数级注释：阴影为 CSS box-shadow 字符串（允许 none）
      shadow: (v) => String(v ?? defaults.shadow),
      // 函数级注释：标签字号为数值并做范围钳制
      labelFontSize: (v) => {
        const n = Number(v);
        if (!Number.isFinite(n)) return defaults.labelFontSize;
        return Math.max(10, Math.min(64, Math.round(n)));
      },
      // 函数级注释：标签偏移为数值并做范围钳制
      labelOffsetX: (v) => {
        const n = Number(v);
        if (!Number.isFinite(n)) return defaults.labelOffsetX;
        return Math.max(-2000, Math.min(2000, Math.round(n)));
      },
      // 函数级注释：标签偏移为数值并做范围钳制
      labelOffsetY: (v) => {
        const n = Number(v);
        if (!Number.isFinite(n)) return defaults.labelOffsetY;
        return Math.max(-2000, Math.min(2000, Math.round(n)));
      },
      // 函数级注释：标签字重做白名单限制
      labelFontWeight: (v) => {
        const s0 = String(v ?? defaults.labelFontWeight).trim();
        const n = Number(s0);
        const s = Number.isFinite(n) ? String(Math.round(n)) : s0;
        const ok = new Set(['300', '400', '500', '600', '700', '800', '900']);
        return ok.has(s) ? s : String(defaults.labelFontWeight);
      }
    };

    const node = window.EPWidgets.createVueWidget(grid, {
      type: 'Frame',
      templatePath: 'widgets/Frame/widget.html',
      initialProps,
      opts,
      defaultW: 28,
      defaultH: 16,
      defaults,
      coercers,
      // 函数级注释：创建 Vue App，渲染 Frame 外观（背景/边框/圆角/阴影）
      appFactory(template) {
        return {
          template,
          data() {
            return {
              bgColor: initialProps.bgColor ?? defaults.bgColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              radius: initialProps.radius ?? defaults.radius,
              shadow: initialProps.shadow ?? defaults.shadow,
              label: initialProps.label ?? defaults.label,
              labelColor: initialProps.labelColor ?? defaults.labelColor,
              labelBgColor: initialProps.labelBgColor ?? defaults.labelBgColor,
              labelFontSize: initialProps.labelFontSize ?? defaults.labelFontSize,
              labelFontWeight: initialProps.labelFontWeight ?? defaults.labelFontWeight,
              labelOffsetX: initialProps.labelOffsetX ?? defaults.labelOffsetX,
              labelOffsetY: initialProps.labelOffsetY ?? defaults.labelOffsetY
            };
          },
          computed: {
            // 函数级注释：根容器样式（允许标题溢出显示）
            rootStyle() {
              return {
                width: '100%',
                height: '100%',
                position: 'relative',
                overflow: 'visible'
              };
            },
            // 函数级注释：Frame 样式（四边形边框 + 背景 + 阴影）
            frameStyle() {
              const r = Number(this.radius);
              const rr = Number.isFinite(r) ? r : defaults.radius;
              const sh = String(this.shadow || '').trim() || 'none';
              return {
                width: '100%',
                height: '100%',
                boxSizing: 'border-box',
                border: `1px ${this.borderStyle} ${this.borderColor}`,
                borderRadius: rr + 'px',
                backgroundColor: this.bgColor,
                boxShadow: sh,
                overflow: 'hidden'
              };
            },
            // 函数级注释：标题标签样式（遮住边框形成“缺口”效果）
            labelStyle() {
              const text = (this.label !== undefined && this.label !== null) ? String(this.label).trim() : '';
              if (!text) return { display: 'none' };
              const fs0 = Number(this.labelFontSize);
              const fs = Number.isFinite(fs0) ? Math.max(10, Math.min(64, Math.round(fs0))) : defaults.labelFontSize;
              const fw0 = String(this.labelFontWeight ?? defaults.labelFontWeight).trim();
              const fw = fw0 ? fw0 : String(defaults.labelFontWeight);
              const ox0 = Number(this.labelOffsetX);
              const oy0 = Number(this.labelOffsetY);
              const ox = Number.isFinite(ox0) ? Math.max(-2000, Math.min(2000, Math.round(ox0))) : defaults.labelOffsetX;
              const oy = Number.isFinite(oy0) ? Math.max(-2000, Math.min(2000, Math.round(oy0))) : defaults.labelOffsetY;
              const bg = String(this.labelBgColor || defaults.labelBgColor).trim() || defaults.labelBgColor;
              const color = String(this.labelColor || defaults.labelColor).trim() || defaults.labelColor;
              return {
                position: 'absolute',
                top: '0px',
                left: '0px',
                transform: `translate(${ox}px, ${oy}px) translateY(-50%)`,
                fontSize: fs + 'px',
                fontWeight: fw,
                color: color,
                backgroundColor: bg,
                padding: '2px 8px',
                borderRadius: '6px',
                lineHeight: '1.2',
                whiteSpace: 'nowrap',
                pointerEvents: 'none'
              };
            }
          }
        };
      }
    });

    // 函数级注释：标记为 Frame，并设置较低 z-index，保证位于普通控件之下
    try { node.classList.add('ns-frame'); } catch {}
    try { node.style.zIndex = '0'; } catch {}

    return node;
  };

  // 函数级注释：兼容旧名称（若历史布局中存在 createEPCardWidget 的调用）
  window.EPWidgets.createEPCardWidget = window.EPWidgets.createEPFrameWidget;
})();