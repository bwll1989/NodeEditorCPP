// ns-prop-schema.js —— 属性面板 Schema 定义与通用渲染（替代 updatePropPanel 大部分内联 HTML）
(function() {
  'use strict';

  const BORDER_STYLE_OPTS = ['none', 'solid', 'dashed', 'dotted', 'double'];
  const COMMAND_ID = { key: 'commandId', label: 'Command ID', type: 'text' };

  const LEGACY_PROP_TYPES = new Set([]);

  const CUSTOM_PANEL_TYPES = new Set(['Frame', '时间线', '时间码', '卡片']);

  const FRAME_SHADOW_PRESETS = [
    { v: 'none', t: '无' },
    { v: '0 4px 12px rgba(0,0,0,0.12)', t: '柔和' },
    { v: '0 10px 30px rgba(0,0,0,0.18)', t: '中等' },
    { v: '0 20px 60px rgba(0,0,0,0.25)', t: '强烈' }
  ];

  const SCHEMA_BY_TYPE = {
    '按钮': [
      COMMAND_ID,
      { key: 'label', label: '文本', type: 'text' },
      { key: 'buttonColor', label: '按钮颜色', type: 'color', fallback: '#409EFF' },
      { key: 'activeColor', label: '按下颜色', type: 'color', fallback: '#3a8ee6' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#ffffff' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#409EFF' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '滑块': [
      COMMAND_ID,
      { key: 'min', label: '最小值', type: 'number' },
      { key: 'max', label: '最大值', type: 'number' },
      { key: 'value', label: '当前值', type: 'number' },
      { key: 'barColor', label: '轨道颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'fillColor', label: '进度颜色', type: 'color', fallback: '#2b6cb0' },
      { key: 'valueColor', label: '数值颜色', type: 'color', fallback: '#111827' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '浮点滑块': [
      COMMAND_ID,
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
    '竖向滑动条': [
      COMMAND_ID,
      { key: 'min', label: '最小值', type: 'number' },
      { key: 'max', label: '最大值', type: 'number' },
      { key: 'value', label: '当前值', type: 'number' },
      { key: 'height', label: '高度', type: 'text' },
      { key: 'barColor', label: '轨道颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'fillColor', label: '进度颜色', type: 'color', fallback: '#2b6cb0' },
      { key: 'valueColor', label: '数值颜色', type: 'color', fallback: '#111827' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '竖向浮点滑块': [
      COMMAND_ID,
      { key: 'min', label: '最小值', type: 'number' },
      { key: 'max', label: '最大值', type: 'number' },
      { key: 'step', label: '步进', type: 'number' },
      { key: 'value', label: '当前值', type: 'number' },
      { key: 'height', label: '高度', type: 'text' },
      { key: 'barColor', label: '轨道颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'fillColor', label: '进度颜色', type: 'color', fallback: '#2b6cb0' },
      { key: 'valueColor', label: '数值颜色', type: 'color', fallback: '#111827' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '勾选': [
      COMMAND_ID,
      { key: 'label', label: '文本', type: 'text' },
      { key: 'checked', label: '选中', type: 'checkbox' },
      { key: 'activeColor', label: '选中颜色', type: 'color', fallback: '#409EFF' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#334155' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '开关': [
      COMMAND_ID,
      { key: 'checked', label: '选中', type: 'checkbox' },
      { key: 'onColor', label: '开启颜色', type: 'color', fallback: '#409EFF' },
      { key: 'offColor', label: '关闭颜色', type: 'color', fallback: '#dcdfe6' },
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
    '分割线': [
      { key: 'text', label: '文本', type: 'text' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#64748b' },
      { key: 'lineColor', label: '线条颜色', type: 'color', fallback: '#cbd5e1' },
      { key: 'lineWidth', label: '线宽 (px)', type: 'number' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '竖向分割线': [
      { key: 'lineColor', label: '线条颜色', type: 'color', fallback: '#cbd5e1' },
      { key: 'lineWidth', label: '线宽 (px)', type: 'number' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
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
    '切换按钮': [
      COMMAND_ID,
      { key: 'labelOn', label: '开启文本', type: 'text' },
      { key: 'labelOff', label: '关闭文本', type: 'text' },
      { key: 'active', label: '当前状态', type: 'checkbox' },
      { key: 'buttonColor', label: '按钮颜色', type: 'color', fallback: '#409EFF' },
      { key: 'pressColor', label: '按下颜色', type: 'color', fallback: '#3a8ee6' },
      { key: 'activeColor', label: '激活颜色', type: 'color', fallback: '#0e5d45' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#ffffff' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#409EFF' },
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS }
    ],
    '超链接': [
      COMMAND_ID,
      { key: 'label', label: '文本', type: 'text' },
      { key: 'href', label: '链接', type: 'text' },
      { key: 'targetBlank', label: '新窗口打开', type: 'checkbox' },
      { key: 'buttonColor', label: '按钮颜色', type: 'color', fallback: '#409EFF' },
      { key: 'activeColor', label: '按下颜色', type: 'color', fallback: '#3a8ee6' },
      { key: 'textColor', label: '文字颜色', type: 'color', fallback: '#ffffff' },
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#409EFF' },
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
    return CUSTOM_PANEL_TYPES.has(t) || !!SCHEMA_BY_TYPE[t];
  }

  function appendMultiHint(form, multiCount) {
    if (!(multiCount > 1)) return;
    const multiRow = document.createElement('div');
    multiRow.className = 'mb-2 p-2 bg-light border rounded';
    multiRow.innerHTML = '<div class="prop-label">提示</div><div class="prop-value">当前已选择 ' + multiCount + ' 个控件，属性面板仅显示最后选中控件</div>';
    form.appendChild(multiRow);
  }

  function appendTypeRow(form, type) {
    const typeRow = document.createElement('div');
    typeRow.className = 'mb-2';
    typeRow.innerHTML = '<div class="prop-label">类型</div><div class="prop-value text-muted">' + type + '</div>';
    form.appendChild(typeRow);
  }

  function appendCommandIdRow(form, props, onChange, options) {
    const opts = options || {};
    const idRow = document.createElement('div');
    idRow.className = 'mb-2 p-2 bg-light border rounded';
    idRow.innerHTML = '<div class="prop-label fw-bold mb-1">ID 设置</div>';
    const cmdIdDiv = document.createElement('div');
    cmdIdDiv.className = 'mb-1';
    cmdIdDiv.innerHTML = '<div class="prop-label">' + (opts.label || 'Command ID') + '</div>';
    const cmdIdInput = document.createElement('input');
    cmdIdInput.className = 'form-control form-control-sm';
    if (opts.inputId) cmdIdInput.id = opts.inputId;
    cmdIdInput.value = props.commandId || '/cmd/demo';
    cmdIdInput.addEventListener('change', () => onChange({ commandId: cmdIdInput.value }));
    cmdIdDiv.appendChild(cmdIdInput);
    idRow.appendChild(cmdIdDiv);
    form.appendChild(idRow);
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
    const styleRow = document.createElement('div');
    styleRow.className = 'mb-2 p-2 bg-light border rounded';
    styleRow.innerHTML = '<div class="prop-label fw-bold mb-1">样式设置</div>';
    appendBgColorRow(styleRow, props, onChange, hooks, node);
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
      styleRow.appendChild(fontSizeDiv);
    }
    form.appendChild(styleRow);
    return styleRow;
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

  function renderCardPanel(form, ctx) {
    const { node, type, props, multiCount, initColorInput, onChange } = ctx || {};
    if (!form || !node) return false;
    appendMultiHint(form, multiCount);
    appendTypeRow(form, type || '卡片');

    const styleRow = document.createElement('div');
    styleRow.className = 'mb-2 p-2 bg-light border rounded';
    styleRow.innerHTML = '<div class="prop-label fw-bold mb-1">样式设置</div>';
    appendBgColorRow(styleRow, props, onChange, { initColorInput }, node);

    const fontSizeDiv = document.createElement('div');
    fontSizeDiv.className = 'mb-1';
    fontSizeDiv.innerHTML = '<div class="prop-label">标题字号 (px)</div>';
    const fontSizeInput = document.createElement('input');
    fontSizeInput.type = 'number';
    fontSizeInput.className = 'form-control form-control-sm';
    fontSizeInput.min = '10';
    fontSizeInput.max = '64';
    fontSizeInput.value = String(props.fontSize ?? props.labelFontSize ?? 14);
    const applyFontSize = () => {
      const v = fontSizeInput.value;
      onChange({ fontSize: v, labelFontSize: Number(v) || 14 });
      try { node.style.fontSize = v + 'px'; } catch {}
    };
    fontSizeInput.addEventListener('change', applyFontSize);
    fontSizeInput.addEventListener('input', applyFontSize);
    fontSizeDiv.appendChild(fontSizeInput);
    styleRow.appendChild(fontSizeDiv);
    form.appendChild(styleRow);

    const specBox = document.createElement('div');
    specBox.className = 'mb-2 p-2 bg-light border rounded';
    specBox.innerHTML = '<div class="prop-label fw-bold mb-1">卡片属性</div>';
    const hooks = { initColorInput };

    const titleRow = document.createElement('div');
    titleRow.className = 'mb-2';
    titleRow.innerHTML = '<div class="prop-label">标题</div>';
    const titleInput = document.createElement('input');
    titleInput.type = 'text';
    titleInput.className = 'form-control form-control-sm';
    titleInput.value = String(props.title ?? props.label ?? '');
    const applyTitle = () => {
      const v = String(titleInput.value ?? '');
      onChange({ title: v, label: v });
    };
    titleInput.addEventListener('change', applyTitle);
    titleInput.addEventListener('input', applyTitle);
    titleRow.appendChild(titleInput);
    specBox.appendChild(titleRow);

    specBox.appendChild(createFieldRow(
      { key: 'textColor', label: '标题颜色', type: 'color', fallback: '#111827' },
      props.textColor ?? props.labelColor ?? '#111827',
      (_k, v) => onChange({ textColor: v, labelColor: v }),
      hooks
    ));
    specBox.appendChild(createFieldRow(
      { key: 'labelBgColor', label: '标题背景', type: 'color', fallback: '#f8fafc' },
      props.labelBgColor ?? '#f8fafc',
      (_k, v) => onChange({ labelBgColor: v }),
      hooks
    ));

    const fwRow = document.createElement('div');
    fwRow.className = 'mb-2';
    fwRow.innerHTML = '<div class="prop-label">标题字重</div>';
    const fwSelect = document.createElement('select');
    fwSelect.className = 'form-select form-select-sm';
    ['300', '400', '500', '600', '700', '800'].forEach(w => {
      const opt = document.createElement('option');
      opt.value = w;
      opt.textContent = w;
      if (String(props.labelFontWeight ?? '600') === w) opt.selected = true;
      fwSelect.appendChild(opt);
    });
    fwSelect.addEventListener('change', () => onChange({ labelFontWeight: fwSelect.value }));
    fwRow.appendChild(fwSelect);
    specBox.appendChild(fwRow);

    specBox.appendChild(createFieldRow(
      { key: 'borderColor', label: '边框颜色', type: 'color', fallback: '#e5e7eb' },
      props.borderColor ?? '#e5e7eb',
      (_k, v) => onChange({ borderColor: v }),
      hooks
    ));
    specBox.appendChild(createFieldRow(
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS },
      props.borderStyle || 'solid',
      (_k, v) => onChange({ borderStyle: v }),
      hooks
    ));
    specBox.appendChild(createFieldRow(
      { key: 'radius', label: '圆角 (px)', type: 'number' },
      props.radius ?? 10,
      (_k, v) => onChange({ radius: Number(v) }),
      hooks
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
    specBox.appendChild(shadowRow);

    form.appendChild(specBox);
    return true;
  }

  function renderTimelinePanel(form, ctx) {
    const { node, type, props, multiCount, initColorInput, onChange, refreshPanel } = ctx || {};
    if (!form || !node) return false;
    appendMultiHint(form, multiCount);
    appendTypeRow(form, type || '时间线');
    appendCommandIdRow(form, props, onChange, { label: '游标 Command ID', inputId: 'propTimelineCommandId' });

    const specBox = document.createElement('div');
    specBox.className = 'mb-2 p-2 bg-light border rounded';
    specBox.innerHTML = '<div class="prop-label fw-bold mb-1">控件属性</div>';
    specBox.appendChild(createFieldRow({ key: 'fps', label: '帧率 (fps)', type: 'number' }, props.fps ?? 25, (k, v) => onChange({ [k]: Number(v) }), { initColorInput }));
    specBox.appendChild(createFieldRow({ key: 'tickStepSec', label: '刻度步进 (秒)', type: 'number' }, props.tickStepSec ?? 15, (k, v) => onChange({ [k]: Number(v) }), { initColorInput }));
    form.appendChild(specBox);

    const itemsBox = document.createElement('div');
    itemsBox.className = 'mb-2 p-2 bg-light border rounded';
    itemsBox.innerHTML = '<div class="prop-label fw-bold mb-1">节点列表（CommandID → 备注）</div>';
    const items = Array.isArray(props.items) ? props.items : [];
    const updateItems = (next) => {
      onChange({ items: next });
      if (typeof refreshPanel === 'function') refreshPanel();
    };
    items.forEach((it, idx) => {
      const row = document.createElement('div');
      row.className = 'd-flex gap-2 align-items-center mb-1';
      const idInput = document.createElement('input');
      idInput.type = 'text';
      idInput.className = 'form-control form-control-sm';
      idInput.style.width = '220px';
      idInput.value = (it && it.id !== undefined) ? String(it.id) : '';
      idInput.placeholder = 'CommandID';
      idInput.addEventListener('change', () => {
        const next = items.map((x, i) => (i === idx ? Object.assign({}, x || {}, { id: idInput.value }) : x));
        updateItems(next);
      });
      const textInput = document.createElement('input');
      textInput.type = 'text';
      textInput.className = 'form-control form-control-sm';
      textInput.value = (it && it.text !== undefined) ? String(it.text) : '';
      textInput.placeholder = '备注';
      textInput.addEventListener('change', () => {
        const next = items.map((x, i) => (i === idx ? Object.assign({}, x || {}, { text: textInput.value }) : x));
        updateItems(next);
      });
      const rmBtn = document.createElement('button');
      rmBtn.type = 'button';
      rmBtn.className = 'btn btn-sm btn-outline-danger';
      rmBtn.textContent = '删除';
      rmBtn.addEventListener('click', () => updateItems(items.filter((_, i) => i !== idx)));
      row.appendChild(idInput);
      row.appendChild(textInput);
      row.appendChild(rmBtn);
      itemsBox.appendChild(row);
    });
    const addBtn = document.createElement('button');
    addBtn.type = 'button';
    addBtn.className = 'btn btn-sm btn-outline-primary mt-2';
    addBtn.textContent = '添加节点';
    addBtn.addEventListener('click', () => {
      const next = items.slice();
      next.push({ id: String(next.length), text: '节点' + (next.length + 1) });
      updateItems(next);
    });
    itemsBox.appendChild(addBtn);
    form.appendChild(itemsBox);

    const styleRow = createStyleSection(form, props, onChange, { initColorInput }, node);
    const timelineColors = [
      { key: 'lineColor', label: '连线颜色', fallback: '#e5e7eb' },
      { key: 'pointColor', label: '点颜色', fallback: '#2563eb' },
      { key: 'textColor', label: '文字颜色', fallback: '#111827' },
      { key: 'labelBgColor', label: '节点背景色', fallback: '#ffffff' },
      { key: 'borderColor', label: '边框颜色', fallback: '#e5e7eb' }
    ];
    timelineColors.forEach(field => {
      const val = field.key === 'pointColor'
        ? (props.pointColor || props.activeColor || field.fallback)
        : (props[field.key] || field.fallback);
      styleRow.appendChild(createFieldRow(
        { key: field.key, label: field.label, type: 'color', fallback: field.fallback },
        val,
        (k, v) => onChange({ [k]: v }),
        { initColorInput }
      ));
    });
    styleRow.appendChild(createFieldRow(
      { key: 'borderStyle', label: '边框样式', type: 'select', options: BORDER_STYLE_OPTS },
      props.borderStyle || 'none',
      (k, v) => onChange({ [k]: v }),
      { initColorInput }
    ));
    return true;
  }

  function renderTimecodePanel(form, ctx) {
    const { node, type, props, multiCount, initColorInput, onChange } = ctx || {};
    if (!form || !node) return false;
    appendMultiHint(form, multiCount);
    appendTypeRow(form, type || '时间码');
    appendCommandIdRow(form, props, onChange, { label: 'Command ID' });

    const specBox = document.createElement('div');
    specBox.className = 'mb-2 p-2 bg-light border rounded';
    specBox.innerHTML = '<div class="prop-label fw-bold mb-1">控件属性</div>';
    specBox.appendChild(createFieldRow({ key: 'fps', label: '帧率 (fps)', type: 'number' }, props.fps ?? 25, (k, v) => onChange({ [k]: Number(v) }), { initColorInput }));
    specBox.appendChild(createFieldRow({ key: 'readOnly', label: '只读（隐藏按钮）', type: 'checkbox' }, props.readOnly, (k, v) => onChange({ [k]: v }), { initColorInput }));
    specBox.appendChild(createFieldRow({ key: 'value', label: '帧数', type: 'number' }, props.value ?? 0, (k, v) => onChange({ [k]: Number(v) }), { initColorInput }));
    form.appendChild(specBox);

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
        o.value = opt;
        o.textContent = opt;
        if (String(value ?? field.default ?? '') === String(opt)) o.selected = true;
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
    if (type === '卡片') return renderCardPanel(form, ctx);
    if (type === '时间线') return renderTimelinePanel(form, ctx);
    if (type === '时间码') return renderTimecodePanel(form, ctx);

    const schema = SCHEMA_BY_TYPE[type];
    if (!schema) return false;

    appendMultiHint(form, multiCount);
    appendTypeRow(form, type);

    const styleRow = document.createElement('div');
    styleRow.className = 'mb-2 p-2 bg-light border rounded';
    styleRow.innerHTML = '<div class="prop-label fw-bold mb-1">样式设置</div>';
    appendBgColorRow(styleRow, props, onChange, { initColorInput }, node);

    const fontSizeDiv = document.createElement('div');
    fontSizeDiv.className = 'mb-1';
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
    styleRow.appendChild(fontSizeDiv);
    form.appendChild(styleRow);

    const specBox = document.createElement('div');
    specBox.className = 'mb-2 p-2 bg-light border rounded';
    specBox.innerHTML = '<div class="prop-label fw-bold mb-1">控件属性</div>';
    const hooks = { initColorInput };
    schema.forEach(field => {
      specBox.appendChild(createFieldRow(field, props[field.key], (key, val) => {
        onChange({ [key]: val });
      }, hooks));
    });
    form.appendChild(specBox);
    return true;
  }

  window.NSPropSchema = {
    LEGACY_PROP_TYPES,
    CUSTOM_PANEL_TYPES,
    SCHEMA_BY_TYPE,
    supportsType,
    renderWidgetPanel,
    renderFramePanel,
    renderCardPanel,
    renderTimelinePanel,
    renderTimecodePanel,
    getSchema(type) {
      return SCHEMA_BY_TYPE[String(type || '').trim()] || null;
    }
  };
})();
