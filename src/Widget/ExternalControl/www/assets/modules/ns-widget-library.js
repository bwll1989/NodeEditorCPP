// ns-widget-library.js —— 从 manifest 动态渲染控件库，并绑定拖拽/点击添加
(function() {
  'use strict';

  const MIME = 'text/x-ns-widget';

  const ICONS = {
    addButtonWidget: '<rect x="4" y="7" width="16" height="10" rx="2"></rect><path d="M8 12h8"></path>',
    addSliderWidget: '<path d="M5 12h14"></path><circle cx="10" cy="12" r="3"></circle>',
    addFloatSliderWidget: '<path d="M5 12h14"></path><circle cx="14" cy="12" r="3"></circle><path d="M7 8v8"></path>',
    addVSliderWidget: '<path d="M12 5v14"></path><circle cx="12" cy="10" r="3"></circle>',
    addVFloatSliderWidget: '<path d="M12 5v14"></path><circle cx="12" cy="14" r="3"></circle><path d="M8 7h8"></path>',
    addCheckboxWidget: '<rect x="6" y="6" width="12" height="12" rx="2"></rect><path d="M9 12l2 2 4-5"></path>',
    addSwitchWidget: '<rect x="5" y="9" width="14" height="6" rx="3"></rect><circle cx="10" cy="12" r="2"></circle>',
    addInputWidget: '<rect x="5" y="8" width="14" height="8" rx="2"></rect><path d="M9 10v4"></path>',
    addToggleButtonWidget: '<path d="M7 9h10"></path><path d="M7 15h10"></path><path d="M9 9v6"></path>',
    addDividerWidget: '<path d="M6 12h12"></path>',
    addVDividerWidget: '<path d="M12 6v12"></path>',
    addLabelWidget: '<path d="M8 7h8"></path><path d="M12 7v10"></path><path d="M9 17h6"></path>',
    addKnobWidget: '<circle cx="12" cy="12" r="7"></circle><path d="M12 12l4-3"></path>',
    addTimecodeWidget: '<rect x="4" y="6" width="16" height="12" rx="2"></rect><path d="M8 10h2"></path><path d="M14 10h2"></path>',
    addTimelineWidget: '<path d="M4 12h16"></path><circle cx="6" cy="12" r="2"></circle><circle cx="12" cy="12" r="2"></circle><circle cx="18" cy="12" r="2"></circle>',
    addNumberWidget: '<path d="M4 12h16"></path><path d="M10 6v12"></path>',
    addScatter3DWidget: '<circle cx="8" cy="14" r="1.5"></circle><circle cx="12" cy="9" r="1.5"></circle><circle cx="16" cy="13" r="1.5"></circle><path d="M5 18h14"></path><path d="M5 18V7"></path>',
    addLine3DWidget: '<path d="M5 16 L9 10 L13 13 L18 7"></path><circle cx="5" cy="16" r="1.2"></circle><circle cx="9" cy="10" r="1.2"></circle><circle cx="13" cy="13" r="1.2"></circle><circle cx="18" cy="7" r="1.2"></circle>',
    addFrameWidget: '<rect x="5" y="6" width="14" height="12" rx="2"></rect>',
    addLinkWidget: '<path d="M10 13a5 5 0 0 1 0-7l2-2a5 5 0 0 1 7 7l-2 2"></path><path d="M14 11a5 5 0 0 1 0 7l-2 2a5 5 0 0 1-7-7l2-2"></path>'
  };

  function getActiveCanvasEl() {
    try {
      const NS = window.NS;
      const info = NS && NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
      return info && info.canvasEl ? info.canvasEl : null;
    } catch { return null; }
  }

  function renderLibraryGrid(containerId) {
    const grid = document.getElementById(containerId || 'widgetLibraryGrid');
    if (!grid || !window.NSWidgetManifest) return;
    grid.innerHTML = '';
    window.NSWidgetManifest.libraryEntries().forEach(entry => {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.id = entry.libId;
      btn.className = 'widget-thumb';
      btn.dataset.widgetType = entry.type;
      const icon = document.createElement('span');
      icon.className = 'widget-thumb-icon';
      icon.innerHTML = '<svg viewBox="0 0 24 24" aria-hidden="true">' + (ICONS[entry.libId] || '<circle cx="12" cy="12" r="6"></circle>') + '</svg>';
      const label = document.createElement('span');
      label.className = 'widget-thumb-label';
      label.textContent = entry.thumbLabel || entry.type;
      btn.appendChild(icon);
      btn.appendChild(label);
      grid.appendChild(btn);
    });
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
          e.dataTransfer.effectAllowed = 'copy';
          try { e.dataTransfer.setData(MIME, libId); } catch {}
          try { e.dataTransfer.setData('text/plain', libId); } catch {}
          const c = getActiveCanvasEl();
          if (c) c.classList.add('ns-drop-over');
        } catch {}
      });
      btn.addEventListener('dragend', () => {
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
          if (c) c.classList.add('ns-drop-over');
        } catch {}
      });
      host.addEventListener('dragleave', () => {
        try {
          const c = getActiveCanvasEl();
          if (c) c.classList.remove('ns-drop-over');
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
