// ns-widget-library.js —— 分类控件库 + 拖拽 ghost / 落点预览
(function() {
  'use strict';

  const MIME = 'text/x-ns-widget';

  const CATEGORIES = [
    {
      id: 'basic',
      title: '基础控件',
      types: ['Trigger 按钮', '开关', 'LED', '勾选', '单选框', 'Toggle 按钮', '输入框', '标签', 'Text', '数值', '步进器', '滑块', '旋钮', '分割线', '时间码', 'Frame', '超链接']
    },
    {
      id: 'chart',
      title: '图表',
      types: ['3D散点', '3D折线', 'Web']
    }
  ];

  const ICONS = {
    addButtonWidget: '<rect x="4" y="7" width="16" height="10" rx="2"></rect><path d="M8 12h8"></path>',
    addSliderWidget: '<path d="M5 12h14"></path><circle cx="10" cy="12" r="3"></circle><path d="M12 5v14"></path>',
    addCheckboxWidget: '<rect x="6" y="6" width="12" height="12" rx="2"></rect><path d="M9 12l2 2 4-5"></path>',
    addSwitchWidget: '<rect x="5" y="9" width="14" height="6" rx="3"></rect><circle cx="10" cy="12" r="2"></circle>',
    addLedWidget: '<circle cx="12" cy="12" r="6"></circle><circle cx="12" cy="12" r="2.5"></circle>',
    addInputWidget: '<rect x="5" y="8" width="14" height="8" rx="2"></rect><path d="M9 10v4"></path>',
    addToggleButtonWidget: '<path d="M7 9h10"></path><path d="M7 15h10"></path><path d="M9 9v6"></path>',
    addDividerWidget: '<path d="M6 12h12"></path><path d="M12 6v12"></path>',
    addLabelWidget: '<path d="M8 7h8"></path><path d="M12 7v10"></path><path d="M9 17h6"></path>',
    addTextWidget: '<path d="M6 7h12"></path><path d="M12 7v10"></path>',
    addKnobWidget: '<circle cx="12" cy="12" r="7"></circle><path d="M12 12l4-3"></path>',
    addTimecodeWidget: '<rect x="4" y="6" width="16" height="12" rx="2"></rect><path d="M8 10h2"></path><path d="M14 10h2"></path>',
    addNumberWidget: '<path d="M4 12h16"></path><path d="M10 6v12"></path>',
    addStepperWidget: '<rect x="4" y="7" width="6" height="10" rx="1"></rect><rect x="14" y="7" width="6" height="10" rx="1"></rect><path d="M11 12h2"></path>',
    addScatter3DWidget: '<circle cx="8" cy="14" r="1.5"></circle><circle cx="12" cy="9" r="1.5"></circle><circle cx="16" cy="13" r="1.5"></circle><path d="M5 18h14"></path><path d="M5 18V7"></path>',
    addLine3DWidget: '<path d="M5 16 L9 10 L13 13 L18 7"></path><circle cx="5" cy="16" r="1.2"></circle><circle cx="9" cy="10" r="1.2"></circle><circle cx="13" cy="13" r="1.2"></circle><circle cx="18" cy="7" r="1.2"></circle>',
    addFrameWidget: '<rect x="5" y="6" width="14" height="12" rx="2"></rect>',
    addLinkWidget: '<path d="M10 13a5 5 0 0 1 0-7l2-2a5 5 0 0 1 7 7l-2 2"></path><path d="M14 11a5 5 0 0 1 0 7l-2 2a5 5 0 0 1-7-7l2-2"></path>',
    addRadioWidget: '<circle cx="8" cy="8" r="3"></circle><circle cx="8" cy="16" r="3"></circle><path d="M14 8h6"></path><path d="M14 16h6"></path>',
    addWebWidget: '<rect x="4" y="5" width="16" height="14" rx="2"></rect><path d="M4 9h16"></path><path d="M8 5v4"></path>'
  };

  const DEFAULT_SIZE = {
    'Trigger 按钮': [120, 40],
    '滑块': [180, 40],
    '勾选': [40, 40],
    '开关': [64, 36],
    'LED': [40, 40],
    '输入框': [160, 40],
    'Toggle 按钮': [120, 40],
    '分割线': [180, 24],
    '标签': [120, 32],
    'Text': [200, 80],
    '旋钮': [100, 100],
    '时间码': [200, 48],
    '数值': [120, 40],
    '步进器': [140, 40],
    '3D散点': [280, 200],
    '3D折线': [280, 200],
    'Frame': [240, 160],
    '超链接': [120, 40],
    '单选框': [220, 100],
    'Web': [320, 200]
  };

  let dropGhostEl = null;
  let dragGhostEl = null;
  let draggingType = '';

  function getActiveCanvasEl() {
    try {
      const NS = window.NS;
      const info = NS && NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
      return info && info.canvasEl ? info.canvasEl : null;
    } catch { return null; }
  }

  function guessSize(type) {
    const s = DEFAULT_SIZE[String(type || '').trim()] || [140, 48];
    return { w: s[0], h: s[1] };
  }

  function clearDropGhost() {
    try {
      if (dropGhostEl && dropGhostEl.parentNode) dropGhostEl.parentNode.removeChild(dropGhostEl);
    } catch {}
    dropGhostEl = null;
  }

  function clearDragGhost() {
    try {
      if (dragGhostEl && dragGhostEl.parentNode) dragGhostEl.parentNode.removeChild(dragGhostEl);
    } catch {}
    dragGhostEl = null;
  }

  function updateDropGhost(canvas, clientX, clientY, type) {
    if (!canvas || !window.NSCanvas) return;
    const size = guessSize(type || draggingType);
    const p = window.NSCanvas.__clientToCanvasDesignXY(canvas, clientX, clientY);
    const x = Math.max(0, Math.round(p.x - size.w / 2));
    const y = Math.max(0, Math.round(p.y - size.h / 2));
    if (!dropGhostEl) {
      dropGhostEl = document.createElement('div');
      dropGhostEl.className = 'ns-drop-ghost';
      canvas.appendChild(dropGhostEl);
    } else if (dropGhostEl.parentNode !== canvas) {
      try { canvas.appendChild(dropGhostEl); } catch {}
    }
    dropGhostEl.style.left = x + 'px';
    dropGhostEl.style.top = y + 'px';
    dropGhostEl.style.width = size.w + 'px';
    dropGhostEl.style.height = size.h + 'px';
  }

  function createThumb(entry) {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.id = entry.libId;
    btn.className = 'widget-thumb';
    btn.dataset.widgetType = entry.type;
    btn.title = entry.thumbLabel || entry.type;
    const icon = document.createElement('span');
    icon.className = 'widget-thumb-icon';
    icon.innerHTML = '<svg viewBox="0 0 24 24" aria-hidden="true">' + (ICONS[entry.libId] || '<circle cx="12" cy="12" r="6"></circle>') + '</svg>';
    const label = document.createElement('span');
    label.className = 'widget-thumb-label';
    label.textContent = entry.thumbLabel || entry.type;
    btn.appendChild(icon);
    btn.appendChild(label);
    return btn;
  }

  function renderLibraryGrid(containerId) {
    const grid = document.getElementById(containerId || 'widgetLibraryGrid');
    if (!grid || !window.NSWidgetManifest) return;
    grid.innerHTML = '';
    grid.classList.add('ns-lib-cats');
    grid.classList.remove('widget-thumb-grid');

    const entries = window.NSWidgetManifest.libraryEntries();
    const byType = {};
    entries.forEach(e => { byType[e.type] = e; });
    const used = new Set();

    CATEGORIES.forEach((cat, idx) => {
      const items = cat.types.map(t => byType[t]).filter(Boolean);
      if (!items.length) return;
      items.forEach(e => used.add(e.type));

      const catEl = document.createElement('div');
      catEl.className = 'ns-lib-cat' + (idx === 0 ? '' : ' collapsed');
      catEl.dataset.catId = cat.id;

      const head = document.createElement('button');
      head.type = 'button';
      head.className = 'ns-lib-cat-head';
      head.innerHTML = '<span>' + cat.title + ' · ' + items.length + '</span><span class="ns-lib-cat-caret" aria-hidden="true">▼</span>';
      head.addEventListener('click', () => catEl.classList.toggle('collapsed'));

      const body = document.createElement('div');
      body.className = 'ns-lib-cat-body widget-thumb-grid';
      items.forEach(entry => body.appendChild(createThumb(entry)));

      catEl.appendChild(head);
      catEl.appendChild(body);
      grid.appendChild(catEl);
    });

    const rest = entries.filter(e => !used.has(e.type));
    if (rest.length) {
      const catEl = document.createElement('div');
      catEl.className = 'ns-lib-cat collapsed';
      const head = document.createElement('button');
      head.type = 'button';
      head.className = 'ns-lib-cat-head';
      head.innerHTML = '<span>其他 · ' + rest.length + '</span><span class="ns-lib-cat-caret" aria-hidden="true">▼</span>';
      head.addEventListener('click', () => catEl.classList.toggle('collapsed'));
      const body = document.createElement('div');
      body.className = 'ns-lib-cat-body widget-thumb-grid';
      rest.forEach(entry => body.appendChild(createThumb(entry)));
      catEl.appendChild(head);
      catEl.appendChild(body);
      grid.appendChild(catEl);
    }
  }

  function initWidgetLibrary(hooks) {
    const h = hooks || {};
    renderLibraryGrid('widgetLibraryGrid');

    const getActiveGrid = typeof h.getActiveGrid === 'function' ? h.getActiveGrid : () => null;
    const attachAndSelect = typeof h.attachAndSelect === 'function' ? h.attachAndSelect : () => {};
    const setGlobalEditMode = typeof h.setGlobalEditMode === 'function' ? h.setGlobalEditMode : () => {};
    const applyEditModeAll = typeof h.applyEditModeAll === 'function' ? h.applyEditModeAll : () => {};
    const getGlobalEditMode = typeof h.getGlobalEditMode === 'function' ? h.getGlobalEditMode : () => false;

    function addWidgetByType(type, px, py) {
      const t = String(type || '').trim();
      if (!t || !window.NSUtils) return;
      if (window.NSToast && (t === '3D散点' || t === '3D折线')) {
        window.NSToast.info('正在加载图表组件…', { duration: 1400 });
      }
      window.NSUtils.ensureWidgetReady(t, () => {
        try {
          if (!document.body.classList.contains('edit-mode')) {
            setGlobalEditMode(true);
            applyEditModeAll(true);
          }
        } catch {}
        const grid = getActiveGrid();
        if (!grid) return;
        const node = window.NSUtils.__createWidgetByType(grid, t, {}, {});
        if (!node) return;
        try { node.dataset.type = t; } catch {}
        try { window.NSInteract.attachOverlay(node); } catch {}

        if (typeof px === 'number' && typeof py === 'number') {
          try {
            const r0 = window.NSUtils.__readRectPx(node);
            const w = Math.max(40, Math.round(Number(r0.w) || 40));
            const hgt = Math.max(30, Math.round(Number(r0.h) || 30));
            const x = Math.max(0, Math.round(px - w / 2));
            const y = Math.max(0, Math.round(py - hgt / 2));
            window.NSUtils.__writeRectPx(node, { x, y, w, h: hgt });
          } catch {}
        }

        attachAndSelect(node, t);
      });
    }

    function addWidgetFromLibraryId(libId, px, py) {
      const type = window.NSWidgetManifest.getTypeByLibId(libId);
      if (type) addWidgetByType(type, px, py);
    }

    Array.from(document.querySelectorAll('#widgetLibraryGrid .widget-thumb')).forEach(btn => {
      const libId = String(btn.id || '').trim();
      const type = btn.dataset.widgetType || window.NSWidgetManifest.getTypeByLibId(libId);
      btn.setAttribute('draggable', 'true');
      btn.addEventListener('click', () => {
        try { if (!getGlobalEditMode()) return; } catch {}
        addWidgetByType(type);
      });
      btn.addEventListener('dragstart', (e) => {
        try {
          if (!e || !e.dataTransfer) return;
          draggingType = type;
          e.dataTransfer.effectAllowed = 'copy';
          try { e.dataTransfer.setData(MIME, libId); } catch {}
          try { e.dataTransfer.setData('text/plain', libId); } catch {}
          clearDragGhost();
          dragGhostEl = document.createElement('div');
          dragGhostEl.className = 'ns-drag-ghost';
          dragGhostEl.textContent = btn.title || type;
          document.body.appendChild(dragGhostEl);
          try { e.dataTransfer.setDragImage(dragGhostEl, 36, 18); } catch {}
          const c = getActiveCanvasEl();
          if (c) c.classList.add('ns-drop-over');
        } catch {}
      });
      btn.addEventListener('dragend', () => {
        draggingType = '';
        clearDragGhost();
        clearDropGhost();
        try {
          const c = getActiveCanvasEl();
          if (c) c.classList.remove('ns-drop-over');
        } catch {}
      });
    });

    const host = document.getElementById('tabsContent');
    if (host && !host.dataset.libDndBound) {
      host.dataset.libDndBound = '1';
      host.addEventListener('dragover', (e) => {
        try {
          if (!e || !e.dataTransfer) return;
          const types = Array.from(e.dataTransfer.types || []);
          if (!types.includes(MIME) && !types.includes('text/plain')) return;
          e.preventDefault();
          try { e.dataTransfer.dropEffect = 'copy'; } catch {}
          const c = getActiveCanvasEl();
          if (c) {
            c.classList.add('ns-drop-over');
            updateDropGhost(c, e.clientX, e.clientY, draggingType);
          }
        } catch {}
      });
      host.addEventListener('dragleave', (e) => {
        try {
          if (e && e.relatedTarget && host.contains(e.relatedTarget)) return;
          const c = getActiveCanvasEl();
          if (c) c.classList.remove('ns-drop-over');
          clearDropGhost();
        } catch {}
      });
      host.addEventListener('drop', (e) => {
        try {
          if (!e || !e.dataTransfer) return;
          const raw = String((e.dataTransfer.getData(MIME) || e.dataTransfer.getData('text/plain') || '')).trim();
          if (!raw) return;
          e.preventDefault();
          const c = getActiveCanvasEl();
          if (!c) return;
          c.classList.remove('ns-drop-over');
          clearDropGhost();
          clearDragGhost();
          draggingType = '';
          const p = window.NSCanvas.__clientToCanvasDesignXY(c, e.clientX, e.clientY);
          addWidgetFromLibraryId(raw, p.x, p.y);
        } catch {}
      });
    }

    window.NSWidgetLibrary = {
      addWidgetByType,
      addWidgetFromLibraryId,
      getTypeByLibId: (id) => window.NSWidgetManifest.getTypeByLibId(id)
    };
  }

  window.NSWidgetLibraryInit = initWidgetLibrary;
})();
