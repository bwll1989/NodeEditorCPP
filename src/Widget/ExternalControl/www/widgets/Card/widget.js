// 函数级注释：创建EP卡片控件（作为容器背景使用）；卡片本身置底显示，拖入卡片范围的控件会自动归到同一 groupId，从而实现“卡片内控件随卡片一起移动”
(function () {
  window.EPWidgets = window.EPWidgets || {};

  // 函数级注释：创建卡片控件；卡片用于承载多个控件形成一个视觉卡片区域（靠 groupId 实现“随卡片一起移动”）
  window.EPWidgets.createEPCardWidget = function (grid, initialProps = {}, opts = {}) {
    const defaults = {
      bgColor: '#ffffff',
      fontSize: '14',
      title: '',
      textColor: '#111827',
      borderColor: '#e5e7eb',
      borderStyle: 'solid',
      radius: 10
    };

    const coercers = {
      // 函数级注释：确保字号以字符串保存（兼容现有属性体系）
      fontSize: (v) => String(v ?? defaults.fontSize),
      // 函数级注释：圆角强制为数值并做范围钳制
      radius: (v) => {
        const n = Number(v);
        if (!Number.isFinite(n)) return defaults.radius;
        return Math.max(0, Math.min(30, Math.round(n)));
      }
    };

    const node = window.EPWidgets.createVueWidget(grid, {
      type: '卡片',
      templatePath: 'widgets/Card/widget.html',
      initialProps,
      opts,
      defaultW: 28,
      defaultH: 16,
      defaults,
      coercers,
      // 函数级注释：创建 Vue App，渲染卡片外观（背景/边框/圆角/可选标题）
      appFactory(template) {
        return {
          template,
          data() {
            return {
              bgColor: initialProps.bgColor ?? defaults.bgColor,
              fontSize: initialProps.fontSize ?? defaults.fontSize,
              title: initialProps.title ?? defaults.title,
              textColor: initialProps.textColor ?? defaults.textColor,
              borderColor: initialProps.borderColor ?? defaults.borderColor,
              borderStyle: initialProps.borderStyle ?? defaults.borderStyle,
              radius: (initialProps.radius ?? defaults.radius)
            };
          },
          computed: {
            // 函数级注释：卡片整体样式（置底背景区域）
            cardStyle() {
              const r = Number(this.radius);
              const rr = Number.isFinite(r) ? r : defaults.radius;
              return {
                width: '100%',
                height: '100%',
                boxSizing: 'border-box',
                border: `1px ${this.borderStyle} ${this.borderColor}`,
                borderRadius: rr + 'px',
                backgroundColor: this.bgColor,
                overflow: 'hidden',
                display: 'flex',
                flexDirection: 'column'
              };
            },
            // 函数级注释：标题样式（可选）
            titleStyle() {
              const fs = Number(this.fontSize);
              const fontPx = Number.isFinite(fs) ? fs : 14;
              return {
                padding: '8px 10px',
                color: this.textColor,
                fontSize: fontPx + 'px',
                fontWeight: 600,
                borderBottom: '1px solid rgba(0,0,0,0.06)',
                background: 'transparent',
                userSelect: 'none',
                whiteSpace: 'nowrap',
                overflow: 'hidden',
                textOverflow: 'ellipsis'
              };
            }
          }
        };
      }
    });

    // 函数级注释：标记卡片控件并确保其拥有稳定的 groupId（用于承载其它控件）
    try { node.classList.add('ns-card'); } catch {}
    try { node.style.zIndex = '0'; } catch {}
    try {
      const cur = String((node && node.dataset ? node.dataset.groupId : '') || '').trim();
      if (!cur) {
        const gid = 'c_' + Math.random().toString(36).slice(2, 8) + Date.now().toString(36).slice(-4);
        node.dataset.groupId = gid;
      }
    } catch {}

    return node;
  };
})();