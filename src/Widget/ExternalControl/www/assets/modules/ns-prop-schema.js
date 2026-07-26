// ns-prop-schema.js —— 属性面板 Schema 定义与通用渲染（替代 updatePropPanel 大部分内联 HTML）
(function() {
  'use strict';

  const BORDER_STYLE_OPTS = ['none', 'solid', 'dashed', 'dotted', 'double'];
  const BUTTON_TYPE_OPTS = [
    { v: 'primary', t: 'primary' },
    { v: 'success', t: 'success' },
    { v: 'warning', t: 'warning' },
    { v: 'danger', t: 'danger' },
    { v: 'info', t: 'info' },
    { v: 'default', t: 'default' }
  ];
  const COMMAND_ID = { key: 'commandId', label: 'Command ID', type: 'text' };

  const LEGACY_PROP_TYPES = new Set([]);

  const CUSTOM_PANEL_TYPES = new Set(['Frame', '时间码', '单选框']);

  const FRAME_SHADOW_PRESETS = [
    { v: 'none', t: '无' },
    { v: '0 4px 12px rgba(0,0,0,0.12)', t: '柔和' },
    { v: '0 10px 30px rgba(0,0,0,0.18)', t: '中等' },
    { v: '0 20px 60px rgba(0,0,0,0.25)', t: '强烈' }
  ];

  const SCHEMA_BY_TYPE = {
    'Trigger 按钮': [
      COMMAND_ID,
      { key: 'label', label: '文本', type: 'text' },
      { key: 'buttonType', label: '类型', type: 'select', options: BUTTON_TYPE_OPTS },
      { key: 'plain', label: '朴素按钮', type: 'checkbox' },
      { key: 'round', label: '圆角按钮', type: 'checkbox' },
      { key: 'buttonColor', label: '按钮颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'activeColor', label: '按下颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'textColor', label: '文字颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'borderColor', label: '边框颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '滑块': [
      COMMAND_ID,
      { key: 'direction', label: '方向', type: 'select', options: [
        { v: 'horizontal', t: '横向' },
        { v: 'vertical', t: '纵向' }
      ]},
      { key: 'min', label: '最小值', type: 'number' },
      { key: 'max', label: '最大值', type: 'number' },
      { key: 'step', label: '步进', type: 'number' },
      { key: 'value', label: '当前值', type: 'number' },
      { key: 'barColor', label: '轨道颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'fillColor', label: '进度颜色', type: 'color', fallback: '#2b6cb0' },
      { key: 'valueColor', label: '数值颜色', type: 'color', fallback: '#111827' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '勾选': [
      COMMAND_ID,
      { key: 'checked', label: '选中', type: 'checkbox' },
      { key: 'activeColor', label: '选中颜色', type: 'color', fallback: '#409EFF' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#94a3b8' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '开关': [
      COMMAND_ID,
      { key: 'checked', label: '选中', type: 'checkbox' },
      { key: 'onColor', label: '开启颜色', type: 'color', fallback: '#13ce66' },
      { key: 'offColor', label: '关闭颜色', type: 'color', fallback: '#cbd5e1' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    'LED': [
      COMMAND_ID,
      // on 仅作编辑态预览/初始态；运行时由 WebSocket 状态只读驱动
      { key: 'on', label: '初始点亮', type: 'checkbox' },
      { key: 'blink', label: '闪烁', type: 'checkbox' },
      { key: 'onColor', label: '点亮颜色', type: 'color', fallback: '#22c55e' },
      { key: 'offColor', label: '熄灭颜色', type: 'color', fallback: '#cbd5e1' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '输入框': [
      COMMAND_ID,
      { key: 'placeholder', label: '占位符', type: 'text' },
      { key: 'value', label: '当前值', type: 'text' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#111827' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '标签': [
      COMMAND_ID,
      { key: 'text', label: '文本', type: 'text' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#334155' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    'Text': [
      COMMAND_ID,
      { key: 'text', label: '文本（支持多行）', type: 'textarea', rows: 5 },
      { key: 'textType', label: '类型', type: 'select', options: [
        { v: '', t: '默认' },
        { v: 'primary', t: 'primary' },
        { v: 'success', t: 'success' },
        { v: 'info', t: 'info' },
        { v: 'warning', t: 'warning' },
        { v: 'danger', t: 'danger' }
      ]},
      { key: 'truncated', label: '单行省略', type: 'checkbox' },
      { key: 'lineClamp', label: '最大行数 (0=不限)', type: 'number' },
      { key: 'align', label: '水平对齐', type: 'select', options: [
        { v: 'left', t: '左' },
        { v: 'center', t: '中' },
        { v: 'right', t: '右' }
      ]},
      { key: 'textColor', label: '文字颜色(覆盖类型色)', type: 'color', fallback: '#111827' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '分割线': [
      { key: 'direction', label: '方向', type: 'select', options: [
        { v: 'horizontal', t: '横向' },
        { v: 'vertical', t: '纵向' }
      ]},
      { key: 'text', label: '文本', type: 'text' },
      { key: 'contentPosition', label: '文本位置', type: 'select', options: [
        { v: 'left', t: '左' },
        { v: 'center', t: '中' },
        { v: 'right', t: '右' }
      ]},
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#909399' },
      { key: 'lineColor', label: '线条颜色', type: 'color', fallback: '#dcdfe6' },
      { key: 'lineWidth', label: '线宽 (px)', type: 'number' },
      { key: 'borderStyle', label: '线条样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '旋钮': [
      COMMAND_ID,
      { key: 'min', label: '最小值', type: 'number' },
      { key: 'max', label: '最大值', type: 'number' },
      { key: 'value', label: '当前值', type: 'number' },
      { key: 'step', label: '步进', type: 'number' },
      { key: 'valueColor', label: '数值弧颜色', type: 'color', fallback: '#3B82F6' },
      { key: 'rangeColor', label: '背景弧颜色', type: 'color', fallback: '#dfe7ef' },
      { key: 'strokeWidth', label: '弧宽', type: 'number' },
      { key: 'showValue', label: '显示数值', type: 'checkbox' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#495057' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#dfe7ef' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '数值': [
      COMMAND_ID,
      { key: 'min', label: '最小值', type: 'number' },
      { key: 'max', label: '最大值', type: 'number' },
      { key: 'step', label: '步进', type: 'number' },
      { key: 'precision', label: '精度 (小数位)', type: 'number' },
      { key: 'value', label: '当前值', type: 'number' },
      { key: 'readOnly', label: '只读', type: 'checkbox' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#111827' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '步进器': [
      COMMAND_ID,
      { key: 'min', label: '最小值', type: 'number' },
      { key: 'max', label: '最大值', type: 'number' },
      { key: 'step', label: '步进', type: 'number' },
      { key: 'precision', label: '精度 (小数位)', type: 'number' },
      { key: 'value', label: '当前值', type: 'number' },
      { key: 'readOnly', label: '只读', type: 'checkbox' },
      { key: 'buttonColor', label: '按钮颜色', type: 'color', fallback: '#409EFF' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#111827' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    'Toggle 按钮': [
      COMMAND_ID,
      { key: 'labelOn', label: '开启文本', type: 'text' },
      { key: 'labelOff', label: '关闭文本', type: 'text' },
      { key: 'active', label: '当前状态', type: 'checkbox' },
      { key: 'buttonType', label: '关闭类型', type: 'select', options: BUTTON_TYPE_OPTS },
      { key: 'activeType', label: '开启类型', type: 'select', options: BUTTON_TYPE_OPTS },
      { key: 'plain', label: '朴素(关闭时)', type: 'checkbox' },
      { key: 'round', label: '圆角按钮', type: 'checkbox' },
      { key: 'buttonColor', label: '按钮颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'pressColor', label: '按下颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'activeColor', label: '激活颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'textColor', label: '文字颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'borderColor', label: '边框颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '超链接': [
      COMMAND_ID,
      { key: 'label', label: '文本', type: 'text' },
      { key: 'href', label: '链接', type: 'text' },
      { key: 'targetBlank', label: '新窗口打开', type: 'checkbox' },
      { key: 'buttonType', label: '类型', type: 'select', options: BUTTON_TYPE_OPTS },
      { key: 'plain', label: '朴素按钮', type: 'checkbox' },
      { key: 'round', label: '圆角按钮', type: 'checkbox' },
      { key: 'buttonColor', label: '按钮颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'activeColor', label: '按下颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'textColor', label: '文字颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'borderColor', label: '边框颜色(可选覆盖)', type: 'color', fallback: '' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    'Web': [
      COMMAND_ID,
      { key: 'url', label: '网页 URL', type: 'text' },
      { key: 'allowScripts', label: '允许脚本', type: 'checkbox' },
      { key: 'allowForms', label: '允许表单', type: 'checkbox' },
      { key: 'allowSameOrigin', label: '允许同源', type: 'checkbox' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '3D散点': [
      COMMAND_ID,
      { key: 'maxPoints', label: '最大点数', type: 'number' },
      { key: 'symbolSize', label: '点大小', type: 'number' },
      { key: 'pointColor', label: '点颜色', type: 'color', fallback: '#38bdf8' },
      { key: 'xLabel', label: 'X 轴标签', type: 'text' },
      { key: 'yLabel', label: 'Y 轴标签', type: 'text' },
      { key: 'zLabel', label: 'Z 轴标签', type: 'text' },
      { key: 'showAxes', label: '显示坐标轴', type: 'checkbox' },
      { key: 'autoRotate', label: '自动旋转', type: 'checkbox' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#334155' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '3D折线': [
      COMMAND_ID,
      { key: 'maxPoints', label: '最大点数', type: 'number' },
      { key: 'lineWidth', label: '线宽', type: 'number' },
      { key: 'lineColor', label: '线颜色', type: 'color', fallback: '#38bdf8' },
      { key: 'xLabel', label: 'X 轴标签', type: 'text' },
      { key: 'yLabel', label: 'Y 轴标签', type: 'text' },
      { key: 'zLabel', label: 'Z 轴标签', type: 'text' },
      { key: 'showAxes', label: '显示坐标轴', type: 'checkbox' },
      { key: 'autoRotate', label: '自动旋转', type: 'checkbox' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#334155' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ]
  };

  function supportsType(type) {
    const t = String(type || '').trim();
    if (LEGACY_PROP_TYPES.has(t)) return false;
    if (CUSTOM_PANEL_TYPES.has(t) || !!SCHEMA_BY_TYPE[t]) return true;
    if (t === '按钮' || t === '切换按钮' || t === '竖向分割线'
      || t === '浮点滑块' || t === '竖向滑动条' || t === '竖向浮点滑块') return true;
    return false;
  }

  function resolvePropType(type) {
    const t = String(type || '').trim();
    if (t === '按钮') return 'Trigger 按钮';
    if (t === '切换按钮') return 'Toggle 按钮';
    if (t === '竖向分割线') return '分割线';
    if (t === '浮点滑块' || t === '竖向滑动条' || t === '竖向浮点滑块') return '滑块';
    return t;
  }

  function appendMultiHint(form, multiCount) {
    if (!(multiCount > 1)) return;
    const multiRow = document.createElement('div');
    multiRow.className = 'ns-prop-section';
    multiRow.innerHTML =
      '<div class="ns-prop-section-body">' +
        '<div class="prop-label">多选提示</div>' +
        '<div class="prop-value text-muted">已选择 ' + multiCount + ' 个控件，属性面板仅显示最后选中控件</div>' +
      '</div>';
    form.appendChild(multiRow);
  }

  function appendTypeRow(form, type) {
    const pill = document.createElement('div');
    pill.className = 'ns-prop-type-pill';
    pill.textContent = String(type || '未知');
    form.appendChild(pill);
  }

  function createPropSection(title, openByDefault) {
    const sec = document.createElement('div');
    sec.className = 'ns-prop-section' + (openByDefault === false ? ' collapsed' : '');
    const head = document.createElement('button');
    head.type = 'button';
    head.className = 'ns-prop-section-head';
    head.innerHTML = '<span></span><span class="ns-prop-section-caret" aria-hidden="true">▼</span>';
    head.querySelector('span').textContent = title;
    head.addEventListener('click', () => sec.classList.toggle('collapsed'));
    const body = document.createElement('div');
    body.className = 'ns-prop-section-body';
    sec.appendChild(head);
    sec.appendChild(body);
    return { sec, body, head };
  }

  function classifyFieldGroup(field) {
    const key = String((field && field.key) || '');
    if (key === 'commandId') return 'bind';
    if (field && field.type === 'color') return 'appearance';
    if (/Color|border|font|shadow|radius|stroke|bg|opacity/i.test(key)) return 'appearance';
    return 'content';
  }

  function appendCommandIdRow(form, props, onChange, options) {
    const opts = options || {};
    const section = createPropSection(opts.sectionTitle || '命令绑定', true);
    const cmdIdDiv = document.createElement('div');
    cmdIdDiv.className = 'mb-1';
    cmdIdDiv.innerHTML = '<div class="prop-label">' + (opts.label || 'Command ID') + '</div>';
    const cmdIdInput = document.createElement('input');
    cmdIdInput.className = 'form-control form-control-sm';
    if (opts.inputId) cmdIdInput.id = opts.inputId;
    cmdIdInput.value = props.commandId || '/cmd/demo';
    cmdIdInput.addEventListener('change', () => onChange({ commandId: cmdIdInput.value }));
    cmdIdDiv.appendChild(cmdIdInput);
    section.body.appendChild(cmdIdDiv);
    form.appendChild(section.sec);
    return section;
  }

  function appendBgColorRow(styleRow, props, onChange, hooks, node) {
    const bgColorDiv = document.createElement('div');
    bgColorDiv.className = 'mb-1 d-flex align-items-center justify-content-between';
    bgColorDiv.innerHTML = '<div class="prop-label">背景色</div>';
    const bgColorInput = document.createElement('input');
    bgColorInput.type = 'color';
    bgColorInput.className = 'form-control form-control-color form-control-sm';
    const initColor = hooks && hooks.initColorInput;
    const bgRaw = (props.bgColor !== undefined && props.bgColor !== null) ? props.bgColor : 'transparent';
    if (typeof initColor === 'function') initColor(bgColorInput, bgRaw, '#ffffff');
    else bgColorInput.value = String(bgRaw || '#ffffff');
    bgColorInput.addEventListener('change', () => {
      onChange({ bgColor: bgColorInput.value });
      try { if (node) node.querySelector('.grid-stack-item-content').style.backgroundColor = bgColorInput.value; } catch {}
    });
    bgColorDiv.appendChild(bgColorInput);
    styleRow.appendChild(bgColorDiv);
  }

  function createStyleSection(form, props, onChange, hooks, node, options) {
    const opts = options || {};
    const section = createPropSection(opts.title || '外观', true);
    appendBgColorRow(section.body, props, onChange, hooks, node);
    if (opts.showFontSize !== false) {
      const fontSizeDiv = document.createElement('div');
      fontSizeDiv.className = 'mb-1';
      fontSizeDiv.innerHTML = '<div class="prop-label">字体大小 (px)</div>';
      const fontSizeInput = document.createElement('input');
      fontSizeInput.type = 'number';
      fontSizeInput.className = 'form-control form-control-sm';
      fontSizeInput.value = props.fontSize || '14';
      fontSizeInput.addEventListener('change', () => {
        onChange({ fontSize: fontSizeInput.value });
        try { if (node) node.style.fontSize = fontSizeInput.value + 'px'; } catch {}
      });
      fontSizeDiv.appendChild(fontSizeInput);
      section.body.appendChild(fontSizeDiv);
    }
    form.appendChild(section.sec);
    return section.body;
  }

  function renderFramePanel(form, ctx) {
    const { node, type, props, multiCount, initColorInput, onChange } = ctx || {};
    if (!form || !node) return false;
    appendMultiHint(form, multiCount);
    appendTypeRow(form, type || 'Frame');
    const styleRow = createStyleSection(form, props, onChange, { initColorInput }, node, { showFontSize: false });

    const fields = [
      { key: 'label', label: '标题文本', type: 'text', def: '' },
      { key: 'labelColor', label: '标题颜色', type: 'color', fallback: '#111827' },
      { key: 'labelBgColor', label: '标题背景', type: 'color', fallback: '#f8fafc' },
      { key: 'labelFontSize', label: '标题字号 (px)', type: 'number', def: 14 },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'radius', label: '圆角 (px)', type: 'number', def: 10 }
    ];
    fields.forEach(field => {
      styleRow.appendChild(createFieldRow(field, props[field.key], (key, val) => onChange({ [key]: val }), { initColorInput }));
    });

    const fwRow = document.createElement('div');
    fwRow.className = 'mb-2';
    fwRow.innerHTML = '<div class="prop-label">标题字重</div>';
    const fwSelect = document.createElement('select');
    fwSelect.className = 'form-select form-select-sm';
    ['300', '400', '500', '600', '700', '800'].forEach(w => {
      const opt = document.createElement('option');
      opt.value = w;
      opt.textContent = w;
      if (String(props.labelFontWeight ?? '500') === w) opt.selected = true;
      fwSelect.appendChild(opt);
    });
    fwSelect.addEventListener('change', () => onChange({ labelFontWeight: fwSelect.value }));
    fwRow.appendChild(fwSelect);
    styleRow.appendChild(fwRow);

    const offRow = document.createElement('div');
    offRow.className = 'mb-2';
    offRow.innerHTML = '<div class="prop-label">标题偏移 (x / y)</div>';
    const offWrap = document.createElement('div');
    offWrap.className = 'd-flex align-items-center';
    offWrap.style.gap = '6px';
    const offX = document.createElement('input');
    offX.type = 'number';
    offX.step = '1';
    offX.className = 'form-control form-control-sm';
    offX.value = String(props.labelOffsetX ?? 12);
    const offY = document.createElement('input');
    offY.type = 'number';
    offY.step = '1';
    offY.className = 'form-control form-control-sm';
    offY.value = String(props.labelOffsetY ?? 0);
    const applyOff = () => onChange({ labelOffsetX: Number(offX.value), labelOffsetY: Number(offY.value) });
    offX.addEventListener('change', applyOff);
    offX.addEventListener('input', applyOff);
    offY.addEventListener('change', applyOff);
    offY.addEventListener('input', applyOff);
    offWrap.appendChild(offX);
    offWrap.appendChild(offY);
    offRow.appendChild(offWrap);
    styleRow.appendChild(offRow);

    styleRow.appendChild(createFieldRow(
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS },
      props.borderStyle || 'solid',
      (key, val) => onChange({ [key]: val }),
      { initColorInput }
    ));

    const shadowRow = document.createElement('div');
    shadowRow.className = 'mb-2';
    shadowRow.innerHTML = '<div class="prop-label">阴影</div>';
    const shadowSelect = document.createElement('select');
    shadowSelect.className = 'form-select form-select-sm';
    FRAME_SHADOW_PRESETS.forEach(p => {
      const opt = document.createElement('option');
      opt.value = p.v;
      opt.textContent = p.t;
      if (String(props.shadow || 'none') === p.v) opt.selected = true;
      shadowSelect.appendChild(opt);
    });
    shadowSelect.addEventListener('change', () => onChange({ shadow: shadowSelect.value }));
    shadowRow.appendChild(shadowSelect);
    styleRow.appendChild(shadowRow);
    return true;
  }

  function renderTimecodePanel(form, ctx) {
    const { node, type, props, multiCount, initColorInput, onChange } = ctx || {};
    if (!form || !node) return false;
    appendMultiHint(form, multiCount);
    appendTypeRow(form, type || '时间码');
    appendCommandIdRow(form, props, onChange, { label: 'Command ID' });

    const specBoxWrap = createPropSection('控件属性', true);
    const specBox = specBoxWrap.body;
    specBox.appendChild(createFieldRow({ key: 'fps', label: '帧率 (fps)', type: 'number' }, props.fps ?? 25, (k, v) => onChange({ [k]: Number(v) }), { initColorInput }));
    specBox.appendChild(createFieldRow({ key: 'readOnly', label: '只读（隐藏按钮）', type: 'checkbox' }, props.readOnly, (k, v) => onChange({ [k]: v }), { initColorInput }));
    specBox.appendChild(createFieldRow({ key: 'value', label: '帧数', type: 'number' }, props.value ?? 0, (k, v) => onChange({ [k]: Number(v) }), { initColorInput }));
    form.appendChild(specBoxWrap.sec);

    const styleRow = createStyleSection(form, props, onChange, { initColorInput }, node);
    styleRow.appendChild(createFieldRow({ key: 'textColor', label: '文字颜色', type: 'color', fallback: '#111827' }, props.textColor, (k, v) => onChange({ [k]: v }), { initColorInput }));
    styleRow.appendChild(createFieldRow({ key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' }, props.borderColor, (k, v) => onChange({ [k]: v }), { initColorInput }));
    styleRow.appendChild(createFieldRow(
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS },
      props.borderStyle || 'none',
      (k, v) => onChange({ [k]: v }),
      { initColorInput }
    ));
    return true;
  }

  function renderRadioPanel(form, ctx) {
    const { node, type, props, multiCount, initColorInput, onChange } = ctx || {};
    if (!form || !node) return false;
    appendMultiHint(form, multiCount);
    appendTypeRow(form, type || '单选框');
    appendCommandIdRow(form, props, onChange, { label: 'Command ID（收发 index）' });

    const clampCount = (n) => {
      const v = Math.floor(Number(n));
      if (!Number.isFinite(v)) return 3;
      return Math.max(2, Math.min(16, v));
    };
    const asList = (v) => (Array.isArray(v) ? v.map((x) => String(x ?? '')) : []);
    const pad = (list, count) => {
      const out = list.slice(0, count);
      while (out.length < count) out.push('选项 ' + (out.length + 1));
      return out;
    };

    let count = clampCount(props.count);
    let labels = pad(asList(props.labels), count);

    const optSection = createPropSection('选项', true);

    const dirRow = document.createElement('div');
    dirRow.className = 'mb-2';
    dirRow.innerHTML = '<label class="prop-label d-block mb-1">排列方向</label>';
    const dirSelect = document.createElement('select');
    dirSelect.className = 'form-select form-select-sm';
    const curDir = String(props.direction || 'vertical');
    const dirIsH = curDir === 'horizontal' || curDir === '横向' || curDir === 'row';
    [
      { v: 'vertical', t: '纵向' },
      { v: 'horizontal', t: '横向' }
    ].forEach((opt) => {
      const o = document.createElement('option');
      o.value = opt.v;
      o.textContent = opt.t;
      if ((opt.v === 'horizontal' && dirIsH) || (opt.v === 'vertical' && !dirIsH)) o.selected = true;
      dirSelect.appendChild(o);
    });
    dirSelect.addEventListener('change', () => {
      onChange({ direction: dirSelect.value });
    });
    dirRow.appendChild(dirSelect);
    optSection.body.appendChild(dirRow);

    const countRow = document.createElement('div');
    countRow.className = 'mb-2';
    countRow.innerHTML = '<label class="prop-label d-block mb-1">选项数量</label>';
    const countInput = document.createElement('input');
    countInput.type = 'number';
    countInput.min = '2';
    countInput.max = '16';
    countInput.className = 'form-control form-control-sm';
    countInput.value = String(count);
    countRow.appendChild(countInput);
    optSection.body.appendChild(countRow);

    const listHost = document.createElement('div');
    optSection.body.appendChild(listHost);
    form.appendChild(optSection.sec);

    function rebuildList() {
      listHost.innerHTML = '';
      for (let i = 0; i < count; i++) {
        const row = document.createElement('div');
        row.className = 'mb-2';
        row.innerHTML = '<label class="prop-label d-block mb-1">选项 [' + i + '] 文字</label>';
        const labelInput = document.createElement('input');
        labelInput.type = 'text';
        labelInput.className = 'form-control form-control-sm';
        labelInput.value = labels[i] || '';
        const applyLabel = () => {
          labels[i] = labelInput.value;
          onChange({ labels: labels.slice(), count });
        };
        labelInput.addEventListener('change', applyLabel);
        labelInput.addEventListener('input', applyLabel);
        row.appendChild(labelInput);
        listHost.appendChild(row);
      }
    }

    countInput.addEventListener('change', () => {
      count = clampCount(countInput.value);
      countInput.value = String(count);
      labels = pad(labels, count);
      onChange({
        count,
        labels: labels.slice(),
        selected: Math.min(Number(props.selected) || 0, count - 1)
      });
      rebuildList();
    });
    rebuildList();

    const styleSection = createPropSection('外观', true);
    const hooks = { initColorInput };
    styleSection.body.appendChild(createFieldRow(
      { key: 'selected', label: '当前选中 index', type: 'number' },
      props.selected ?? 0,
      (_k, v) => onChange({ selected: Number(v) }),
      hooks
    ));
    styleSection.body.appendChild(createFieldRow(
      { key: 'activeColor', label: '选中颜色', type: 'color', fallback: '#409EFF' },
      props.activeColor ?? '#409EFF',
      (_k, v) => onChange({ activeColor: v }),
      hooks
    ));
    styleSection.body.appendChild(createFieldRow(
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#111827' },
      props.textColor ?? '#111827',
      (_k, v) => onChange({ textColor: v }),
      hooks
    ));
    styleSection.body.appendChild(createFieldRow(
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      props.borderColor ?? '#e5e7eb',
      (_k, v) => onChange({ borderColor: v }),
      hooks
    ));
    styleSection.body.appendChild(createFieldRow(
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS },
      props.borderStyle || 'none',
      (_k, v) => onChange({ borderStyle: v }),
      hooks
    ));
    form.appendChild(styleSection.sec);
    return true;
  }

  function createFieldRow(field, value, onChange, hooks) {
    const row = document.createElement('div');
    row.className = field.type === 'color'
      ? 'mb-2 d-flex align-items-center justify-content-between'
      : (field.type === 'checkbox' ? 'mb-2 form-check' : 'mb-2');

    const labelText = field.label || field.key;
    const label = document.createElement(field.type === 'checkbox' ? 'label' : 'label');
    label.className = field.type === 'checkbox' ? 'prop-label form-check-label' : 'prop-label d-block mb-1';
    label.textContent = labelText;

    let input;
    if (field.type === 'checkbox') {
      input = document.createElement('input');
      input.type = 'checkbox';
      input.className = 'form-check-input';
      input.checked = !!(value === true || value === 'true' || value === 1);
      input.addEventListener('change', () => onChange(field.key, !!input.checked));
      row.appendChild(input);
      row.appendChild(label);
    } else if (field.type === 'select') {
      input = document.createElement('select');
      input.className = 'form-select form-select-sm';
      (field.options || []).forEach(opt => {
        const o = document.createElement('option');
        if (opt && typeof opt === 'object') {
          o.value = String(opt.v);
          o.textContent = String(opt.t != null ? opt.t : opt.v);
          if (String(value ?? field.default ?? '') === String(opt.v)) o.selected = true;
        } else {
          o.value = opt;
          o.textContent = opt;
          if (String(value ?? field.default ?? '') === String(opt)) o.selected = true;
        }
        input.appendChild(o);
      });
      input.addEventListener('change', () => onChange(field.key, input.value));
      row.appendChild(label);
      row.appendChild(input);
    } else if (field.type === 'color') {
      input = document.createElement('input');
      input.type = 'color';
      input.className = 'form-control form-control-color form-control-sm';
      const initColor = hooks && hooks.initColorInput;
      const raw = (value !== undefined && value !== null) ? value : (field.fallback || '#ffffff');
      if (typeof initColor === 'function') initColor(input, raw, field.fallback || '#ffffff');
      else input.value = String(raw || '#ffffff');
      input.addEventListener('change', () => onChange(field.key, input.value));
      row.appendChild(label);
      row.appendChild(input);
    } else if (field.type === 'textarea') {
      input = document.createElement('textarea');
      input.className = 'form-control form-control-sm';
      input.rows = Number(field.rows) > 0 ? Number(field.rows) : 4;
      input.style.resize = 'vertical';
      input.style.minHeight = '72px';
      input.value = value !== undefined && value !== null ? String(value) : '';
      const apply = () => onChange(field.key, input.value);
      input.addEventListener('change', apply);
      input.addEventListener('input', apply);
      row.appendChild(label);
      row.appendChild(input);
    } else {
      input = document.createElement('input');
      input.className = 'form-control form-control-sm';
      input.type = field.type === 'number' ? 'number' : 'text';
      input.value = value !== undefined && value !== null ? String(value) : '';
      const apply = () => {
        let v = input.value;
        if (field.type === 'number') v = Number(v);
        onChange(field.key, v);
      };
      input.addEventListener('change', apply);
      input.addEventListener('input', apply);
      row.appendChild(label);
      row.appendChild(input);
    }

    try {
      if (window.NSA11y && typeof window.NSA11y.bindControl === 'function') {
        window.NSA11y.bindControl(labelText, input, { labelEl: label });
      }
    } catch {}

    return row;
  }

  function renderWidgetPanel(form, ctx) {
    const { node, type, props, multiCount, initColorInput, onChange } = ctx || {};
    if (!form || !node || !type) return false;

    if (type === 'Frame') return renderFramePanel(form, ctx);
    if (type === '时间码') return renderTimecodePanel(form, ctx);
    if (type === '单选框') return renderRadioPanel(form, ctx);

    const schemaType = resolvePropType(type);
    const schema = SCHEMA_BY_TYPE[schemaType];
    if (!schema) return false;

    appendMultiHint(form, multiCount);
    appendTypeRow(form, schemaType);

    const groups = { bind: [], content: [], appearance: [] };
    schema.forEach(field => {
      groups[classifyFieldGroup(field)].push(field);
    });

    const hooks = { initColorInput };
    const SECTION_META = [
      { id: 'bind', title: '命令绑定', open: true },
      { id: 'content', title: '内容 / 行为', open: true },
      { id: 'appearance', title: '外观', open: true }
    ];

    // 通用外观：背景 + 字号放在外观分组最前
    const appearanceSection = createPropSection('外观', true);
    appendBgColorRow(appearanceSection.body, props, onChange, hooks, node);
    const fontSizeDiv = document.createElement('div');
    fontSizeDiv.className = 'mb-2';
    fontSizeDiv.innerHTML = '<label class="prop-label d-block mb-1">字体大小 (px)</label>';
    const fontSizeInput = document.createElement('input');
    fontSizeInput.type = 'number';
    fontSizeInput.className = 'form-control form-control-sm';
    fontSizeInput.value = props.fontSize || '14';
    fontSizeInput.addEventListener('change', () => {
      onChange({ fontSize: fontSizeInput.value });
      try { node.style.fontSize = fontSizeInput.value + 'px'; } catch {}
    });
    fontSizeDiv.appendChild(fontSizeInput);
    try { if (window.NSA11y) window.NSA11y.wirePropRow(fontSizeDiv, fontSizeInput, '字体大小 (px)'); } catch {}
    appearanceSection.body.appendChild(fontSizeDiv);

    SECTION_META.forEach(meta => {
      const fields = groups[meta.id] || [];
      if (meta.id === 'appearance') {
        fields.forEach(field => {
          appearanceSection.body.appendChild(createFieldRow(field, props[field.key], (key, val) => {
            onChange({ [key]: val });
          }, hooks));
        });
        form.appendChild(appearanceSection.sec);
        return;
      }
      if (!fields.length) return;
      const section = createPropSection(meta.title, meta.open);
      fields.forEach(field => {
        section.body.appendChild(createFieldRow(field, props[field.key], (key, val) => {
          onChange({ [key]: val });
        }, hooks));
      });
      form.appendChild(section.sec);
    });

    return true;
  }

  window.NSPropSchema = {
    LEGACY_PROP_TYPES,
    CUSTOM_PANEL_TYPES,
    SCHEMA_BY_TYPE,
    supportsType,
    renderWidgetPanel,
    renderFramePanel,
    renderTimecodePanel,
    renderRadioPanel,
    getSchema(type) {
      const t = resolvePropType(type);
      return SCHEMA_BY_TYPE[t] || null;
    }
  };
})();
