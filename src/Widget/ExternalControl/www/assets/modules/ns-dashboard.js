// ns-dashboard.js —— initDashboard 及其内部所有函数：标签页管理、选区、分组、拖拽、对齐、编辑模式、撤销恢复、复制粘贴
(function() {
  'use strict';
  const NS = window.NS;
  const NSCanvas = window.NSCanvas;
  const NSWsSync = window.NSWsSync;
  const NSUtils = window.NSUtils;

  function initDashboard() {
    // 提升变量声明到顶部，避免 TDZ 问题
    let currentSelected = null;
    let anchorSelected = null;
    const selectedNodes = new Set();
    window.__selectedGroupId = '';
    try { NSUtils.mergeLeftSidebarIntoRight(); } catch {}
    try { bindMousePosTracking(); } catch {}

    // 函数级注释：生成当前页的快照（设计尺寸与控件列表）
    function __snapshotGrid(grid) {
      try {
        const info = NS.grids.get(NS.activeTabId);
        const design = (info && info.design) ? info.design : { width: EPWidgets.layoutDefaults.designWidth, height: EPWidgets.layoutDefaults.designHeight, bgColor: EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc' };
        const items = NSUtils.collectGridItems(grid);
        return { design, items };
      } catch { return { design: {}, items: [] }; }
    }
    // 函数级注释：将当前页快照压入历史（去重 & 限制长度）
    function pushHistorySnapshot(tid, grid) {
      try {
        if (!tid || !grid) return;
        const snap = __snapshotGrid(grid);
        const key = 'ns_hist_' + tid;
        const past = Array.isArray(JSON.parse(localStorage.getItem(key) || '[]')) ? JSON.parse(localStorage.getItem(key) || '[]') : [];
        const s = JSON.stringify(snap);
        const last = past.length ? JSON.stringify(past[past.length - 1]) : '';
        if (s !== last) {
          past.push(snap);
          while (past.length > 30) past.shift();
          localStorage.setItem(key, JSON.stringify(past));
          localStorage.removeItem(key + '_future');
        }
      } catch {}
    }
    // 函数级注释：开始一次分页渲染，递增渲染序号以便丢弃旧的异步回调
    function __beginTabRender(tid) {
      const info = tid ? NS.grids.get(tid) : null;
      if (!info) return 0;
      info.renderSeq = (Number(info.renderSeq) || 0) + 1;
      return info.renderSeq;
    }

    // 函数级注释：判断某次渲染回调是否仍为最新
    function __isTabRenderCurrent(tid, seq) {
      const info = tid ? NS.grids.get(tid) : null;
      if (!info) return false;
      return (Number(info.renderSeq) || 0) === (Number(seq) || 0);
    }

    // 暴露到全局，供 loadLayout 等外部函数使用
    try { window.__beginTabRender = __beginTabRender; } catch {}
    try { window.__isTabRenderCurrent = __isTabRenderCurrent; } catch {}

    // 函数级注释：应用快照到当前页（带渲染序号防重复）
    function applySnapshot(tid, data) {
      try {
        const info = NS.grids.get(tid);
        if (!info) return;
        const grid = info.grid;
        NSCanvas.clearLayoutContainer(grid);
        const seq = __beginTabRender(tid);
        NSCanvas.applyPageDesign(tid, data.design);
        NSUtils.ensureWidgetTypesReady((data.items || []).map(s => s && s.type), () => {
          if (!__isTabRenderCurrent(tid, seq)) return;
          (data.items || []).forEach(spec => { NSUtils.createWidgetFromSpec(grid, spec); });
          try { NSWsSync.queryAllStatuses(); } catch {}
        });
        try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
      } catch {}
    }
    // 函数级注释：撤销/恢复（基于本地历史栈）
    function undoLayout() {
      try {
        const tid = NS.activeTabId; if (!tid) return;
        const key = 'ns_hist_' + tid; const fkey = key + '_future';
        const past = Array.isArray(JSON.parse(localStorage.getItem(key) || '[]')) ? JSON.parse(localStorage.getItem(key) || '[]') : [];
        if (past.length < 2) return;
        const cur = past.pop(); const prev = past[past.length - 1];
        localStorage.setItem(key, JSON.stringify(past));
        const future = Array.isArray(JSON.parse(localStorage.getItem(fkey) || '[]')) ? JSON.parse(localStorage.getItem(fkey) || '[]') : [];
        future.push(cur); localStorage.setItem(fkey, JSON.stringify(future));
        applySnapshot(tid, prev);
      } catch {}
    }
    function redoLayout() {
      try {
        const tid = NS.activeTabId; if (!tid) return;
        const key = 'ns_hist_' + tid; const fkey = key + '_future';
        const past = Array.isArray(JSON.parse(localStorage.getItem(key) || '[]')) ? JSON.parse(localStorage.getItem(key) || '[]') : [];
        const future = Array.isArray(JSON.parse(localStorage.getItem(fkey) || '[]')) ? JSON.parse(localStorage.getItem(fkey) || '[]') : [];
        if (future.length < 1) return;
        const snap = future.pop(); localStorage.setItem(fkey, JSON.stringify(future));
        past.push(snap); localStorage.setItem(key, JSON.stringify(past));
        applySnapshot(NS.activeTabId, snap);
      } catch {}
    }
    // 函数级注释：复制/粘贴当前选择
    function __copySelection() {
      const wrap = Array.from(document.querySelectorAll('#tabsContent [data-tab-id]')).find(el => el.style.display !== 'none');
      const canvas = wrap ? wrap.querySelector('.pixel-canvas') : null;
      if (!canvas) return;
      let nodes = Array.from(selectedNodes);
      const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.size === 'number') ? window.__selectedGroupIds : null;
      if ((!nodes || nodes.length === 0) && gs && gs.size > 0) {
        nodes = [];
        Array.from(gs).forEach(gid => {
          const m = NSUtils.__getGroupMembers(canvas, gid);
          nodes.push(...m);
        });
      }
      if (!nodes || nodes.length === 0) return;
      const specs = nodes.map(n => {
        const props = EPWidgets.getProps(n);
        const r = NSUtils.__readRectPx(n);
        const type = String(n.dataset.type || '');
        const chain = NSUtils.__getNodeGroupChain(n);
        const gid = (chain && chain.length) ? String(chain[chain.length - 1] || '').trim() : '';
        const spec = { type, props, rect: { x: r.x, y: r.y, w: r.w, h: r.h } };
        if (gid) spec.groupId = gid;
        if (chain && chain.length > 1) spec.groupChain = chain.join(',');
        return spec;
      });
      const bbox = specs.reduce((b, s) => ({ x: Math.min(b.x, s.rect.x), y: Math.min(b.y, s.rect.y) }), { x: Infinity, y: Infinity });
      const clip = { items: specs, bbox, tabId: NS.activeTabId };
      window.__clipboard = clip;
      try { localStorage.setItem('ns_clipboard', JSON.stringify(clip)); } catch {}
    }
    function __pasteSelection() {
      try {
        if (window.__nsPasteLock) return;
        window.__nsPasteLock = true;
        setTimeout(() => { try { window.__nsPasteLock = false; } catch {} }, 0);

        const clip = window.__clipboard || JSON.parse(localStorage.getItem('ns_clipboard') || '{}');
        if (!clip || !Array.isArray(clip.items) || clip.items.length === 0) return;
        const grid = getActiveGrid(); if (!grid) return;
        const pos = window.__lastMouseCanvasPos || null;
        const bx = (clip.bbox && Number.isFinite(clip.bbox.x)) ? clip.bbox.x : 0;
        const by = (clip.bbox && Number.isFinite(clip.bbox.y)) ? clip.bbox.y : 0;
        const targetX = (pos && Number.isFinite(pos.x)) ? pos.x : (bx + 20);
        const targetY = (pos && Number.isFinite(pos.y)) ? pos.y : (by + 20);
        const dx = targetX - bx;
        const dy = targetY - by;
        const gidMap = {};
        (clip.items || []).forEach(spec => {
          const chainStr = String(spec.groupChain || '').trim();
          const gids = chainStr ? chainStr.split(',').map(s => String(s || '').trim()).filter(Boolean) : [];
          const og = String(spec.groupId || '').trim();
          if (og && gids.indexOf(og) < 0) gids.push(og);
          gids.forEach(id => {
            if (id && !gidMap[id]) gidMap[id] = 'g_' + Math.random().toString(36).slice(2, 8) + Date.now().toString(36).slice(-4);
          });
        });
        NSUtils.ensureWidgetTypesReady((clip.items || []).map(s => s && s.type), () => {
          clip.items.forEach(spec => {
            const r = { x: (spec.rect.x || 0) + dx, y: (spec.rect.y || 0) + dy, w: spec.rect.w, h: spec.rect.h };
            const chainStr = String(spec.groupChain || '').trim();
            let chain = chainStr ? chainStr.split(',').map(s => String(s || '').trim()).filter(Boolean) : [];
            if (!chain.length) {
              const og = String(spec.groupId || '').trim();
              if (og) chain = [og];
            }
            const mapped = chain.map(id => String(gidMap[id] || id));
            const leaf = mapped.length ? String(mapped[mapped.length - 1] || '').trim() : '';
            const s = { type: spec.type, props: spec.props, rect: r };
            if (leaf) s.groupId = leaf;
            if (mapped.length > 1) s.groupChain = mapped.join(',');
            NSUtils.createWidgetFromSpec(grid, s);
          });
        });
        pushHistorySnapshot(NS.activeTabId, grid);
        try { window.__saveLayoutLocal(NS.activeTabId, grid); } catch {}
      } catch {}
    }
    window.__copySelection = __copySelection;
    window.__pasteSelection = __pasteSelection;

    // 函数级注释：获取当前激活页的画布容器
    function getActiveGrid() {
      if (!NS.activeTabId) return null;
      const info = NS.grids.get(NS.activeTabId);
      return info ? info.grid : null;
    }

    // 函数级注释：将布局保存到本地存储
    function saveLayoutLocal(tid, grid) {
      try {
        const items = NSUtils.collectGridItems(grid);
        const info = NS.grids.get(tid);
        const design = (info && info.design) ? info.design : { width: EPWidgets.layoutDefaults.designWidth, height: EPWidgets.layoutDefaults.designHeight, bgColor: EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc' };
        localStorage.setItem('ns_layout_' + tid, JSON.stringify({ design, items }));
        pushHistorySnapshot(tid, grid);
      } catch (err) {
        console.error('saveLayoutLocal failed', err);
      }
    }
    window.__saveLayoutLocal = saveLayoutLocal;

    // 函数级注释：从本地存储加载布局
    function loadLayoutLocal(grid, tid, silent = false) {
      try {
        const raw = localStorage.getItem('ns_layout_' + tid);
        if (!raw) {
          if (!silent) alert('当前页面没有本地保存的布局');
          return false;
        }
        const data = JSON.parse(raw);
        const items = (data && Array.isArray(data.items)) ? data.items : [];
        const design = (data && data.design && typeof data.design === 'object') ? data.design : null;
        if (design) NSCanvas.applyPageDesign(tid, design);

        if (items.length === 0) {
          if (!silent) alert('当前页面本地布局为空');
          return false;
        }

        NSCanvas.clearLayoutContainer(grid);
        const seq = __beginTabRender(tid);
        try { const info0 = NS.grids.get(tid); if (info0) info0.rendering = true; } catch {}
        NSUtils.ensureWidgetTypesReady((items || []).map(s => s && s.type), () => {
          if (!__isTabRenderCurrent(tid, seq)) return;
          items.forEach(spec => { NSUtils.createWidgetFromSpec(grid, spec); });
          try { const info2 = NS.grids.get(tid); if (info2) { info2.loaded = true; info2.rendering = false; } } catch {}
          try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
          try { NSWsSync.queryAllStatuses(); } catch {}
        });
        return true;
      } catch (err) {
        console.error('loadLayoutLocal failed', err);
        if (!silent) alert('本地布局加载失败: ' + err);
        return false;
      }
    }

    // 函数级注释：创建一个新标签页并初始化像素画布
    function createTab(name, id, opts) {
      const tid = id || ('page_' + Math.random().toString(36).slice(2, 8));
      const tabsList = document.getElementById('tabsList');
      const tabsContent = document.getElementById('tabsContent');
      const tabBtn = document.createElement('button');
      tabBtn.className = 'nav-link btn btn-sm btn-outline-secondary';
      renderTabLabel(tabBtn, name, tid);
      tabBtn.dataset.tabId = tid;
      tabBtn.onclick = () => switchTab(tid);
      tabBtn.ondblclick = (e) => {
        e.stopPropagation();
        e.preventDefault();
        const info = NS.grids.get(tid);
        const oldName = (info && info.name) || name || '';
        const input = document.createElement('input');
        input.type = 'text';
        input.className = 'form-control form-control-sm';
        input.style.width = '140px';
        input.value = oldName;
        tabBtn.innerHTML = '';
        tabBtn.appendChild(input);
        input.focus();
        input.select();
        const finish = (commit) => {
          const newName = (input.value || '').trim();
          tabBtn.removeChild(input);
          const finalName = commit && newName ? newName : oldName;
          renderTabLabel(tabBtn, finalName, tid);
          if (info) info.name = finalName;
          const idx = NS.tabs.findIndex(t => t.id === tid);
          if (idx >= 0) {
            NS.tabs[idx].name = finalName;
            try { localStorage.setItem('ns_tabs', JSON.stringify(NS.tabs)); } catch {}
          }
        };
        input.onblur = () => finish(true);
        input.onkeydown = (evt) => {
          if (evt.key === 'Enter') finish(true);
          else if (evt.key === 'Escape') finish(false);
        };
      };
      tabsList.appendChild(tabBtn);

      const pageWrap = document.createElement('div');
      pageWrap.style.position = 'absolute';
      pageWrap.style.inset = '0';
      pageWrap.style.display = 'none';
      pageWrap.style.overflow = 'hidden';
      pageWrap.dataset.tabId = tid;

      const viewportEl = document.createElement('div');
      viewportEl.className = 'canvas-viewport';
      pageWrap.appendChild(viewportEl);

      const canvasEl = document.createElement('div');
      canvasEl.className = 'pixel-canvas';
      viewportEl.appendChild(canvasEl);
      tabsContent.appendChild(pageWrap);

      const grid = { el: canvasEl, __pixel: true };
      NS.grids.set(tid, { name, tabBtn, pageWrap, viewportEl, grid, canvasEl, view: null, design: { width: EPWidgets.layoutDefaults.designWidth, height: EPWidgets.layoutDefaults.designHeight, bgColor: EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc' }, loaded: false, renderSeq: 0, rendering: false });
      NSCanvas.applyPageDesign(tid);
      try { NSCanvas.__initCanvasPanZoom(tid); } catch {}

      if (!NS.tabs.find(t => t.id === tid)) {
        NS.tabs.push({ id: tid, name });
        try {
          localStorage.setItem('ns_tabs', JSON.stringify(NS.tabs));
          if (!(opts && opts.skipSwitch)) localStorage.setItem('ns_active_tab', tid);
        } catch (err) { console.warn('Persist tabs failed', err); }
      }
      const skipLocal = opts && opts.skipLocalLoad;
      const skipSwitch = opts && opts.skipSwitch;
      if (!skipLocal) loadLayoutLocal(grid, tid, true);
      if (!skipSwitch) switchTab(tid);
      return tid;
    }
    window.__createTab = createTab;

    function renderTabLabel(tabBtn, name, tid) {
      tabBtn.textContent = name;
      const close = document.createElement('span');
      close.className = 'tab-close';
      close.title = '删除页面';
      close.textContent = '\u00d7';
      close.onclick = (e) => { e.stopPropagation(); deleteTab(tid); };
      try { close.style.display = getGlobalEditMode() ? '' : 'none'; } catch {}
      tabBtn.appendChild(close);
    }

    function deleteTab(tid) {
      try {
        if (!getGlobalEditMode()) { alert('请切换到编辑模式后删除页面'); return; }
        if (NS.grids.size <= 1) { alert('至少保留一个页面'); return; }
        const info = NS.grids.get(tid);
        if (!info) return;
        const nameLabel = (info && info.name) ? String(info.name) : String(tid);
        if (!window.confirm('确认删除页面"' + nameLabel + '"？删除后不可恢复')) return;
        if (info.pageWrap && info.pageWrap.parentElement) {
          info.pageWrap.parentElement.removeChild(info.pageWrap);
        }
        if (info.tabBtn && info.tabBtn.parentElement) {
          info.tabBtn.parentElement.removeChild(info.tabBtn);
        }
        NS.grids.delete(tid);
        const idx = NS.tabs.findIndex(t => t.id === tid);
        if (idx >= 0) NS.tabs.splice(idx, 1);
        try { localStorage.removeItem('ns_layout_' + tid); } catch {}
        try { localStorage.setItem('ns_tabs', JSON.stringify(NS.tabs)); } catch {}
        if (NS.activeTabId === tid) {
          const next = NS.tabs[0] ? NS.tabs[0].id : (NS.grids.keys().next().value || null);
          if (next) {
            try { localStorage.setItem('ns_active_tab', next); } catch {}
            switchTab(next);
          } else {
            NS.activeTabId = null;
          }
        }
      } catch (err) {
        alert('删除页面失败: ' + err);
      }
    }

    function switchTab(id) {
      clearSelection();
      NS.activeTabId = id;
      NS.grids.forEach((info, tid) => {
        const active = tid === id;
        info.pageWrap.style.display = active ? 'block' : 'none';
        info.tabBtn.classList.toggle('active', active);
      });
      const info = NS.grids.get(id);
      if (info && !info.loaded && !info.rendering) {
        const ok = loadLayoutLocal(info.grid, id, true);
        info.loaded = true;
        if (!ok) {
          try { NSInteract.loadLayout(info.grid, true); } catch {}
        }
        try { pushHistorySnapshot(id, info.grid); } catch {}
      }
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
      try {
        const key = 'ns_hist_' + id;
        const past = Array.isArray(JSON.parse(localStorage.getItem(key) || '[]')) ? JSON.parse(localStorage.getItem(key) || '[]') : [];
        if (!past || past.length === 0) pushHistorySnapshot(id, info.grid);
      } catch {}
    }
    window.__switchTab = switchTab;

    // 控件添加按钮
    document.getElementById('addButtonWidget').onclick = () => NSUtils.ensureWidgetReady('按钮', ()=>attachAndSelect(EPWidgets.createEPButtonWidget(getActiveGrid()), '按钮'));
    document.getElementById('addSliderWidget').onclick = () => NSUtils.ensureWidgetReady('滑块', ()=>attachAndSelect(EPWidgets.createEPSliderWidget(getActiveGrid()), '滑块'));
    document.getElementById('addFloatSliderWidget').onclick = () => NSUtils.ensureWidgetReady('浮点滑块', ()=>attachAndSelect(EPWidgets.createEPFloatSliderWidget(getActiveGrid()), '浮点滑块'));
    document.getElementById('addVSliderWidget').onclick = () => NSUtils.ensureWidgetReady('竖向滑动条', ()=>attachAndSelect(EPWidgets.createEPVSliderWidget(getActiveGrid()), '竖向滑动条'));
    document.getElementById('addVFloatSliderWidget').onclick = () => NSUtils.ensureWidgetReady('竖向浮点滑块', ()=>attachAndSelect(EPWidgets.createEPVFloatSliderWidget(getActiveGrid()), '竖向浮点滑块'));
    document.getElementById('addCheckboxWidget').onclick = () => NSUtils.ensureWidgetReady('勾选', ()=>attachAndSelect(EPWidgets.createEPCheckboxWidget(getActiveGrid()), '勾选'));
    document.getElementById('addSwitchWidget').onclick = () => NSUtils.ensureWidgetReady('开关', ()=>attachAndSelect(EPWidgets.createEPSwitchWidget(getActiveGrid()), '开关'));
    document.getElementById('addInputWidget').onclick = () => NSUtils.ensureWidgetReady('输入框', ()=>attachAndSelect(EPWidgets.createEPInputWidget(getActiveGrid()), '输入框'));
    document.getElementById('addToggleButtonWidget').onclick = () => NSUtils.ensureWidgetReady('切换按钮', ()=>attachAndSelect(EPWidgets.createEPToggleButtonWidget(getActiveGrid()), '切换按钮'));
    document.getElementById('addDividerWidget').onclick = () => NSUtils.ensureWidgetReady('分割线', ()=>attachAndSelect(EPWidgets.createEPDividerWidget(getActiveGrid()), '分割线'));
    document.getElementById('addVDividerWidget').onclick = () => NSUtils.ensureWidgetReady('竖向分割线', ()=>attachAndSelect(EPWidgets.createEPVDividerWidget(getActiveGrid()), '竖向分割线'));
    document.getElementById('addLabelWidget').onclick = () => NSUtils.ensureWidgetReady('标签', ()=>attachAndSelect(EPWidgets.createEPLabelWidget(getActiveGrid()), '标签'));
    document.getElementById('addKnobWidget').onclick = () => NSUtils.ensureWidgetReady('旋钮', ()=>attachAndSelect(EPWidgets.createEPKnobWidget(getActiveGrid()), '旋钮'));
    document.getElementById('addTimecodeWidget').onclick = () => NSUtils.ensureWidgetReady('时间码', ()=>attachAndSelect(EPWidgets.createEPTimecodeWidget(getActiveGrid()), '时间码'));
    document.getElementById('addTimelineWidget').onclick = () => NSUtils.ensureWidgetReady('时间线', ()=>attachAndSelect(EPWidgets.createEPTimelineWidget(getActiveGrid()), '时间线'));
    document.getElementById('addNumberWidget').onclick = () => NSUtils.ensureWidgetReady('数值', ()=>attachAndSelect(EPWidgets.createEPNumberWidget(getActiveGrid()), '数值'));
    document.getElementById('addFrameWidget').onclick = () => NSUtils.ensureWidgetReady('Frame', ()=>attachAndSelect(EPWidgets.createEPFrameWidget(getActiveGrid()), 'Frame'));
    document.getElementById('addLinkWidget').onclick = () => NSUtils.ensureWidgetReady('超链接', ()=>attachAndSelect(EPWidgets.createEPLinkWidget(getActiveGrid()), '超链接'));

    // 拖拽到画布创建
    (function bindWidgetLibraryDragDrop(){
      const LIB_ID_TO_TYPE = {
        addButtonWidget: '按钮', addSliderWidget: '滑块', addFloatSliderWidget: '浮点滑块',
        addVSliderWidget: '竖向滑动条', addVFloatSliderWidget: '竖向浮点滑块',
        addCheckboxWidget: '勾选', addSwitchWidget: '开关', addInputWidget: '输入框',
        addToggleButtonWidget: '切换按钮', addDividerWidget: '分割线', addVDividerWidget: '竖向分割线',
        addLabelWidget: '标签', addKnobWidget: '旋钮', addTimecodeWidget: '时间码',
        addTimelineWidget: '时间线', addNumberWidget: '数值', addFrameWidget: 'Frame', addLinkWidget: '超链接'
      };
      const MIME = 'text/x-ns-widget';

      function getActiveCanvasEl() {
        try {
          const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
          return info && info.canvasEl ? info.canvasEl : null;
        } catch { return null; }
      }

      function clientToDesignXY(canvasEl, clientX, clientY) {
        return NSCanvas.__clientToCanvasDesignXY(canvasEl, clientX, clientY);
      }

      function addWidgetFromLibraryId(libId, px, py) {
        const id = String(libId || '').trim();
        const type = LIB_ID_TO_TYPE[id] || '';
        if (!type) return;
        NSUtils.ensureWidgetReady(type, () => {
          try {
            if (!document.body.classList.contains('edit-mode')) {
              setGlobalEditMode(true);
              applyEditModeAll(true);
            }
          } catch {}
          const grid = getActiveGrid();
          if (!grid) return;
          const node = NSUtils.__createWidgetByType(grid, type, {}, {});
          if (!node) return;
          try { node.dataset.type = type; } catch {}
          try { NSInteract.attachOverlay(node); } catch {}
          try {
            const r0 = NSUtils.__readRectPx(node);
            const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
            const boundW = info && info.design ? Number(info.design.width) : NaN;
            const boundH = info && info.design ? Number(info.design.height) : NaN;
            const w = Math.max(40, Math.round(Number(r0.w) || 40));
            const h = Math.max(30, Math.round(Number(r0.h) || 30));
            let x = Math.round((Number(px) || 0) - w / 2);
            let y = Math.round((Number(py) || 0) - h / 2);
            if (Number.isFinite(boundW)) x = Math.max(0, Math.min(boundW - w, x));
            if (Number.isFinite(boundH)) y = Math.max(0, Math.min(boundH - h, y));
            NSUtils.__writeRectPx(node, { x, y, w, h });
          } catch {}
          if (node && typeof window.__selectNode === 'function') {
            window.__selectNode(node, { toggle: false });
          }
          try { if (typeof window.__saveLayoutLocal === 'function' && NS.activeTabId) window.__saveLayoutLocal(NS.activeTabId, grid); } catch {}
        });
      }

      function initLibraryButtons() {
        const btns = Array.from(document.querySelectorAll('.widget-thumb-grid .widget-thumb'));
        btns.forEach(btn => {
          try {
            const id = String(btn && btn.id ? btn.id : '').trim();
            if (!id) return;
            if (!LIB_ID_TO_TYPE[id]) return;
            btn.setAttribute('draggable', 'true');
            btn.addEventListener('dragstart', (e) => {
              try {
                if (!e || !e.dataTransfer) return;
                e.dataTransfer.effectAllowed = 'copy';
                try { e.dataTransfer.setData(MIME, id); } catch {}
                try { e.dataTransfer.setData('text/plain', id); } catch {}
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
          } catch {}
        });
      }

      function initCanvasDropTarget() {
        const host = document.getElementById('tabsContent');
        if (!host) return;
        if (host.dataset.libDndBound) return;
        host.dataset.libDndBound = '1';
        host.addEventListener('dragover', (e) => {
          try {
            if (!e || !e.dataTransfer) return;
            const types = Array.from(e.dataTransfer.types || []);
            const ok = types.includes(MIME) || types.includes('text/plain');
            if (!ok) return;
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
            const p = clientToDesignXY(c, e.clientX, e.clientY);
            addWidgetFromLibraryId(raw, p.x, p.y);
          } catch {}
        });
      }
      initLibraryButtons();
      initCanvasDropTarget();
    })();

    // 对齐/分组按钮
    (function bindAlignActions(){
      const safe = (id, fn) => {
        const el = document.getElementById(id);
        if (el) el.onclick = fn;
      };
      safe('alignLeft', () => alignSelected('left'));
      safe('alignRight', () => alignSelected('right'));
      safe('alignTop', () => alignSelected('top'));
      safe('alignBottom', () => alignSelected('bottom'));
      safe('alignCenterH', () => alignSelected('hcenter'));
      safe('alignCenterV', () => alignSelected('vcenter'));
      safe('distributeH', () => distributeSelected('h'));
      safe('distributeV', () => distributeSelected('v'));
      safe('sameSize', () => sameSizeSelected());
      safe('groupSelected', () => groupSelected());
      safe('ungroupSelected', () => ungroupSelected());
      safe('undoBtn', () => undoLayout());
      safe('redoBtn', () => redoLayout());
    })();

    // 新增页面按钮
    document.getElementById('sidebarAddTab').onclick = () => {
      try { if (!getGlobalEditMode()) return; } catch {}
      const base = '页面';
      const idx = NS.grids.size + 1;
      const tid = createTab(base + idx);
      try {
        localStorage.setItem('ns_active_tab', tid);
        localStorage.setItem('ns_tabs', JSON.stringify(NS.tabs));
      } catch {}
    };

    // 侧边栏折叠
    document.querySelectorAll('.sidebar-section-header').forEach(header => {
      header.onclick = () => {
        header.parentElement.classList.toggle('collapsed');
      };
    });

    // 移动端/编辑模式判断
    function isMobileEditDisabled() {
      try {
        const ua = String(navigator.userAgent || '').toLowerCase();
        const mobileUa = /iphone|ipad|ipod|android|mobile/.test(ua);
        const coarse = !!(window.matchMedia && window.matchMedia('(pointer: coarse)').matches);
        const narrow = !!(window.matchMedia && window.matchMedia('(max-width: 900px)').matches);
        return mobileUa || (coarse && narrow);
      } catch { return false; }
    }

    function getGlobalEditMode() {
      if (isMobileEditDisabled()) return false;
      try {
        const raw = localStorage.getItem('ns_edit_mode');
        if (raw === null || raw === undefined) return false;
        return raw === 'true';
      } catch { return false; }
    }

    function applySidebarsCollapsed(collapsed) {
      const sideRight = document.querySelector('.sidebar-right');
      if (collapsed) {
        if (sideRight) sideRight.classList.add('collapsed');
      } else {
        if (sideRight) sideRight.classList.remove('collapsed');
      }
    }

    function applyEditModeAll(isEdit) {
      try {
        NS.grids.forEach(info => {
          if (info && info.grid && typeof info.grid.setStatic === 'function') info.grid.setStatic(!isEdit);
        });
      } catch {}
      if (isEdit) {
        document.body.classList.add('edit-mode');
        document.body.classList.remove('view-mode');
      } else {
        document.body.classList.add('view-mode');
        document.body.classList.remove('edit-mode');
        clearSelection();
      }
      applySidebarsCollapsed(!isEdit);
      try {
        if (isEdit) {
          if (!window.__splitInstance) NSWsSync.setupSplitPanels();
        } else {
          if (window.__splitInstance && typeof window.__splitInstance.destroy === 'function') {
            try { window.__splitInstance.destroy(false, false); } catch {}
            window.__splitInstance = null;
          }
        }
      } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
      try {
        document.querySelectorAll('.tab-close').forEach(el => {
          el.style.display = isEdit ? '' : 'none';
        });
      } catch {}
      try {
        const addBtn = document.getElementById('sidebarAddTab');
        if (addBtn) addBtn.style.display = isEdit ? '' : 'none';
      } catch {}
      try { updatePropPanel(currentSelected); } catch {}
    }

    function setGlobalEditMode(isEdit) {
      const next = isMobileEditDisabled() ? false : !!isEdit;
      try { localStorage.setItem('ns_edit_mode', next ? 'true' : 'false'); } catch {}
      applyEditModeAll(next);
      const btn = document.getElementById('toggleEditMode');
      if (btn) {
        if (isMobileEditDisabled()) {
          btn.style.display = 'none';
        } else {
          btn.style.display = '';
          btn.textContent = next ? '退出编辑' : '编辑模式';
          btn.classList.toggle('active', !!next);
        }
      }
    }

    document.getElementById('toggleEditMode').onclick = async () => {
      const current = getGlobalEditMode();
      if (!current) {
        const ok = await NSWsSync.requireSettingAuth();
        if (!ok) return;
      }
      setGlobalEditMode(!current);
      if (current) { saveAllLayouts(); }
    };

    document.getElementById('openSettings').onclick = NSWsSync.openSettingsPage;
    document.getElementById('resetCanvasView').onclick = () => { try { NSCanvas.resetCanvasView(); } catch {} };

    // 初始化Tabs
    try {
      const rawTabs = localStorage.getItem('ns_tabs');
      NS.tabs = Array.isArray(JSON.parse(rawTabs || '[]')) ? JSON.parse(rawTabs || '[]') : [];
    } catch { NS.tabs = []; }
    if (NS.tabs.length > 0) {
      NS.tabs.forEach(t => createTab(t.name, t.id));
      const savedActive = localStorage.getItem('ns_active_tab');
      if (savedActive && NS.grids.has(savedActive)) switchTab(savedActive);
      else switchTab(NS.tabs[0].id);
    } else {
      const defaultId = createTab('页面1');
      try {
        localStorage.setItem('ns_tabs', JSON.stringify([{ id: defaultId, name: '页面1' }]));
        localStorage.setItem('ns_active_tab', defaultId);
      } catch {}
    }

    // 恢复编辑模式
    (async () => {
      const mobileDisableEdit = isMobileEditDisabled();
      if (mobileDisableEdit) {
        try { localStorage.setItem('ns_edit_mode', 'false'); } catch {}
      }
      let initialEdit = mobileDisableEdit ? false : getGlobalEditMode();
      if (initialEdit) {
        const ok = await NSWsSync.requireSettingAuth();
        if (!ok) {
          initialEdit = false;
          try { localStorage.setItem('ns_edit_mode', 'false'); } catch {}
        }
      }
      applyEditModeAll(initialEdit);
      const btnInit = document.getElementById('toggleEditMode');
      if (btnInit) {
        if (mobileDisableEdit) {
          btnInit.style.display = 'none';
        } else {
          btnInit.style.display = '';
          btnInit.textContent = initialEdit ? '完成编辑' : '编辑模式';
          btnInit.classList.toggle('active', !!initialEdit);
        }
      }
    })();

    try { NSInteract.loadAllLayouts(); } catch {}

    // 选区与对齐逻辑
    function updateAlignHint() {
      const el = document.getElementById('alignSelectionHint');
      if (!el) return;
      const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.size === 'number') ? window.__selectedGroupIds : null;
      const gCount = gs ? Number(gs.size || 0) : 0;
      const nCount = Number(selectedNodes.size || 0);
      if (nCount > 0 && gCount > 0) { el.textContent = '已选组' + String(gCount) + ' 控件' + String(nCount); return; }
      if (nCount > 0) { el.textContent = '已选' + String(nCount); return; }
      if (gCount > 0) { el.textContent = '已选组' + String(gCount); return; }
      el.textContent = '已选0';
    }

    function clearSelection() {
      try { selectedNodes.forEach(n => { try { n.classList.remove('grid-selected'); } catch {} }); } catch {}
      selectedNodes.clear();
      currentSelected = null;
      anchorSelected = null;
      try { window.__selectedGroupIds = new Set(); window.__selectedGroupAnchor = ''; window.__groupDrillId = ''; } catch {}
      updateAlignHint();
      updatePropPanel(null);
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function removeFromSelection(node) {
      if (!node) return;
      try { selectedNodes.delete(node); } catch {}
      try { node.classList.remove('grid-selected'); } catch {}
      if (currentSelected === node) currentSelected = null;
      if (anchorSelected === node) anchorSelected = null;
      if (!anchorSelected && selectedNodes.size > 0) anchorSelected = selectedNodes.values().next().value || null;
      if (!currentSelected) currentSelected = anchorSelected || selectedNodes.values().next().value || null;
      updateAlignHint();
      updatePropPanel(currentSelected);
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }
    window.__removeFromSelection = removeFromSelection;

    function selectNode(node, opts) {
      if (!node) { clearSelection(); return; }
      const isToggle = !!(opts && opts.toggle);
      if (!isToggle) {
        try { window.__selectedGroupIds = new Set(); window.__selectedGroupAnchor = ''; } catch {}
        selectedNodes.forEach(n => { if (n !== node) n.classList.remove('grid-selected'); });
        selectedNodes.clear();
        selectedNodes.add(node);
        node.classList.add('grid-selected');
        currentSelected = node;
        anchorSelected = node;
        try { window.__groupDrillId = String((node && node.dataset ? node.dataset.groupId : '') || '').trim(); } catch {}
        updateAlignHint();
        updatePropPanel(node);
        try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
        return;
      }
      if (selectedNodes.has(node)) {
        selectedNodes.delete(node);
        node.classList.remove('grid-selected');
        if (anchorSelected === node) anchorSelected = null;
        if (currentSelected === node) currentSelected = null;
        if (!anchorSelected && selectedNodes.size > 0) anchorSelected = selectedNodes.values().next().value || null;
        if (!currentSelected) currentSelected = anchorSelected || selectedNodes.values().next().value || null;
        try { window.__groupDrillId = currentSelected ? String((currentSelected.dataset ? currentSelected.dataset.groupId : '') || '').trim() : ''; } catch {}
        updateAlignHint();
        updatePropPanel(currentSelected);
        return;
      }
      selectedNodes.add(node);
      node.classList.add('grid-selected');
      currentSelected = node;
      if (!anchorSelected) anchorSelected = selectedNodes.values().next().value || node;
      try { window.__groupDrillId = String((node && node.dataset ? node.dataset.groupId : '') || '').trim(); } catch {}
      updateAlignHint();
      updatePropPanel(currentSelected);
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function selectGroup(groupId, opts) {
      const gid = String(groupId || '').trim();
      if (!gid) return;
      const isToggle = !!(opts && opts.toggle);
      if (!isToggle) {
        try {
          selectedNodes.forEach(n => { try { n.classList.remove('grid-selected'); } catch {} });
          selectedNodes.clear();
          currentSelected = null;
          anchorSelected = null;
          window.__groupDrillId = '';
        } catch {}
      }
      try {
        const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.has === 'function') ? window.__selectedGroupIds : new Set();
        if (!isToggle) {
          gs.clear();
          gs.add(gid);
          window.__selectedGroupIds = gs;
          window.__selectedGroupAnchor = gid;
        } else {
          if (gs.has(gid)) gs.delete(gid); else gs.add(gid);
          window.__selectedGroupIds = gs;
          if (String(window.__selectedGroupAnchor || '').trim() && !gs.has(String(window.__selectedGroupAnchor || '').trim())) {
            window.__selectedGroupAnchor = (gs.size ? (gs.values().next().value || '') : '');
          } else if (!String(window.__selectedGroupAnchor || '').trim() && gs.size) {
            window.__selectedGroupAnchor = (gs.values().next().value || '');
          }
        }
      } catch {}
      updateAlignHint();
      if (!isToggle) updatePropPanel(null);
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    window.__selectNode = selectNode;
    window.__clearSelection = clearSelection;
    window.__selectGroup = selectGroup;

    // 分组命中测试
    (function bindGroupHitTestOnCanvas(){
      const host = document.getElementById('tabsContent');
      if (!host) return;
      if (host.dataset.groupHitBound) return;
      host.dataset.groupHitBound = '1';
      host.addEventListener('pointerdown', (e) => {
        try { if (NSInteract.__isMobileLike && NSInteract.__isMobileLike()) return; } catch {}
        try { if (!document.body.classList.contains('edit-mode')) return; } catch {}
        const canvas = (e.target && e.target.closest) ? e.target.closest('.pixel-canvas') : null;
        if (!canvas) return;
        if ((e.target && e.target.closest) ? e.target.closest('.grid-stack-item') : null) return;
        try { if (e && e.cancelable) e.preventDefault(); } catch {}
        const p0 = NSCanvas.__clientToCanvasDesignXY(canvas, e.clientX, e.clientY);
        const px = Number(p0.x) || 0;
        const py = Number(p0.y) || 0;
        const nodes = Array.from(canvas.querySelectorAll('.grid-stack-item[data-group-id], .grid-stack-item[data-group-chain]'));
        const groups = new Map();
        nodes.forEach(n => {
          const chain = NSUtils.__getNodeGroupChain(n);
          if (!chain || !chain.length) return;
          const r = NSUtils.__readRectPx(n);
          chain.forEach(gid => {
            const g = String(gid || '').trim();
            if (!g) return;
            if (!groups.has(g)) groups.set(g, []);
            groups.get(g).push(r);
          });
        });
        let hitGid = '';
        let bestArea = Infinity;
        groups.forEach((rects, gid) => {
          if (!rects || rects.length < 2) return;
          let minX = Infinity, minY = Infinity, maxR = -Infinity, maxB = -Infinity;
          rects.forEach(r => { minX = Math.min(minX, r.x); minY = Math.min(minY, r.y); maxR = Math.max(maxR, r.x + r.w); maxB = Math.max(maxB, r.y + r.h); });
          const x = minX - 8, y = minY - 8;
          const w = (maxR - minX) + 16;
          const h = (maxB - minY) + 16;
          if (px >= x && px <= x + w && py >= y && py <= y + h) {
            const area = Math.max(1, w * h);
            if (area < bestArea) { bestArea = area; hitGid = gid; }
          }
        });
        const toggle = !!(e && (e.ctrlKey || e.metaKey || e.shiftKey));
        if (!hitGid) {
          if (!toggle && typeof window.__clearSelection === 'function') window.__clearSelection();
          return;
        }
        const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.has === 'function') ? window.__selectedGroupIds : null;
        const wasGroupSelected = !!(gs && gs.has(hitGid));
        if (typeof window.__selectGroup === 'function') {
          if (!wasGroupSelected) window.__selectGroup(hitGid, { toggle });
        }
        let moved = false;
        const sx = Number(e.clientX) || 0;
        const sy = Number(e.clientY) || 0;
        const cleanup = () => {
          canvas.removeEventListener('pointermove', onMove);
          canvas.removeEventListener('pointerup', onUp);
          canvas.removeEventListener('pointercancel', onUp);
          try { canvas.releasePointerCapture(e.pointerId); } catch {}
        };
        const collectTargets = () => {
          const gs2 = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.has === 'function') ? window.__selectedGroupIds : null;
          const raw = (gs2 && gs2.size && gs2.has(hitGid)) ? Array.from(gs2) : [hitGid];
          const gids = NSUtils.__normalizeSelectedGroupIds(canvas, raw);
          const out = [];
          const seen = new Set();
          gids.forEach(g => {
            const g2 = String(g || '').trim();
            if (!g2) return;
            const members = NSUtils.__getGroupMembers(canvas, g2);
            members.forEach(n => { if (!n) return; if (seen.has(n)) return; seen.add(n); out.push(n); });
          });
          return out;
        };
        const onMove = (ev) => {
          const dx = (Number(ev.clientX) || 0) - sx;
          const dy = (Number(ev.clientY) || 0) - sy;
          if (!moved && (Math.abs(dx) > 3 || Math.abs(dy) > 3)) {
            moved = true;
            cleanup();
            const targets = collectTargets();
            if (targets && targets.length) NSInteract.__startDragNodes(canvas, targets, e);
          }
        };
        const onUp = () => { cleanup(); };
        try { canvas.setPointerCapture(e.pointerId); } catch {}
        canvas.addEventListener('pointermove', onMove);
        canvas.addEventListener('pointerup', onUp);
        canvas.addEventListener('pointercancel', onUp);
      }, true);
    })();

    function __getActiveCanvasEl() {
      try {
        if (NS.activeTabId) {
          const w = document.querySelector('#tabsContent [data-tab-id="' + String(NS.activeTabId).replace(/"/g, '\\"') + '"]');
          const c = w ? w.querySelector('.pixel-canvas') : null;
          if (c) return c;
        }
      } catch {}
      try {
        const wrap = Array.from(document.querySelectorAll('#tabsContent [data-tab-id]')).find(el => el.style.display !== 'none') || null;
        return wrap ? wrap.querySelector('.pixel-canvas') : null;
      } catch { return null; }
    }

    function bindMousePosTracking() {
      try {
        const host = document.getElementById('tabsContent');
        if (!host) return;
        if (host.dataset.mouseTrackBound) return;
        host.dataset.mouseTrackBound = '1';
        const update = (e) => {
          const canvas = (e.target && e.target.closest) ? e.target.closest('.pixel-canvas') : null;
          if (!canvas) return;
          const p = NSCanvas.__clientToCanvasDesignXY(canvas, e.clientX, e.clientY);
          const x = Number(p.x) || 0;
          const y = Number(p.y) || 0;
          window.__lastMouseCanvasPos = { x, y };
        };
        host.addEventListener('pointermove', update, true);
        host.addEventListener('pointerdown', update, true);
      } catch {}
    }

    function __getSelectedEntities(canvas) {
      const nodes = Array.from(selectedNodes);
      const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.size === 'number') ? window.__selectedGroupIds : null;
      const gids = NSUtils.__normalizeSelectedGroupIds(canvas, gs ? Array.from(gs) : []);
      const independentNodes = nodes.filter(n => !gids.some(gid => NSUtils.__nodeInGroup(n, gid)));
      const entities = [];
      gids.forEach(gid => {
        const r = NSUtils.__getGroupRect(canvas, gid);
        if (r) entities.push({ type: 'group', id: gid, r });
      });
      independentNodes.forEach(n => {
        const r = NSUtils.__readRectPx(n);
        if (r) entities.push({ type: 'node', node: n, r });
      });
      return { gids, independentNodes, entities };
    }

    function alignSelected(mode) {
      if (!getGlobalEditMode()) return;
      const grid = getActiveGrid(); if (!grid) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { gids, independentNodes, entities } = __getSelectedEntities(canvas);
      if (entities.length < 2) return;
      let refEntity = null, refRect = null;
      let refGid = String((window && window.__selectedGroupAnchor) || '').trim();
      let refNode = anchorSelected;
      if (refNode && independentNodes.includes(refNode)) {
        refEntity = { type: 'node', node: refNode };
        refRect = NSUtils.__readRectPx(refNode);
      } else if (refGid && gids.includes(refGid)) {
        refEntity = { type: 'group', id: refGid };
        refRect = NSUtils.__getGroupRect(canvas, refGid);
      } else {
        refEntity = entities[0]; refRect = refEntity.r;
      }
      if (!refRect) return;
      const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
      const boundW = info && info.design ? Number(info.design.width) : NaN;
      const boundH = info && info.design ? Number(info.design.height) : NaN;
      entities.forEach(it => {
        if (it.type === 'group' && it.id === refEntity.id) return;
        if (it.type === 'node' && it.node === refEntity.node) return;
        let x = it.r.x, y = it.r.y;
        if (mode === 'left') x = refRect.x;
        else if (mode === 'right') x = refRect.x + refRect.w - it.r.w;
        else if (mode === 'top') y = refRect.y;
        else if (mode === 'bottom') y = refRect.y + refRect.h - it.r.h;
        else if (mode === 'hcenter') x = refRect.x + Math.round((refRect.w - it.r.w) / 2);
        else if (mode === 'vcenter') y = refRect.y + Math.round((refRect.h - it.r.h) / 2);
        if (it.type === 'group') {
          const dx = x - it.r.x; const dy = y - it.r.y;
          NSUtils.__shiftGroup(canvas, it.id, dx, dy, boundW, boundH);
        } else {
          if (Number.isFinite(boundW)) x = Math.max(0, Math.min(boundW - it.r.w, x));
          if (Number.isFinite(boundH)) y = Math.max(0, Math.min(boundH - it.r.h, y));
          NSUtils.__writeRectPx(it.node, { x, y, w: it.r.w, h: it.r.h });
        }
      });
      try { if (NS.activeTabId) saveLayoutLocal(NS.activeTabId, grid); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function groupSelected() {
      if (!getGlobalEditMode()) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { gids, independentNodes, entities } = __getSelectedEntities(canvas);
      if (entities.length < 2) return;
      const targets = []; const seen = new Set();
      gids.forEach(gid => {
        const members = NSUtils.__getGroupMembers(canvas, gid);
        members.forEach(n => { if (n && !seen.has(n)) { seen.add(n); targets.push(n); } });
      });
      independentNodes.forEach(n => { if (n && !seen.has(n)) { seen.add(n); targets.push(n); } });
      if (targets.length < 2) return;
      const newGid = 'g_' + Math.random().toString(36).slice(2, 8) + Date.now().toString(36).slice(-4);
      let common = NSUtils.__getNodeGroupChain(targets[0]);
      targets.slice(1).forEach(n => {
        const c = NSUtils.__getNodeGroupChain(n);
        let i = 0;
        while (i < common.length && i < c.length && String(common[i]) === String(c[i])) i++;
        common = common.slice(0, i);
      });
      targets.forEach(n => {
        const chain = NSUtils.__getNodeGroupChain(n);
        const c2 = chain.slice(common.length);
        const next = common.concat([newGid], c2);
        NSUtils.__setNodeGroupChain(n, next);
      });
      try { clearSelection(); selectGroup(newGid); } catch {}
      try { updatePropPanel(currentSelected); } catch {}
      try { if (NS.activeTabId) saveLayoutLocal(NS.activeTabId, getActiveGrid()); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function ungroupSelected() {
      if (!getGlobalEditMode()) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { gids, independentNodes, entities } = __getSelectedEntities(canvas);
      if (entities.length < 1) return;
      gids.forEach(gid => {
        const g = String(gid || '').trim(); if (!g) return;
        const members = NSUtils.__getGroupMembers(canvas, g);
        members.forEach(n => {
          const chain = NSUtils.__getNodeGroupChain(n);
          const idx = chain.indexOf(g);
          if (idx >= 0) chain.splice(idx, 1);
          NSUtils.__setNodeGroupChain(n, chain);
        });
      });
      independentNodes.forEach(n => {
        const chain = NSUtils.__getNodeGroupChain(n);
        if (chain && chain.length) chain.pop();
        NSUtils.__setNodeGroupChain(n, chain);
      });
      try { clearSelection(); } catch {}
      updateAlignHint();
      updatePropPanel(null);
      try { if (NS.activeTabId) saveLayoutLocal(NS.activeTabId, getActiveGrid()); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function distributeSelected(axis) {
      if (!getGlobalEditMode()) return;
      const grid = getActiveGrid(); if (!grid) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { entities } = __getSelectedEntities(canvas);
      if (entities.length < 3) return;
      const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
      const boundW = info && info.design ? Number(info.design.width) : NaN;
      const boundH = info && info.design ? Number(info.design.height) : NaN;
      if (axis === 'h') {
        entities.sort((a, b) => (a.r.x - b.r.x) || (a.r.y - b.r.y));
        const minX = Math.min.apply(null, entities.map(it => it.r.x));
        const maxR = Math.max.apply(null, entities.map(it => it.r.x + it.r.w));
        const sumW = entities.reduce((s, it) => s + it.r.w, 0);
        const gap = (maxR - minX - sumW) / (entities.length - 1);
        let cursor = minX;
        entities.forEach(it => {
          if (it.type === 'group') { const dx = cursor - it.r.x; NSUtils.__shiftGroup(canvas, it.id, dx, 0, boundW, boundH); }
          else { NSUtils.__writeRectPx(it.node, { x: cursor, y: it.r.y, w: it.r.w, h: it.r.h }); }
          cursor = cursor + it.r.w + gap;
        });
      } else {
        entities.sort((a, b) => (a.r.y - b.r.y) || (a.r.x - b.r.x));
        const minY = Math.min.apply(null, entities.map(it => it.r.y));
        const maxB = Math.max.apply(null, entities.map(it => it.r.y + it.r.h));
        const sumH = entities.reduce((s, it) => s + it.r.h, 0);
        const gap = (maxB - minY - sumH) / (entities.length - 1);
        let cursor = minY;
        entities.forEach(it => {
          if (it.type === 'group') { const dy = cursor - it.r.y; NSUtils.__shiftGroup(canvas, it.id, 0, dy, boundW, boundH); }
          else { NSUtils.__writeRectPx(it.node, { x: it.r.x, y: cursor, w: it.r.w, h: it.r.h }); }
          cursor = cursor + it.r.h + gap;
        });
      }
      try { if (NS.activeTabId) saveLayoutLocal(NS.activeTabId, grid); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function sameSizeSelected() {
      if (!getGlobalEditMode()) return;
      const grid = getActiveGrid(); if (!grid) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { gids, independentNodes, entities } = __getSelectedEntities(canvas);
      if (entities.length < 2) return;
      let refEntity = null, refRect = null;
      let refGid = String((window && window.__selectedGroupAnchor) || '').trim();
      let refNode = anchorSelected;
      if (refNode && independentNodes.includes(refNode)) {
        refEntity = { type: 'node', node: refNode }; refRect = NSUtils.__readRectPx(refNode);
      } else if (refGid && gids.includes(refGid)) {
        refEntity = { type: 'group', id: refGid }; refRect = NSUtils.__getGroupRect(canvas, refGid);
      } else {
        refEntity = entities[0]; refRect = refEntity.r;
      }
      if (!refRect) return;
      const targetW = Math.max(10, refRect.w); const targetH = Math.max(10, refRect.h);
      const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
      const boundW = info && info.design ? Number(info.design.width) : NaN;
      const boundH = info && info.design ? Number(info.design.height) : NaN;
      entities.forEach(it => {
        if (it.type === 'group' && it.id === refEntity.id) return;
        if (it.type === 'node' && it.node === refEntity.node) return;
        if (it.type === 'group') { NSUtils.__resizeGroupTo(canvas, it.id, targetW, targetH, boundW, boundH); }
        else {
          let x = it.r.x, y = it.r.y;
          if (Number.isFinite(boundW)) x = Math.max(0, Math.min(boundW - targetW, x));
          if (Number.isFinite(boundH)) y = Math.max(0, Math.min(boundH - targetH, y));
          NSUtils.__writeRectPx(it.node, { x, y, w: targetW, h: targetH });
        }
      });
      try { if (NS.activeTabId) saveLayoutLocal(NS.activeTabId, grid); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof NSInteract.__scheduleGroupIndicatorsUpdate === 'function') NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    // 颜色输入工具（供属性面板使用）
    function __initColorInput(inp, raw, fallbackHex) {
      try {
        const fb0 = String(fallbackHex || '#000000').trim();
        const fb = /^#[0-9a-fA-F]{6}$/.test(fb0) ? fb0 : '#000000';
        const s = (raw !== undefined && raw !== null) ? String(raw).trim() : '';
        const rawStr = s || fb;
        inp.dataset.colorRaw = rawStr;
        inp.value = fb;
      } catch {}
    }

    function __upgradeColorInputs(rootEl) {
      try {
        const root = rootEl || document;
        const VueG = (window && window.Vue) ? window.Vue : null;
        const EPG = (window && window.ElementPlus) ? window.ElementPlus : null;
        if (!VueG || !VueG.createApp || !EPG) return;
        const inputs = root && root.querySelectorAll ? Array.from(root.querySelectorAll('input[type="color"]')) : [];
        inputs.forEach(inp => {
          if (!inp || (inp.dataset && inp.dataset.alphaUpgraded === '1')) return;
          const parent = inp.parentElement;
          if (!parent) return;
          const originalOnChange = inp.onchange;
          const disabled = !!inp.disabled;
          const raw = (inp.dataset && inp.dataset.colorRaw) ? String(inp.dataset.colorRaw || '').trim() : '';
          const baseCss = String(raw || inp.value || '').trim() || 'rgba(0,0,0,1)';
          inp.dataset.alphaUpgraded = '1';
          inp.type = 'hidden';
          try { inp.style.display = 'none'; } catch {}
          inp.value = baseCss;
          inp.dataset.colorRaw = baseCss;
          const ctl = document.createElement('div');
          ctl.className = 'd-flex align-items-center';
          ctl.style.gap = '8px';
          ctl.style.position = 'relative';
          const mountEl = document.createElement('div');
          mountEl.style.maxWidth = '260px';
          let mounted = false;
          const ensureMounted = () => {
            if (mounted) return;
            const initColor = String(inp.value || baseCss).trim() || baseCss;
            const app = VueG.createApp({
              data() { return { color: initColor, disabled: disabled }; },
              watch: { color(v) {
                const s = (v !== undefined && v !== null) ? String(v).trim() : '';
                if (!s) return;
                inp.value = s; inp.dataset.colorRaw = s;
                try { inp.dispatchEvent(new Event('input', { bubbles: true })); } catch {}
                try { inp.dispatchEvent(new Event('change', { bubbles: true })); } catch {}
              }},
              template: '<el-color-picker v-model="color" :disabled="disabled" :teleported="false" popper-class="ns-color-popper" show-alpha color-format="rgb" clearable />'
            });
            try { app.use(EPG); } catch {}
            app.mount(mountEl);
            mounted = true;
          };
          let replaced = false;
          try { parent.replaceChild(ctl, inp); replaced = true; } catch {}
          if (!replaced) return;
          if (disabled) { try { inp.disabled = true; } catch {} }
          ctl.appendChild(mountEl);
          ctl.appendChild(inp);
          ensureMounted();
        });
      } catch {}
    }

    function __closeEpColorPickers() {
      try { if (document && document.activeElement && document.activeElement.blur) document.activeElement.blur(); } catch {}
      try {
        const poppers = Array.from(document.querySelectorAll('.el-popper, .ns-color-popper'));
        poppers.forEach(p => {
          try {
            if (!p) return;
            const hasColor = !!(p.querySelector && (p.querySelector('.el-color-dropdown') || p.querySelector('.el-color-picker__panel') || p.querySelector('.el-color-svpanel')));
            if (!hasColor) return;
            try { p.style.display = 'none'; } catch {}
          } catch {}
        });
      } catch {}
    }

      function updatePropPanel(node) {
        try { __closeEpColorPickers(); } catch {}
        const panel = document.getElementById('propPanel');
        panel.innerHTML = '';
        if (!node) {
          const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.size === 'number') ? window.__selectedGroupIds : null;
          const gCount = gs ? Number(gs.size || 0) : 0;
          if (gCount > 0) {
            const gids = Array.from(gs);
            const wrap = Array.from(document.querySelectorAll('#tabsContent [data-tab-id]')).find(el => el.style.display !== 'none') || null;
            const canvas = wrap ? wrap.querySelector('.pixel-canvas') : null;

            if (gids.length === 1) {
              const gid = String(gids[0] || '').trim();
              let memberCount = 0;
              try {
                if (canvas) memberCount = NSUtils.__getGroupMembers(canvas, gid).length;
              } catch {}
              panel.innerHTML = '<div class="text-muted">已选择分组</div>'
                + '<div class="mt-1">Group ID：<span class="text-muted">' + gid + '</span></div>'
                + '<div class="mt-1">成员数量：<span class="text-muted">' + String(memberCount || 0) + '</span></div>';
              return;
            }

            panel.innerHTML = '<div class="text-muted">已选择分组</div>'
              + '<div class="mt-1">分组数量：<span class="text-muted">' + String(gids.length) + '</span></div>'
              + '<div class="mt-2" style="max-height: 140px; overflow:auto;">'
              + gids.map(g => '<div class="text-muted">• ' + String(g) + '</div>').join('')
              + '</div>';
            return;
          }
          const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
          const d = (info && info.design) ? info.design : { width: EPWidgets.layoutDefaults.designWidth, height: EPWidgets.layoutDefaults.designHeight, bgColor: EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc' };
          const canEdit = (function(){
            try { if (typeof getGlobalEditMode === 'function') return !!getGlobalEditMode(); } catch {}
            try { return !!(document.body && document.body.classList && document.body.classList.contains('edit-mode')); } catch {}
            return false;
          })();

          const form = document.createElement('div');

          const head = document.createElement('div');
          head.className = 'mb-2';
          head.innerHTML = '<div class="prop-label">画布属性</div><div class="prop-value text-muted">未选择控件</div>';
          form.appendChild(head);

          const box = document.createElement('div');
          box.className = 'mb-2 p-2 bg-light border rounded';
          box.innerHTML = '<div class="prop-label fw-bold mb-1">画布设置</div>';

          const presetRow = document.createElement('div');
          presetRow.className = 'mb-1';
          presetRow.innerHTML = '<div class="prop-label">常用尺寸</div>';
          const presetSelect = document.createElement('select');
          presetSelect.className = 'form-select form-select-sm';
          if (!canEdit) presetSelect.disabled = true;
          const presets = [
            { w: 1280, h: 720, label: '1280×720 (720p)' },
            { w: 1366, h: 768, label: '1366×768' },
            { w: 1920, h: 1080, label: '1920×1080 (1080p)' },
            { w: 2560, h: 1440, label: '2560×1440 (2K)' },
            { w: 3840, h: 2160, label: '3840×2160 (4K)' },
            { w: 1080, h: 1920, label: '1080×1920 (竖屏)' }
          ];
          const optCustom = document.createElement('option');
          optCustom.value = '';
          optCustom.textContent = '自定义';
          presetSelect.appendChild(optCustom);
          presets.forEach(p => {
            const opt = document.createElement('option');
            opt.value = String(p.w) + 'x' + String(p.h);
            opt.textContent = p.label;
            presetSelect.appendChild(opt);
          });
          presetRow.appendChild(presetSelect);
          box.appendChild(presetRow);

          const wRow = document.createElement('div');
          wRow.className = 'mb-1';
          wRow.innerHTML = '<div class="prop-label">画布宽度 (px)</div>';
          const wInput = document.createElement('input');
          wInput.type = 'number';
          wInput.className = 'form-control form-control-sm';
          wInput.min = '320';
          wInput.value = String(Math.floor(Number(d.width) || EPWidgets.layoutDefaults.designWidth || 1280));
          if (!canEdit) wInput.disabled = true;
          wRow.appendChild(wInput);
          box.appendChild(wRow);

          const hRow = document.createElement('div');
          hRow.className = 'mb-1';
          hRow.innerHTML = '<div class="prop-label">画布高度 (px)</div>';
          const hInput = document.createElement('input');
          hInput.type = 'number';
          hInput.className = 'form-control form-control-sm';
          hInput.min = '240';
          hInput.value = String(Math.floor(Number(d.height) || EPWidgets.layoutDefaults.designHeight || 720));
          if (!canEdit) hInput.disabled = true;
          hRow.appendChild(hInput);
          box.appendChild(hRow);

          try {
            const curKey = String(wInput.value || '') + 'x' + String(hInput.value || '');
            if (presetSelect && presetSelect.querySelector && presetSelect.querySelector('option[value="' + curKey.replace(/"/g, '\\"') + '"]')) {
              presetSelect.value = curKey;
            } else if (presetSelect) {
              presetSelect.value = '';
            }
          } catch {}

          const bgRow = document.createElement('div');
          bgRow.className = 'mb-1 d-flex align-items-center justify-content-between';
          bgRow.innerHTML = '<div class="prop-label">背景颜色</div>';
          const bgInput = document.createElement('input');
          bgInput.type = 'color';
          bgInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(bgInput, (d && d.bgColor !== undefined && d.bgColor !== null) ? d.bgColor : (EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc'), '#f8fafc');
          if (!canEdit) bgInput.disabled = true;
          bgRow.appendChild(bgInput);
          box.appendChild(bgRow);

          if (!canEdit) {
            const hint = document.createElement('div');
            hint.className = 'mt-2 text-muted';
            hint.style.fontSize = '12px';
            hint.textContent = '切换到编辑模式后可修改画布属性';
            box.appendChild(hint);
          }

          form.appendChild(box);

          if (canEdit && info) {
            // 函数级注释：读取表单值并应用画布属性（尺寸/背景），并保存到本地布局
            const applyCanvasProps = () => {
              const nextW = Math.max(320, Math.floor(Number(wInput.value) || 0));
              const nextH = Math.max(240, Math.floor(Number(hInput.value) || 0));
              const nextBg = String(bgInput.value || '').trim() || String(EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc');
              const key = String(nextW) + 'x' + String(nextH);
              try {
                if (presetSelect && presetSelect.querySelector && presetSelect.querySelector('option[value="' + key.replace(/"/g, '\\"') + '"]')) presetSelect.value = key;
                else if (presetSelect) presetSelect.value = '';
              } catch {}
              NSCanvas.applyPageDesign(NS.activeTabId, { width: nextW, height: nextH, bgColor: nextBg });
              try { saveLayoutLocal(NS.activeTabId, info.grid); } catch {}
            };

            presetSelect.onchange = () => {
              const v = String(presetSelect.value || '').trim();
              if (!v) return;
              const parts = v.split('x');
              if (parts.length === 2) {
                wInput.value = String(parts[0] || '').trim();
                hInput.value = String(parts[1] || '').trim();
              }
              applyCanvasProps();
            };

            wInput.onchange = () => applyCanvasProps();
            hInput.onchange = () => applyCanvasProps();
            bgInput.onchange = () => applyCanvasProps();

            const applyRow = document.createElement('div');
            applyRow.className = 'mt-2';
            const applyBtn = document.createElement('button');
            applyBtn.className = 'btn btn-sm btn-primary';
            applyBtn.textContent = '应用画布修改';
            applyRow.appendChild(applyBtn);
            form.appendChild(applyRow);

            applyBtn.onclick = () => applyCanvasProps();
          }

          panel.appendChild(form);
          try { __upgradeColorInputs(panel); } catch {}
          return;
        }
        const type = node.dataset.type || '未知';
        const props = EPWidgets.getProps(node);
        const form = document.createElement('div');

        if (selectedNodes.size > 1) {
          const multiRow = document.createElement('div');
          multiRow.className = 'mb-2 p-2 bg-light border rounded';
          multiRow.innerHTML = '<div class="prop-label">提示</div><div class="prop-value">当前已选择 ' + selectedNodes.size + ' 个控件，属性面板仅显示最后选中控件</div>';
          form.appendChild(multiRow);
        }

        // 类型显示
        const typeRow = document.createElement('div');
        typeRow.className = 'mb-2';
        typeRow.innerHTML = '<div class="prop-label">类型</div><div class="prop-value text-muted">'+ type +'</div>';
        form.appendChild(typeRow);
        
        // 基础ID设置（时间线控件的 Command ID 用作“游标”绑定，其余控件为通用 Command ID；Frame 不需要）
        if (type !== 'Frame') {
          const idRow = document.createElement('div');
          idRow.className = 'mb-2 p-2 bg-light border rounded';
          idRow.innerHTML = '<div class="prop-label fw-bold mb-1">ID 设置</div>';

          const cmdIdDiv = document.createElement('div');
          cmdIdDiv.className = 'mb-1';
          cmdIdDiv.innerHTML = '<div class="prop-label">' + (type === '时间线' ? '游标 Command ID' : 'Command ID') + '</div>';
          const cmdIdInput = document.createElement('input');
          if (type === '时间线') cmdIdInput.id = 'propTimelineCommandId';
          cmdIdInput.className = 'form-control form-control-sm';
          cmdIdInput.value = props.commandId || '/cmd/demo';
          cmdIdInput.onchange = (e) => EPWidgets.setProps(node, { commandId: e.target.value });
          cmdIdDiv.appendChild(cmdIdInput);
          idRow.appendChild(cmdIdDiv);

          form.appendChild(idRow);
        }

        // 样式设置
        const styleRow = document.createElement('div');
        styleRow.className = 'mb-2 p-2 bg-light border rounded';
        styleRow.innerHTML = '<div class="prop-label fw-bold mb-1">样式设置</div>';
        
        const bgColorDiv = document.createElement('div');
        bgColorDiv.className = 'mb-1 d-flex align-items-center justify-content-between';
        bgColorDiv.innerHTML = '<div class="prop-label">背景色</div>';
        const bgColorInput = document.createElement('input');
        bgColorInput.type = 'color';
        bgColorInput.className = 'form-control form-control-color form-control-sm';
        __initColorInput(bgColorInput, (props.bgColor !== undefined && props.bgColor !== null) ? props.bgColor : 'transparent', '#ffffff');
        bgColorInput.onchange = (e) => {
             EPWidgets.setProps(node, { bgColor: e.target.value });
             try { node.querySelector('.grid-stack-item-content').style.backgroundColor = e.target.value; } catch {}
        };
        bgColorDiv.appendChild(bgColorInput);
        styleRow.appendChild(bgColorDiv);

        if (type !== 'Frame') {
          const fontSizeDiv = document.createElement('div');
          fontSizeDiv.className = 'mb-1';
          fontSizeDiv.innerHTML = '<div class="prop-label">字体大小 (px)</div>';
          const fontSizeInput = document.createElement('input');
          fontSizeInput.type = 'number';
          fontSizeInput.className = 'form-control form-control-sm';
          fontSizeInput.value = props.fontSize || '14';
          fontSizeInput.onchange = (e) => {
               EPWidgets.setProps(node, { fontSize: e.target.value });
               node.style.fontSize = e.target.value + 'px';
          };
          fontSizeDiv.appendChild(fontSizeInput);
          styleRow.appendChild(fontSizeDiv);
        }
        
        form.appendChild(styleRow);

        // 根据类型渲染特定属性
        if (type === '按钮') {
          const labelRow = document.createElement('div');
          labelRow.className = 'mb-2';
          labelRow.innerHTML = '<div class="prop-label">文本</div>';
          const labelInput = document.createElement('input');
          labelInput.id = 'propBtnLabel';
          labelInput.className = 'form-control form-control-sm';
          labelInput.value = props.label || '执行';
          labelInput.onchange = (e) => EPWidgets.setProps(node, { label: e.target.value });
          labelRow.appendChild(labelInput);
          form.appendChild(labelRow);

          // 按钮颜色
          const btnColorRow = document.createElement('div');
          btnColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          btnColorRow.innerHTML = '<div class="prop-label">按钮颜色</div>';
          const btnColorInput = document.createElement('input');
          btnColorInput.id = 'propBtnColor';
          btnColorInput.type = 'color';
          btnColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(btnColorInput, props.buttonColor || '#409EFF', '#409EFF');
          btnColorInput.onchange = (e) => EPWidgets.setProps(node, { buttonColor: e.target.value });
          btnColorRow.appendChild(btnColorInput);
          styleRow.appendChild(btnColorRow);

          // 按下颜色
          const activeColorRow = document.createElement('div');
          activeColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          activeColorRow.innerHTML = '<div class="prop-label">按下颜色</div>';
          const activeColorInput = document.createElement('input');
          activeColorInput.id = 'propBtnActiveColor';
          activeColorInput.type = 'color';
          activeColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(activeColorInput, props.activeColor || '#3a8ee6', '#3a8ee6');
          activeColorInput.onchange = (e) => EPWidgets.setProps(node, { activeColor: e.target.value });
          activeColorRow.appendChild(activeColorInput);
          styleRow.appendChild(activeColorRow);

          // 文字颜色
          const txtColorRow = document.createElement('div');
          txtColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          txtColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const txtColorInput = document.createElement('input');
          txtColorInput.id = 'propBtnTextColor';
          txtColorInput.type = 'color';
          txtColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(txtColorInput, props.textColor || '#ffffff', '#ffffff');
          txtColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          txtColorRow.appendChild(txtColorInput);
          styleRow.appendChild(txtColorRow);

          // 边框颜色
          const borderColorRow = document.createElement('div');
          borderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          borderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const borderColorInput = document.createElement('input');
          borderColorInput.id = 'propBtnBorderColor';
          borderColorInput.type = 'color';
          borderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(borderColorInput, props.borderColor || '#409EFF', '#409EFF');
          borderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          borderColorRow.appendChild(borderColorInput);
          styleRow.appendChild(borderColorRow);

          // 边框样式
          const borderStyleRow = document.createElement('div');
          borderStyleRow.className = 'mb-2';
          borderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const borderStyleSelect = document.createElement('select');
          borderStyleSelect.id = 'propBtnBorderStyle';
          borderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
              const opt = document.createElement('option');
              opt.value = s;
              opt.textContent = s;
              if (s === (props.borderStyle || 'none')) opt.selected = true;
              borderStyleSelect.appendChild(opt);
          });
          borderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          borderStyleRow.appendChild(borderStyleSelect);
          styleRow.appendChild(borderStyleRow);

        } else if (type === '超链接') {
          const labelRow = document.createElement('div');
          labelRow.className = 'mb-2';
          labelRow.innerHTML = '<div class="prop-label">文本</div>';
          const labelInput = document.createElement('input');
          labelInput.id = 'propLinkLabel';
          labelInput.className = 'form-control form-control-sm';
          labelInput.value = props.label || '打开链接';
          labelInput.onchange = (e) => EPWidgets.setProps(node, { label: e.target.value });
          labelRow.appendChild(labelInput);
          form.appendChild(labelRow);

          const hrefRow = document.createElement('div');
          hrefRow.className = 'mb-2';
          hrefRow.innerHTML = '<div class="prop-label">链接</div>';
          const hrefInput = document.createElement('input');
          hrefInput.id = 'propLinkHref';
          hrefInput.className = 'form-control form-control-sm';
          hrefInput.value = props.href || 'https://example.com';
          hrefInput.onchange = (e) => EPWidgets.setProps(node, { href: e.target.value });
          hrefRow.appendChild(hrefInput);
          form.appendChild(hrefRow);

          const targetRow = document.createElement('div');
          targetRow.className = 'mb-2 form-check form-switch';
          const targetInput = document.createElement('input');
          targetInput.id = 'propLinkTargetBlank';
          targetInput.type = 'checkbox';
          targetInput.className = 'form-check-input';
          targetInput.checked = (props.targetBlank !== false);
          targetInput.onchange = (e) => EPWidgets.setProps(node, { targetBlank: !!e.target.checked });
          const targetLbl = document.createElement('label');
          targetLbl.className = 'form-check-label ms-2';
          targetLbl.textContent = '新窗口打开';
          targetRow.appendChild(targetInput);
          targetRow.appendChild(targetLbl);
          form.appendChild(targetRow);

          const btnColorRow = document.createElement('div');
          btnColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          btnColorRow.innerHTML = '<div class="prop-label">按钮颜色</div>';
          const btnColorInput = document.createElement('input');
          btnColorInput.id = 'propLinkBtnColor';
          btnColorInput.type = 'color';
          btnColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(btnColorInput, props.buttonColor || '#409EFF', '#409EFF');
          btnColorInput.onchange = (e) => EPWidgets.setProps(node, { buttonColor: e.target.value });
          btnColorRow.appendChild(btnColorInput);
          styleRow.appendChild(btnColorRow);

          const activeColorRow = document.createElement('div');
          activeColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          activeColorRow.innerHTML = '<div class="prop-label">按下颜色</div>';
          const activeColorInput = document.createElement('input');
          activeColorInput.id = 'propLinkActiveColor';
          activeColorInput.type = 'color';
          activeColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(activeColorInput, props.activeColor || '#3a8ee6', '#3a8ee6');
          activeColorInput.onchange = (e) => EPWidgets.setProps(node, { activeColor: e.target.value });
          activeColorRow.appendChild(activeColorInput);
          styleRow.appendChild(activeColorRow);

          const txtColorRow = document.createElement('div');
          txtColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          txtColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const txtColorInput = document.createElement('input');
          txtColorInput.id = 'propLinkTextColor';
          txtColorInput.type = 'color';
          txtColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(txtColorInput, props.textColor || '#ffffff', '#ffffff');
          txtColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          txtColorRow.appendChild(txtColorInput);
          styleRow.appendChild(txtColorRow);

          const borderColorRow = document.createElement('div');
          borderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          borderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const borderColorInput = document.createElement('input');
          borderColorInput.id = 'propLinkBorderColor';
          borderColorInput.type = 'color';
          borderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(borderColorInput, props.borderColor || '#409EFF', '#409EFF');
          borderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          borderColorRow.appendChild(borderColorInput);
          styleRow.appendChild(borderColorRow);

          const borderStyleRow = document.createElement('div');
          borderStyleRow.className = 'mb-2';
          borderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const borderStyleSelect = document.createElement('select');
          borderStyleSelect.id = 'propLinkBorderStyle';
          borderStyleSelect.className = 'form-select form-select-sm';
          ['solid','dashed','dotted','double','none'].forEach(s=>{
            const opt = document.createElement('option');
            opt.value = s; opt.textContent = s; if (s === (props.borderStyle || 'none')) opt.selected = true; borderStyleSelect.appendChild(opt);
          });
          borderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          borderStyleRow.appendChild(borderStyleSelect);
          styleRow.appendChild(borderStyleRow);

        } else if (type === '滑块') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propSliderMin';
          minInput.type = 'number';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propSliderMax';
          maxInput.type = 'number';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 100;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propSliderValue';
          valInput.type = 'number';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          // 样式设置
          const sBarColorRow = document.createElement('div');
          sBarColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sBarColorRow.innerHTML = '<div class="prop-label">轨道颜色</div>';
          const sBarColorInput = document.createElement('input');
          sBarColorInput.id = 'propSliderBarColor';
          sBarColorInput.type = 'color';
          sBarColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sBarColorInput, props.barColor || '#e5e7eb', '#e5e7eb');
          sBarColorInput.onchange = (e) => EPWidgets.setProps(node, { barColor: e.target.value });
          sBarColorRow.appendChild(sBarColorInput);
          styleRow.appendChild(sBarColorRow);

          const sFillColorRow = document.createElement('div');
          sFillColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sFillColorRow.innerHTML = '<div class="prop-label">进度颜色</div>';
          const sFillColorInput = document.createElement('input');
          sFillColorInput.id = 'propSliderFillColor';
          sFillColorInput.type = 'color';
          sFillColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sFillColorInput, props.fillColor || '#2b6cb0', '#2b6cb0');
          sFillColorInput.onchange = (e) => EPWidgets.setProps(node, { fillColor: e.target.value });
          sFillColorRow.appendChild(sFillColorInput);
          styleRow.appendChild(sFillColorRow);

          const sValColorRow = document.createElement('div');
          sValColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sValColorRow.innerHTML = '<div class="prop-label">数值颜色</div>';
          const sValColorInput = document.createElement('input');
          sValColorInput.id = 'propSliderValueColor';
          sValColorInput.type = 'color';
          sValColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sValColorInput, props.valueColor || '#111827', '#111827');
          sValColorInput.onchange = (e) => EPWidgets.setProps(node, { valueColor: e.target.value });
          sValColorRow.appendChild(sValColorInput);
          styleRow.appendChild(sValColorRow);

          const sBorderColorRow = document.createElement('div');
          sBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const sBorderColorInput = document.createElement('input');
          sBorderColorInput.id = 'propSliderBorderColor';
          sBorderColorInput.type = 'color';
          sBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          sBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          sBorderColorRow.appendChild(sBorderColorInput);
          styleRow.appendChild(sBorderColorRow);

          const sBorderStyleRow = document.createElement('div');
          sBorderStyleRow.className = 'mb-2';
          sBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const sBorderStyleSelect = document.createElement('select');
          sBorderStyleSelect.id = 'propSliderBorderStyle';
          sBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            sBorderStyleSelect.appendChild(opt);
          });
          sBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          sBorderStyleRow.appendChild(sBorderStyleSelect);
          styleRow.appendChild(sBorderStyleRow);
        } else if (type === '浮点滑块') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propFloatSliderMin';
          minInput.type = 'number';
          minInput.step = '0.01';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propFloatSliderMax';
          maxInput.type = 'number';
          maxInput.step = '0.01';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 1;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const stepRow = document.createElement('div');
          stepRow.className = 'mb-2';
          stepRow.innerHTML = '<div class="prop-label">步进</div>';
          const stepInput = document.createElement('input');
          stepInput.id = 'propFloatSliderStep';
          stepInput.type = 'number';
          stepInput.step = '0.001';
          stepInput.className = 'form-control form-control-sm';
          stepInput.value = props.step ?? 0.01;
          stepRow.appendChild(stepInput);
          form.appendChild(stepRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propFloatSliderValue';
          valInput.type = 'number';
          valInput.step = '0.01';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          // 样式设置（浮点）
          const fsBarColorRow = document.createElement('div');
          fsBarColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fsBarColorRow.innerHTML = '<div class="prop-label">轨道颜色</div>';
          const fsBarColorInput = document.createElement('input');
          fsBarColorInput.id = 'propFloatSliderBarColor';
          fsBarColorInput.type = 'color';
          fsBarColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fsBarColorInput, props.barColor || '#e5e7eb', '#e5e7eb');
          fsBarColorInput.onchange = (e) => EPWidgets.setProps(node, { barColor: e.target.value });
          fsBarColorRow.appendChild(fsBarColorInput);
          styleRow.appendChild(fsBarColorRow);

          const fsFillColorRow = document.createElement('div');
          fsFillColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fsFillColorRow.innerHTML = '<div class="prop-label">进度颜色</div>';
          const fsFillColorInput = document.createElement('input');
          fsFillColorInput.id = 'propFloatSliderFillColor';
          fsFillColorInput.type = 'color';
          fsFillColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fsFillColorInput, props.fillColor || '#2b6cb0', '#2b6cb0');
          fsFillColorInput.onchange = (e) => EPWidgets.setProps(node, { fillColor: e.target.value });
          fsFillColorRow.appendChild(fsFillColorInput);
          styleRow.appendChild(fsFillColorRow);

          const fsValColorRow = document.createElement('div');
          fsValColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fsValColorRow.innerHTML = '<div class="prop-label">数值颜色</div>';
          const fsValColorInput = document.createElement('input');
          fsValColorInput.id = 'propFloatSliderValueColor';
          fsValColorInput.type = 'color';
          fsValColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fsValColorInput, props.valueColor || '#111827', '#111827');
          fsValColorInput.onchange = (e) => EPWidgets.setProps(node, { valueColor: e.target.value });
          fsValColorRow.appendChild(fsValColorInput);
          styleRow.appendChild(fsValColorRow);

          const fsBorderColorRow = document.createElement('div');
          fsBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fsBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const fsBorderColorInput = document.createElement('input');
          fsBorderColorInput.id = 'propFloatSliderBorderColor';
          fsBorderColorInput.type = 'color';
          fsBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fsBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          fsBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          fsBorderColorRow.appendChild(fsBorderColorInput);
          styleRow.appendChild(fsBorderColorRow);

          const fsBorderStyleRow = document.createElement('div');
          fsBorderStyleRow.className = 'mb-2';
          fsBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const fsBorderStyleSelect = document.createElement('select');
          fsBorderStyleSelect.id = 'propFloatSliderBorderStyle';
          fsBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            fsBorderStyleSelect.appendChild(opt);
          });
          fsBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          fsBorderStyleRow.appendChild(fsBorderStyleSelect);
          styleRow.appendChild(fsBorderStyleRow);
        } else if (type === '勾选') {
          const labelRow = document.createElement('div');
          labelRow.className = 'mb-2';
          labelRow.innerHTML = '<div class="prop-label">文本</div>';
          const labelInput = document.createElement('input');
          labelInput.id = 'propCheckLabel';
          labelInput.className = 'form-control form-control-sm';
          labelInput.value = props.label || '启用';
          labelRow.appendChild(labelInput);
          form.appendChild(labelRow);
          const chkRow = document.createElement('div');
          chkRow.className = 'mb-2';
          chkRow.innerHTML = '<div class="prop-label">选中</div>';
          const chkInput = document.createElement('input');
          chkInput.id = 'propCheckChecked';
          chkInput.type = 'checkbox';
          chkInput.className = 'form-check-input';
          chkInput.checked = !!props.checked;
          chkRow.appendChild(chkInput);
          form.appendChild(chkRow);
          const cActiveColorRow = document.createElement('div');
          cActiveColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          cActiveColorRow.innerHTML = '<div class="prop-label">选中颜色</div>';
          const cActiveColorInput = document.createElement('input');
          cActiveColorInput.id = 'propCheckActiveColor';
          cActiveColorInput.type = 'color';
          cActiveColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(cActiveColorInput, props.activeColor || '#409EFF', '#409EFF');
          cActiveColorRow.appendChild(cActiveColorInput);
          styleRow.appendChild(cActiveColorRow);
          const cTextColorRow = document.createElement('div');
          cTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          cTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const cTextColorInput = document.createElement('input');
          cTextColorInput.id = 'propCheckTextColor';
          cTextColorInput.type = 'color';
          cTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(cTextColorInput, props.textColor || '#111827', '#111827');
          cTextColorRow.appendChild(cTextColorInput);
          styleRow.appendChild(cTextColorRow);
          const cBorderColorRow = document.createElement('div');
          cBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          cBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const cBorderColorInput = document.createElement('input');
          cBorderColorInput.id = 'propCheckBorderColor';
          cBorderColorInput.type = 'color';
          cBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(cBorderColorInput, props.borderColor || '#409EFF', '#409EFF');
          cBorderColorRow.appendChild(cBorderColorInput);
          styleRow.appendChild(cBorderColorRow);
          const cBorderStyleRow = document.createElement('div');
          cBorderStyleRow.className = 'mb-2';
          cBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const cBorderStyleSelect = document.createElement('select');
          cBorderStyleSelect.id = 'propCheckBorderStyle';
          cBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            cBorderStyleSelect.appendChild(opt);
          });
          cBorderStyleRow.appendChild(cBorderStyleSelect);
          styleRow.appendChild(cBorderStyleRow);
        } else if (type === '开关') {
          const chkRow = document.createElement('div');
          chkRow.className = 'mb-2';
          chkRow.innerHTML = '<div class="prop-label">选中</div>';
          const chkInput = document.createElement('input');
          chkInput.id = 'propSwitchChecked';
          chkInput.type = 'checkbox';
          chkInput.className = 'form-check-input';
          chkInput.checked = !!props.checked;
          chkRow.appendChild(chkInput);
          form.appendChild(chkRow);
          const sOnColorRow = document.createElement('div');
          sOnColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sOnColorRow.innerHTML = '<div class="prop-label">开启颜色</div>';
          const sOnColorInput = document.createElement('input');
          sOnColorInput.id = 'propSwitchOnColor';
          sOnColorInput.type = 'color';
          sOnColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sOnColorInput, props.onColor || '#13ce66', '#13ce66');
          sOnColorRow.appendChild(sOnColorInput);
          styleRow.appendChild(sOnColorRow);
          const sOffColorRow = document.createElement('div');
          sOffColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sOffColorRow.innerHTML = '<div class="prop-label">关闭颜色</div>';
          const sOffColorInput = document.createElement('input');
          sOffColorInput.id = 'propSwitchOffColor';
          sOffColorInput.type = 'color';
          sOffColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sOffColorInput, props.offColor || '#ff4949', '#ff4949');
          sOffColorRow.appendChild(sOffColorInput);
          styleRow.appendChild(sOffColorRow);
          const sBorderColorRow = document.createElement('div');
          sBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const sBorderColorInput = document.createElement('input');
          sBorderColorInput.id = 'propSwitchBorderColor';
          sBorderColorInput.type = 'color';
          sBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          sBorderColorRow.appendChild(sBorderColorInput);
          styleRow.appendChild(sBorderColorRow);
          const sBorderStyleRow = document.createElement('div');
          sBorderStyleRow.className = 'mb-2';
          sBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const sBorderStyleSelect = document.createElement('select');
          sBorderStyleSelect.id = 'propSwitchBorderStyle';
          sBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            sBorderStyleSelect.appendChild(opt);
          });
          sBorderStyleRow.appendChild(sBorderStyleSelect);
          styleRow.appendChild(sBorderStyleRow);
        } else if (type === '输入框') {
          const phRow = document.createElement('div');
          phRow.className = 'mb-2';
          phRow.innerHTML = '<div class="prop-label">占位符</div>';
          const phInput = document.createElement('input');
          phInput.id = 'propInputPlaceholder';
          phInput.className = 'form-control form-control-sm';
          phInput.value = props.placeholder || '请输入';
          phRow.appendChild(phInput);
          form.appendChild(phRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propInputValue';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value || '';
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          const iTextColorRow = document.createElement('div');
          iTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          iTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const iTextColorInput = document.createElement('input');
          iTextColorInput.id = 'propInputTextColor';
          iTextColorInput.type = 'color';
          iTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(iTextColorInput, props.textColor || '#111827', '#111827');
          iTextColorRow.appendChild(iTextColorInput);
          styleRow.appendChild(iTextColorRow);
          const iBorderColorRow = document.createElement('div');
          iBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          iBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const iBorderColorInput = document.createElement('input');
          iBorderColorInput.id = 'propInputBorderColor';
          iBorderColorInput.type = 'color';
          iBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(iBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          iBorderColorRow.appendChild(iBorderColorInput);
          styleRow.appendChild(iBorderColorRow);
          const iBorderStyleRow = document.createElement('div');
          iBorderStyleRow.className = 'mb-2';
          iBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const iBorderStyleSelect = document.createElement('select');
          iBorderStyleSelect.id = 'propInputBorderStyle';
          iBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            iBorderStyleSelect.appendChild(opt);
          });
          iBorderStyleRow.appendChild(iBorderStyleSelect);
          styleRow.appendChild(iBorderStyleRow);
        } else if (type === 'Frame') {
          const fLabelRow = document.createElement('div');
          fLabelRow.className = 'mb-2';
          fLabelRow.innerHTML = '<div class="prop-label">标题文本</div>';
          const fLabelInput = document.createElement('input');
          fLabelInput.id = 'propFrameLabel';
          fLabelInput.type = 'text';
          fLabelInput.className = 'form-control form-control-sm';
          fLabelInput.value = String(props.label ?? '');
          const applyLabel = (e) => { try { EPWidgets.setProps(node, { label: String(e.target.value ?? '') }); } catch {} };
          fLabelInput.onchange = applyLabel;
          fLabelInput.oninput = applyLabel;
          fLabelRow.appendChild(fLabelInput);
          styleRow.appendChild(fLabelRow);

          const fLabelColorRow = document.createElement('div');
          fLabelColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fLabelColorRow.innerHTML = '<div class="prop-label">标题颜色</div>';
          const fLabelColorInput = document.createElement('input');
          fLabelColorInput.id = 'propFrameLabelColor';
          fLabelColorInput.type = 'color';
          fLabelColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fLabelColorInput, props.labelColor || '#111827', '#111827');
          const applyLabelColor = (e) => { try { EPWidgets.setProps(node, { labelColor: e.target.value }); } catch {} };
          fLabelColorInput.onchange = applyLabelColor;
          fLabelColorRow.appendChild(fLabelColorInput);
          styleRow.appendChild(fLabelColorRow);

          const fLabelBgRow = document.createElement('div');
          fLabelBgRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fLabelBgRow.innerHTML = '<div class="prop-label">标题背景</div>';
          const fLabelBgInput = document.createElement('input');
          fLabelBgInput.id = 'propFrameLabelBgColor';
          fLabelBgInput.type = 'color';
          fLabelBgInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fLabelBgInput, props.labelBgColor || '#f8fafc', '#f8fafc');
          const applyLabelBg = (e) => { try { EPWidgets.setProps(node, { labelBgColor: e.target.value }); } catch {} };
          fLabelBgInput.onchange = applyLabelBg;
          fLabelBgRow.appendChild(fLabelBgInput);
          styleRow.appendChild(fLabelBgRow);

          const fLabelFsRow = document.createElement('div');
          fLabelFsRow.className = 'mb-2';
          fLabelFsRow.innerHTML = '<div class="prop-label">标题字号 (px)</div>';
          const fLabelFsInput = document.createElement('input');
          fLabelFsInput.id = 'propFrameLabelFontSize';
          fLabelFsInput.type = 'number';
          fLabelFsInput.min = '10';
          fLabelFsInput.step = '1';
          fLabelFsInput.className = 'form-control form-control-sm';
          fLabelFsInput.value = String(props.labelFontSize ?? 14);
          const applyLabelFs = (e) => { try { EPWidgets.setProps(node, { labelFontSize: Number(e.target.value) }); } catch {} };
          fLabelFsInput.onchange = applyLabelFs;
          fLabelFsInput.oninput = applyLabelFs;
          fLabelFsRow.appendChild(fLabelFsInput);
          styleRow.appendChild(fLabelFsRow);

          const fLabelFwRow = document.createElement('div');
          fLabelFwRow.className = 'mb-2';
          fLabelFwRow.innerHTML = '<div class="prop-label">标题字重</div>';
          const fLabelFwSelect = document.createElement('select');
          fLabelFwSelect.id = 'propFrameLabelFontWeight';
          fLabelFwSelect.className = 'form-select form-select-sm';
          ['300', '400', '500', '600', '700', '800'].forEach(w => {
            const opt = document.createElement('option');
            opt.value = w;
            opt.textContent = w;
            if (String(props.labelFontWeight ?? '500') === w) opt.selected = true;
            fLabelFwSelect.appendChild(opt);
          });
          const applyLabelFw = (e) => { try { EPWidgets.setProps(node, { labelFontWeight: e.target.value }); } catch {} };
          fLabelFwSelect.onchange = applyLabelFw;
          fLabelFwRow.appendChild(fLabelFwSelect);
          styleRow.appendChild(fLabelFwRow);

          const fLabelOffRow = document.createElement('div');
          fLabelOffRow.className = 'mb-2';
          fLabelOffRow.innerHTML = '<div class="prop-label">标题偏移 (x / y)</div>';
          const offWrap = document.createElement('div');
          offWrap.className = 'd-flex align-items-center';
          offWrap.style.gap = '6px';

          const fLabelOffX = document.createElement('input');
          fLabelOffX.id = 'propFrameLabelOffsetX';
          fLabelOffX.type = 'number';
          fLabelOffX.step = '1';
          fLabelOffX.className = 'form-control form-control-sm';
          fLabelOffX.value = String(props.labelOffsetX ?? 12);

          const fLabelOffY = document.createElement('input');
          fLabelOffY.id = 'propFrameLabelOffsetY';
          fLabelOffY.type = 'number';
          fLabelOffY.step = '1';
          fLabelOffY.className = 'form-control form-control-sm';
          fLabelOffY.value = String(props.labelOffsetY ?? 0);

          const applyLabelOff = () => {
            try {
              EPWidgets.setProps(node, {
                labelOffsetX: Number(fLabelOffX.value),
                labelOffsetY: Number(fLabelOffY.value)
              });
            } catch {}
          };
          fLabelOffX.onchange = applyLabelOff;
          fLabelOffX.oninput = applyLabelOff;
          fLabelOffY.onchange = applyLabelOff;
          fLabelOffY.oninput = applyLabelOff;

          offWrap.appendChild(fLabelOffX);
          offWrap.appendChild(fLabelOffY);
          fLabelOffRow.appendChild(offWrap);
          styleRow.appendChild(fLabelOffRow);

          const fBorderColorRow = document.createElement('div');
          fBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const fBorderColorInput = document.createElement('input');
          fBorderColorInput.id = 'propFrameBorderColor';
          fBorderColorInput.type = 'color';
          fBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          const applyBorderColor = (e) => { try { EPWidgets.setProps(node, { borderColor: e.target.value }); } catch {} };
          fBorderColorInput.onchange = applyBorderColor;
          fBorderColorRow.appendChild(fBorderColorInput);
          styleRow.appendChild(fBorderColorRow);

          const fBorderStyleRow = document.createElement('div');
          fBorderStyleRow.className = 'mb-2';
          fBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const fBorderStyleSelect = document.createElement('select');
          fBorderStyleSelect.id = 'propFrameBorderStyle';
          fBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'solid')) opt.selected = true;
            fBorderStyleSelect.appendChild(opt);
          });
          const applyBorderStyle = (e) => { try { EPWidgets.setProps(node, { borderStyle: e.target.value }); } catch {} };
          fBorderStyleSelect.onchange = applyBorderStyle;
          fBorderStyleRow.appendChild(fBorderStyleSelect);
          styleRow.appendChild(fBorderStyleRow);

          const fRadiusRow = document.createElement('div');
          fRadiusRow.className = 'mb-2';
          fRadiusRow.innerHTML = '<div class="prop-label">圆角 (px)</div>';
          const fRadiusInput = document.createElement('input');
          fRadiusInput.id = 'propFrameRadius';
          fRadiusInput.type = 'number';
          fRadiusInput.min = '0';
          fRadiusInput.step = '1';
          fRadiusInput.className = 'form-control form-control-sm';
          fRadiusInput.value = String(props.radius ?? 10);
          const applyRadius = (e) => { try { EPWidgets.setProps(node, { radius: Number(e.target.value) }); } catch {} };
          fRadiusInput.onchange = applyRadius;
          fRadiusInput.oninput = applyRadius;
          fRadiusRow.appendChild(fRadiusInput);
          styleRow.appendChild(fRadiusRow);

          const fShadowRow = document.createElement('div');
          fShadowRow.className = 'mb-2';
          fShadowRow.innerHTML = '<div class="prop-label">阴影</div>';
          const fShadowSelect = document.createElement('select');
          fShadowSelect.id = 'propFrameShadow';
          fShadowSelect.className = 'form-select form-select-sm';
          const shadowPresets = [
            { v: 'none', t: '无' },
            { v: '0 4px 12px rgba(0,0,0,0.12)', t: '柔和' },
            { v: '0 10px 30px rgba(0,0,0,0.18)', t: '中等' },
            { v: '0 20px 60px rgba(0,0,0,0.25)', t: '强烈' }
          ];
          shadowPresets.forEach(p => {
            const opt = document.createElement('option');
            opt.value = p.v;
            opt.textContent = p.t;
            if (String(props.shadow || 'none') === p.v) opt.selected = true;
            fShadowSelect.appendChild(opt);
          });
          const applyShadow = (e) => { try { EPWidgets.setProps(node, { shadow: e.target.value }); } catch {} };
          fShadowSelect.onchange = applyShadow;
          fShadowRow.appendChild(fShadowSelect);
          styleRow.appendChild(fShadowRow);

        } else if (type === '数值') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propNumberMin';
          minInput.type = 'number';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propNumberMax';
          maxInput.type = 'number';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 100;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const stepRow = document.createElement('div');
          stepRow.className = 'mb-2';
          stepRow.innerHTML = '<div class="prop-label">步进</div>';
          const stepInput = document.createElement('input');
          stepInput.id = 'propNumberStep';
          stepInput.type = 'number';
          stepInput.className = 'form-control form-control-sm';
          stepInput.step = '0.001';
          stepInput.value = props.step ?? 1;
          stepRow.appendChild(stepInput);
          form.appendChild(stepRow);
          const precRow = document.createElement('div');
          precRow.className = 'mb-2';
          precRow.innerHTML = '<div class="prop-label">精度 (小数位)</div>';
          const precInput = document.createElement('input');
          precInput.id = 'propNumberPrecision';
          precInput.type = 'number';
          precInput.className = 'form-control form-control-sm';
          precInput.min = '0';
          precInput.step = '1';
          precInput.value = props.precision ?? 0;
          precRow.appendChild(precInput);
          form.appendChild(precRow);
          const valRow2 = document.createElement('div');
          valRow2.className = 'mb-2';
          valRow2.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput2 = document.createElement('input');
          valInput2.id = 'propNumberValue';
          valInput2.type = 'number';
          valInput2.className = 'form-control form-control-sm';
          valInput2.value = props.value ?? 0;
          valRow2.appendChild(valInput2);
          form.appendChild(valRow2);
          const roRow = document.createElement('div');
          roRow.className = 'mb-2';
          roRow.innerHTML = '<div class="prop-label">只读</div>';
          const roInput = document.createElement('input');
          roInput.id = 'propNumberReadOnly';
          roInput.type = 'checkbox';
          roInput.className = 'form-check-input';
          roInput.checked = !!props.readOnly;
          roRow.appendChild(roInput);
          form.appendChild(roRow);
          // 样式设置
          const nTextColorRow = document.createElement('div');
          nTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          nTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const nTextColorInput = document.createElement('input');
          nTextColorInput.id = 'propNumberTextColor';
          nTextColorInput.type = 'color';
          nTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(nTextColorInput, props.textColor || '#111827', '#111827');
          nTextColorRow.appendChild(nTextColorInput);
          styleRow.appendChild(nTextColorRow);
          const nBorderColorRow = document.createElement('div');
          nBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          nBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const nBorderColorInput = document.createElement('input');
          nBorderColorInput.id = 'propNumberBorderColor';
          nBorderColorInput.type = 'color';
          nBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(nBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          nBorderColorRow.appendChild(nBorderColorInput);
          styleRow.appendChild(nBorderColorRow);
          const nBorderStyleRow = document.createElement('div');
          nBorderStyleRow.className = 'mb-2';
          nBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const nBorderStyleSelect = document.createElement('select');
          nBorderStyleSelect.id = 'propNumberBorderStyle';
          nBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            nBorderStyleSelect.appendChild(opt);
          });
          nBorderStyleRow.appendChild(nBorderStyleSelect);
          styleRow.appendChild(nBorderStyleRow);
          // 事件绑定
          minInput.onchange = (e) => EPWidgets.setProps(node, { min: Number(e.target.value) });
          maxInput.onchange = (e) => EPWidgets.setProps(node, { max: Number(e.target.value) });
          stepInput.onchange = (e) => EPWidgets.setProps(node, { step: Number(e.target.value) });
          precInput.onchange = (e) => EPWidgets.setProps(node, { precision: Math.max(0, Math.floor(Number(e.target.value)||0)) });
          valInput2.onchange = (e) => EPWidgets.setProps(node, { value: Number(e.target.value) });
          roInput.onchange = (e) => EPWidgets.setProps(node, { readOnly: e.target.checked });
          nTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          nBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          nBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
        } else if (type === '分割线') {
          const textRow = document.createElement('div');
          textRow.className = 'mb-2';
          textRow.innerHTML = '<div class="prop-label">文本</div>';
          const textInput = document.createElement('input');
          textInput.id = 'propDividerText';
          textInput.className = 'form-control form-control-sm';
          textInput.value = props.text || '分割线';
          textRow.appendChild(textInput);
          form.appendChild(textRow);
          const dTextColorRow = document.createElement('div');
          dTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          dTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const dTextColorInput = document.createElement('input');
          dTextColorInput.id = 'propDividerTextColor';
          dTextColorInput.type = 'color';
          dTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(dTextColorInput, props.textColor || '#64748b', '#64748b');
          dTextColorRow.appendChild(dTextColorInput);
          styleRow.appendChild(dTextColorRow);
          const dLineColorRow = document.createElement('div');
          dLineColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          dLineColorRow.innerHTML = '<div class="prop-label">线条颜色</div>';
          const dLineColorInput = document.createElement('input');
          dLineColorInput.id = 'propDividerLineColor';
          dLineColorInput.type = 'color';
          dLineColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(dLineColorInput, props.lineColor || '#e5e7eb', '#e5e7eb');
          dLineColorRow.appendChild(dLineColorInput);
          styleRow.appendChild(dLineColorRow);
          const dLineWidthRow = document.createElement('div');
          dLineWidthRow.className = 'mb-2';
          dLineWidthRow.innerHTML = '<div class="prop-label">线宽 (px)</div>';
          const dLineWidthInput = document.createElement('input');
          dLineWidthInput.id = 'propDividerLineWidth';
          dLineWidthInput.type = 'number';
          dLineWidthInput.className = 'form-control form-control-sm';
          dLineWidthInput.value = props.lineWidth || 2;
          dLineWidthRow.appendChild(dLineWidthInput);
          styleRow.appendChild(dLineWidthRow);
          const dBorderStyleRow = document.createElement('div');
          dBorderStyleRow.className = 'mb-2';
          dBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const dBorderStyleSelect = document.createElement('select');
          dBorderStyleSelect.id = 'propDividerBorderStyle';
          dBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'solid')) opt.selected = true;
            dBorderStyleSelect.appendChild(opt);
          });
          dBorderStyleRow.appendChild(dBorderStyleSelect);
          styleRow.appendChild(dBorderStyleRow);
        } else if (type === '竖向分割线') {
          const vLineColorRow = document.createElement('div');
          vLineColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vLineColorRow.innerHTML = '<div class="prop-label">线条颜色</div>';
          const vLineColorInput = document.createElement('input');
          vLineColorInput.id = 'propVDividerLineColor';
          vLineColorInput.type = 'color';
          vLineColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vLineColorInput, props.lineColor || '#e5e7eb', '#e5e7eb');
          vLineColorRow.appendChild(vLineColorInput);
          styleRow.appendChild(vLineColorRow);
          const vLineWidthRow = document.createElement('div');
          vLineWidthRow.className = 'mb-2';
          vLineWidthRow.innerHTML = '<div class="prop-label">线宽 (px)</div>';
          const vLineWidthInput = document.createElement('input');
          vLineWidthInput.id = 'propVDividerLineWidth';
          vLineWidthInput.type = 'number';
          vLineWidthInput.className = 'form-control form-control-sm';
          vLineWidthInput.value = props.lineWidth || 2;
          vLineWidthRow.appendChild(vLineWidthInput);
          styleRow.appendChild(vLineWidthRow);
          const vBorderStyleRow = document.createElement('div');
          vBorderStyleRow.className = 'mb-2';
          vBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const vBorderStyleSelect = document.createElement('select');
          vBorderStyleSelect.id = 'propVDividerBorderStyle';
          vBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            vBorderStyleSelect.appendChild(opt);
          });
          vBorderStyleRow.appendChild(vBorderStyleSelect);
          styleRow.appendChild(vBorderStyleRow);
        } else if (type === '标签') {
          const textRow = document.createElement('div');
          textRow.className = 'mb-2';
          textRow.innerHTML = '<div class="prop-label">文本</div>';
          const textInput = document.createElement('input');
          textInput.id = 'propLabelText';
          textInput.className = 'form-control form-control-sm';
          textInput.value = props.text || '标签';
          textRow.appendChild(textInput);
          form.appendChild(textRow);
          
          // 样式设置：文字颜色
          const lblTextColorRow = document.createElement('div');
          lblTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          lblTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const lblTextColorInput = document.createElement('input');
          lblTextColorInput.id = 'propLabelTextColor';
          lblTextColorInput.type = 'color';
          lblTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(lblTextColorInput, props.textColor || '#111827', '#111827');
          lblTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          lblTextColorRow.appendChild(lblTextColorInput);
          styleRow.appendChild(lblTextColorRow);

          // 样式设置：边框颜色
          const lblBorderColorRow = document.createElement('div');
          lblBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          lblBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const lblBorderColorInput = document.createElement('input');
          lblBorderColorInput.id = 'propLabelBorderColor';
          lblBorderColorInput.type = 'color';
          lblBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(lblBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          lblBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          lblBorderColorRow.appendChild(lblBorderColorInput);
          styleRow.appendChild(lblBorderColorRow);

          // 样式设置：边框样式
          const lblBorderStyleRow = document.createElement('div');
          lblBorderStyleRow.className = 'mb-2';
          lblBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const lblBorderStyleSelect = document.createElement('select');
          lblBorderStyleSelect.id = 'propLabelBorderStyle';
          lblBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            lblBorderStyleSelect.appendChild(opt);
          });
          lblBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          lblBorderStyleRow.appendChild(lblBorderStyleSelect);
          styleRow.appendChild(lblBorderStyleRow);
        } else if (type === '时间线') {
          const fpsRow = document.createElement('div');
          fpsRow.className = 'mb-2';
          fpsRow.innerHTML = '<div class="prop-label">帧率 (fps)</div>';
          const fpsInput = document.createElement('input');
          fpsInput.id = 'propTimelineFps';
          fpsInput.type = 'number';
          fpsInput.className = 'form-control form-control-sm';
          fpsInput.value = props.fps ?? 25;
          fpsInput.onchange = (e) => EPWidgets.setProps(node, { fps: Number(e.target.value) });
          fpsRow.appendChild(fpsInput);
          form.appendChild(fpsRow);

          const tickRow = document.createElement('div');
          tickRow.className = 'mb-2';
          tickRow.innerHTML = '<div class="prop-label">刻度步进 (秒)</div>';
          const tickInput = document.createElement('input');
          tickInput.id = 'propTimelineTickStepSec';
          tickInput.type = 'number';
          tickInput.className = 'form-control form-control-sm';
          tickInput.value = props.tickStepSec ?? 15;
          tickInput.onchange = (e) => EPWidgets.setProps(node, { tickStepSec: Number(e.target.value) });
          tickRow.appendChild(tickInput);
          form.appendChild(tickRow);


          const itemsBox = document.createElement('div');
          itemsBox.className = 'mb-2 p-2 bg-light border rounded';
          itemsBox.innerHTML = '<div class="prop-label fw-bold mb-1">节点列表（CommandID → 备注）</div>';
          const items = Array.isArray(props.items) ? props.items : [];

          const updateItems = (next) => {
            EPWidgets.setProps(node, { items: next });
            updatePropPanel(node);
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
            idInput.onchange = (e) => {
              const next = items.map((x, i) => (i === idx ? Object.assign({}, x || {}, { id: e.target.value }) : x));
              updateItems(next);
            };

            const textInput = document.createElement('input');
            textInput.type = 'text';
            textInput.className = 'form-control form-control-sm';
            textInput.value = (it && it.text !== undefined) ? String(it.text) : '';
            textInput.placeholder = '备注';
            textInput.onchange = (e) => {
              const next = items.map((x, i) => (i === idx ? Object.assign({}, x || {}, { text: e.target.value }) : x));
              updateItems(next);
            };


            const rmBtn = document.createElement('button');
            rmBtn.type = 'button';
            rmBtn.className = 'btn btn-sm btn-outline-danger';
            rmBtn.textContent = '删除';
            rmBtn.onclick = () => {
              const next = items.filter((_, i) => i !== idx);
              updateItems(next);
            };

            row.appendChild(idInput);
            row.appendChild(textInput);
            row.appendChild(rmBtn);
            itemsBox.appendChild(row);
          });

          const addBtn = document.createElement('button');
          addBtn.type = 'button';
          addBtn.className = 'btn btn-sm btn-outline-primary mt-2';
          addBtn.textContent = '添加节点';
          addBtn.onclick = () => {
            const next = items.slice();
            next.push({ id: String(next.length), text: '节点' + (next.length + 1) });
            updateItems(next);
          };
          itemsBox.appendChild(addBtn);
          form.appendChild(itemsBox);

          const lineColorRow = document.createElement('div');
          lineColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          lineColorRow.innerHTML = '<div class="prop-label">连线颜色</div>';
          const lineColorInput = document.createElement('input');
          lineColorInput.id = 'propTimelineLineColor';
          lineColorInput.type = 'color';
          lineColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(lineColorInput, props.lineColor || '#e5e7eb', '#e5e7eb');
          lineColorInput.onchange = (e) => EPWidgets.setProps(node, { lineColor: e.target.value });
          lineColorRow.appendChild(lineColorInput);
          styleRow.appendChild(lineColorRow);

          const pointColorRow = document.createElement('div');
          pointColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          pointColorRow.innerHTML = '<div class="prop-label">点颜色</div>';
          const pointColorInput = document.createElement('input');
          pointColorInput.id = 'propTimelinePointColor';
          pointColorInput.type = 'color';
          pointColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(pointColorInput, props.pointColor || props.activeColor || '#2563eb', '#2563eb');
          pointColorInput.onchange = (e) => EPWidgets.setProps(node, { pointColor: e.target.value });
          pointColorRow.appendChild(pointColorInput);
          styleRow.appendChild(pointColorRow);

          const textColorRow = document.createElement('div');
          textColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          textColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const textColorInput = document.createElement('input');
          textColorInput.id = 'propTimelineTextColor';
          textColorInput.type = 'color';
          textColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(textColorInput, props.textColor || '#111827', '#111827');
          textColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          textColorRow.appendChild(textColorInput);
          styleRow.appendChild(textColorRow);

          const labelBgRow = document.createElement('div');
          labelBgRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          labelBgRow.innerHTML = '<div class="prop-label">节点背景色</div>';
          const labelBgInput = document.createElement('input');
          labelBgInput.id = 'propTimelineLabelBgColor';
          labelBgInput.type = 'color';
          labelBgInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(labelBgInput, props.labelBgColor || '#ffffff', '#ffffff');
          labelBgInput.onchange = (e) => EPWidgets.setProps(node, { labelBgColor: e.target.value });
          labelBgRow.appendChild(labelBgInput);
          styleRow.appendChild(labelBgRow);

          const borderColorRow = document.createElement('div');
          borderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          borderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const borderColorInput = document.createElement('input');
          borderColorInput.id = 'propTimelineBorderColor';
          borderColorInput.type = 'color';
          borderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(borderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          borderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          borderColorRow.appendChild(borderColorInput);
          styleRow.appendChild(borderColorRow);

          const borderStyleRow = document.createElement('div');
          borderStyleRow.className = 'mb-2';
          borderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const borderStyleSelect = document.createElement('select');
          borderStyleSelect.id = 'propTimelineBorderStyle';
          borderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            borderStyleSelect.appendChild(opt);
          });
          borderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          borderStyleRow.appendChild(borderStyleSelect);
          styleRow.appendChild(borderStyleRow);
        } else if (type === '时间码') {
          const fpsRow = document.createElement('div');
          fpsRow.className = 'mb-2';
          fpsRow.innerHTML = '<div class="prop-label">帧率 (fps)</div>';
          const fpsInput = document.createElement('input');
          fpsInput.id = 'propTimecodeFps';
          fpsInput.type = 'number';
          fpsInput.className = 'form-control form-control-sm';
          fpsInput.value = props.fps ?? 25;
          fpsInput.onchange = (e) => EPWidgets.setProps(node, { fps: Number(e.target.value) });
          fpsRow.appendChild(fpsInput);
          form.appendChild(fpsRow);

          const readOnlyRow = document.createElement('div');
          readOnlyRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          readOnlyRow.innerHTML = '<div class="prop-label">只读（隐藏按钮）</div>';
          const readOnlyInput = document.createElement('input');
          readOnlyInput.id = 'propTimecodeReadOnly';
          readOnlyInput.type = 'checkbox';
          readOnlyInput.className = 'form-check-input';
          readOnlyInput.checked = !!props.readOnly;
          readOnlyInput.onchange = (e) => EPWidgets.setProps(node, { readOnly: !!e.target.checked });
          readOnlyRow.appendChild(readOnlyInput);
          form.appendChild(readOnlyRow);

          const framesRow = document.createElement('div');
          framesRow.className = 'mb-2';
          framesRow.innerHTML = '<div class="prop-label">帧数</div>';
          const framesInput = document.createElement('input');
          framesInput.id = 'propTimecodeFrames';
          framesInput.type = 'number';
          framesInput.className = 'form-control form-control-sm';
          framesInput.value = props.value ?? 0;
          framesInput.onchange = (e) => EPWidgets.setProps(node, { value: Number(e.target.value) });
          framesRow.appendChild(framesInput);
          form.appendChild(framesRow);

          const tcTextColorRow = document.createElement('div');
          tcTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tcTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const tcTextColorInput = document.createElement('input');
          tcTextColorInput.id = 'propTimecodeTextColor';
          tcTextColorInput.type = 'color';
          tcTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tcTextColorInput, props.textColor || '#111827', '#111827');
          tcTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          tcTextColorRow.appendChild(tcTextColorInput);
          styleRow.appendChild(tcTextColorRow);

          const tcBorderColorRow = document.createElement('div');
          tcBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tcBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const tcBorderColorInput = document.createElement('input');
          tcBorderColorInput.id = 'propTimecodeBorderColor';
          tcBorderColorInput.type = 'color';
          tcBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tcBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          tcBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          tcBorderColorRow.appendChild(tcBorderColorInput);
          styleRow.appendChild(tcBorderColorRow);

          const tcBorderStyleRow = document.createElement('div');
          tcBorderStyleRow.className = 'mb-2';
          tcBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const tcBorderStyleSelect = document.createElement('select');
          tcBorderStyleSelect.id = 'propTimecodeBorderStyle';
          tcBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            tcBorderStyleSelect.appendChild(opt);
          });
          tcBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          tcBorderStyleRow.appendChild(tcBorderStyleSelect);
          styleRow.appendChild(tcBorderStyleRow);
        } else if (type === '竖向滑动条') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propVSliderMin';
          minInput.type = 'number';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propVSliderMax';
          maxInput.type = 'number';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 100;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propVSliderValue';
          valInput.type = 'number';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          const heightRow = document.createElement('div');
          heightRow.className = 'mb-2';
          heightRow.innerHTML = '<div class="prop-label">高度</div>';
          const heightInput = document.createElement('input');
          heightInput.id = 'propVSliderHeight';
          heightInput.type = 'text';
          heightInput.placeholder = '例如 140px';
          heightInput.className = 'form-control form-control-sm';
          heightInput.value = props.height || '100%';
          heightRow.appendChild(heightInput);
          form.appendChild(heightRow);
          const vsBarColorRow = document.createElement('div');
          vsBarColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vsBarColorRow.innerHTML = '<div class="prop-label">轨道颜色</div>';
          const vsBarColorInput = document.createElement('input');
          vsBarColorInput.id = 'propVSliderBarColor';
          vsBarColorInput.type = 'color';
          vsBarColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vsBarColorInput, props.barColor || '#e5e7eb', '#e5e7eb');
          vsBarColorRow.appendChild(vsBarColorInput);
          styleRow.appendChild(vsBarColorRow);
          const vsFillColorRow = document.createElement('div');
          vsFillColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vsFillColorRow.innerHTML = '<div class="prop-label">进度颜色</div>';
          const vsFillColorInput = document.createElement('input');
          vsFillColorInput.id = 'propVSliderFillColor';
          vsFillColorInput.type = 'color';
          vsFillColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vsFillColorInput, props.fillColor || '#2b6cb0', '#2b6cb0');
          vsFillColorRow.appendChild(vsFillColorInput);
          styleRow.appendChild(vsFillColorRow);
          const vsValColorRow = document.createElement('div');
          vsValColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vsValColorRow.innerHTML = '<div class="prop-label">数值颜色</div>';
          const vsValColorInput = document.createElement('input');
          vsValColorInput.id = 'propVSliderValueColor';
          vsValColorInput.type = 'color';
          vsValColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vsValColorInput, props.valueColor || '#111827', '#111827');
          vsValColorRow.appendChild(vsValColorInput);
          styleRow.appendChild(vsValColorRow);
          const vsBorderColorRow = document.createElement('div');
          vsBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vsBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const vsBorderColorInput = document.createElement('input');
          vsBorderColorInput.id = 'propVSliderBorderColor';
          vsBorderColorInput.type = 'color';
          vsBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vsBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          vsBorderColorRow.appendChild(vsBorderColorInput);
          styleRow.appendChild(vsBorderColorRow);
          const vsBorderStyleRow = document.createElement('div');
          vsBorderStyleRow.className = 'mb-2';
          vsBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const vsBorderStyleSelect = document.createElement('select');
          vsBorderStyleSelect.id = 'propVSliderBorderStyle';
          vsBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            vsBorderStyleSelect.appendChild(opt);
          });
          vsBorderStyleRow.appendChild(vsBorderStyleSelect);
          styleRow.appendChild(vsBorderStyleRow);
        } else if (type === '竖向浮点滑块') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propVFloatSliderMin';
          minInput.type = 'number';
          minInput.step = '0.01';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propVFloatSliderMax';
          maxInput.type = 'number';
          maxInput.step = '0.01';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 1;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const stepRow = document.createElement('div');
          stepRow.className = 'mb-2';
          stepRow.innerHTML = '<div class="prop-label">步进</div>';
          const stepInput = document.createElement('input');
          stepInput.id = 'propVFloatSliderStep';
          stepInput.type = 'number';
          stepInput.step = '0.001';
          stepInput.className = 'form-control form-control-sm';
          stepInput.value = props.step ?? 0.01;
          stepRow.appendChild(stepInput);
          form.appendChild(stepRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propVFloatSliderValue';
          valInput.type = 'number';
          valInput.step = '0.01';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          const heightRow = document.createElement('div');
          heightRow.className = 'mb-2';
          heightRow.innerHTML = '<div class="prop-label">高度</div>';
          const heightInput = document.createElement('input');
          heightInput.id = 'propVFloatSliderHeight';
          heightInput.type = 'text';
          heightInput.placeholder = '例如 140px';
          heightInput.className = 'form-control form-control-sm';
          heightInput.value = props.height || '100%';
          heightRow.appendChild(heightInput);
          form.appendChild(heightRow);
          const vfsBarColorRow = document.createElement('div');
          vfsBarColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vfsBarColorRow.innerHTML = '<div class="prop-label">轨道颜色</div>';
          const vfsBarColorInput = document.createElement('input');
          vfsBarColorInput.id = 'propVFloatSliderBarColor';
          vfsBarColorInput.type = 'color';
          vfsBarColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vfsBarColorInput, props.barColor || '#e5e7eb', '#e5e7eb');
          vfsBarColorRow.appendChild(vfsBarColorInput);
          styleRow.appendChild(vfsBarColorRow);
          const vfsFillColorRow = document.createElement('div');
          vfsFillColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vfsFillColorRow.innerHTML = '<div class="prop-label">进度颜色</div>';
          const vfsFillColorInput = document.createElement('input');
          vfsFillColorInput.id = 'propVFloatSliderFillColor';
          vfsFillColorInput.type = 'color';
          vfsFillColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vfsFillColorInput, props.fillColor || '#2b6cb0', '#2b6cb0');
          vfsFillColorRow.appendChild(vfsFillColorInput);
          styleRow.appendChild(vfsFillColorRow);
          const vfsValColorRow = document.createElement('div');
          vfsValColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vfsValColorRow.innerHTML = '<div class="prop-label">数值颜色</div>';
          const vfsValColorInput = document.createElement('input');
          vfsValColorInput.id = 'propVFloatSliderValueColor';
          vfsValColorInput.type = 'color';
          vfsValColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vfsValColorInput, props.valueColor || '#111827', '#111827');
          vfsValColorRow.appendChild(vfsValColorInput);
          styleRow.appendChild(vfsValColorRow);
          const vfsBorderColorRow = document.createElement('div');
          vfsBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vfsBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const vfsBorderColorInput = document.createElement('input');
          vfsBorderColorInput.id = 'propVFloatSliderBorderColor';
          vfsBorderColorInput.type = 'color';
          vfsBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vfsBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          vfsBorderColorRow.appendChild(vfsBorderColorInput);
          styleRow.appendChild(vfsBorderColorRow);
          const vfsBorderStyleRow = document.createElement('div');
          vfsBorderStyleRow.className = 'mb-2';
          vfsBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const vfsBorderStyleSelect = document.createElement('select');
          vfsBorderStyleSelect.id = 'propVFloatSliderBorderStyle';
          vfsBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            vfsBorderStyleSelect.appendChild(opt);
          });
          vfsBorderStyleRow.appendChild(vfsBorderStyleSelect);
          styleRow.appendChild(vfsBorderStyleRow);
        } else if (type === '旋钮') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propKnobMin';
          minInput.type = 'number';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propKnobMax';
          maxInput.type = 'number';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 100;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propKnobValue';
          valInput.type = 'number';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          
          // 新增：颜色和步长设置
          const stepRow = document.createElement('div');
          stepRow.className = 'mb-2';
          stepRow.innerHTML = '<div class="prop-label">步长</div>';
          const stepInput = document.createElement('input');
          stepInput.id = 'propKnobStep';
          stepInput.type = 'number';
          stepInput.className = 'form-control form-control-sm';
          stepInput.value = props.step ?? 1;
          stepRow.appendChild(stepInput);
          form.appendChild(stepRow);
          
          const colorRow = document.createElement('div');
          colorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          colorRow.innerHTML = '<div class="prop-label">值颜色</div>';
          const colorInput = document.createElement('input');
          colorInput.id = 'propKnobValueColor';
          colorInput.type = 'color';
          colorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(colorInput, props.valueColor || '#3B82F6', '#3B82F6');
          colorRow.appendChild(colorInput);
          form.appendChild(colorRow);
          
          const kTextColorRow = document.createElement('div');
          kTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          kTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const kTextColorInput = document.createElement('input');
          kTextColorInput.id = 'propKnobTextColor';
          kTextColorInput.type = 'color';
          kTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(kTextColorInput, props.textColor || '#495057', '#495057');
          // 函数级注释：实时应用旋钮文字颜色
          kTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          kTextColorRow.appendChild(kTextColorInput);
          styleRow.appendChild(kTextColorRow);

          const kBorderColorRow = document.createElement('div');
          kBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          kBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const kBorderColorInput = document.createElement('input');
          kBorderColorInput.id = 'propKnobBorderColor';
          kBorderColorInput.type = 'color';
          kBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(kBorderColorInput, props.borderColor || '#dfe7ef', '#dfe7ef');
          // 函数级注释：实时应用旋钮边框颜色
          kBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          kBorderColorRow.appendChild(kBorderColorInput);
          styleRow.appendChild(kBorderColorRow);

          const kBorderStyleRow = document.createElement('div');
          kBorderStyleRow.className = 'mb-2';
          kBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const kBorderStyleSelect = document.createElement('select');
          kBorderStyleSelect.id = 'propKnobBorderStyle';
          kBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            kBorderStyleSelect.appendChild(opt);
          });
          // 函数级注释：实时应用旋钮边框样式
          kBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          kBorderStyleRow.appendChild(kBorderStyleSelect);
          styleRow.appendChild(kBorderStyleRow);

          const kRangeColorRow = document.createElement('div');
          kRangeColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          kRangeColorRow.innerHTML = '<div class="prop-label">范围颜色</div>';
          const kRangeColorInput = document.createElement('input');
          kRangeColorInput.id = 'propKnobRangeColor';
          kRangeColorInput.type = 'color';
          kRangeColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(kRangeColorInput, props.rangeColor || '#dfe7ef', '#dfe7ef');
          // 函数级注释：实时应用旋钮范围颜色
          kRangeColorInput.onchange = (e) => EPWidgets.setProps(node, { rangeColor: e.target.value });
          kRangeColorRow.appendChild(kRangeColorInput);
          styleRow.appendChild(kRangeColorRow);

          const kStrokeWidthRow = document.createElement('div');
          kStrokeWidthRow.className = 'mb-2';
          kStrokeWidthRow.innerHTML = '<div class="prop-label">描边宽度</div>';
          const kStrokeWidthInput = document.createElement('input');
          kStrokeWidthInput.id = 'propKnobStrokeWidth';
          kStrokeWidthInput.type = 'number';
          kStrokeWidthInput.className = 'form-control form-control-sm';
          kStrokeWidthInput.value = props.strokeWidth || 14;
          // 函数级注释：实时应用旋钮描边宽度
          kStrokeWidthInput.onchange = (e) => EPWidgets.setProps(node, { strokeWidth: Number(e.target.value) });
          kStrokeWidthRow.appendChild(kStrokeWidthInput);
          styleRow.appendChild(kStrokeWidthRow);

          const kShowValueRow = document.createElement('div');
          kShowValueRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          kShowValueRow.innerHTML = '<div class="prop-label">显示数值</div>';
          const kShowValueInput = document.createElement('input');
          kShowValueInput.id = 'propKnobShowValue';
          kShowValueInput.type = 'checkbox';
          kShowValueInput.className = 'form-check-input';
          kShowValueInput.checked = !!props.showValue;
          // 函数级注释：实时切换旋钮显示数值
          kShowValueInput.onchange = (e) => EPWidgets.setProps(node, { showValue: !!e.target.checked });
          kShowValueRow.appendChild(kShowValueInput);
          styleRow.appendChild(kShowValueRow);

        } else if (type === '切换按钮') {
          const onRow = document.createElement('div');
          onRow.className = 'mb-2';
          onRow.innerHTML = '<div class="prop-label">开启文本</div>';
          const onInput = document.createElement('input');
          onInput.id = 'propToggleLabelOn';
          onInput.className = 'form-control form-control-sm';
          onInput.value = props.labelOn || '开启';
          onRow.appendChild(onInput);
          form.appendChild(onRow);
          const offRow = document.createElement('div');
          offRow.className = 'mb-2';
          offRow.innerHTML = '<div class="prop-label">关闭文本</div>';
          const offInput = document.createElement('input');
          offInput.id = 'propToggleLabelOff';
          offInput.className = 'form-control form-control-sm';
          offInput.value = props.labelOff || '关闭';
          offRow.appendChild(offInput);
          form.appendChild(offRow);
          const activeRow = document.createElement('div');
          activeRow.className = 'mb-2';
          activeRow.innerHTML = '<div class="prop-label">激活</div>';
          const activeInput = document.createElement('input');
          activeInput.id = 'propToggleActive';
          activeInput.type = 'checkbox';
          activeInput.className = 'form-check-input';
          activeInput.checked = !!props.active;
          activeRow.appendChild(activeInput);
          form.appendChild(activeRow);
          
          // 样式设置：按钮颜色（未激活）
          const tBtnColorRow = document.createElement('div');
          tBtnColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tBtnColorRow.innerHTML = '<div class="prop-label">按钮颜色</div>';
          const tBtnColorInput = document.createElement('input');
          tBtnColorInput.id = 'propToggleBtnColor';
          tBtnColorInput.type = 'color';
          tBtnColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tBtnColorInput, props.buttonColor || '#409EFF', '#409EFF');
          tBtnColorInput.onchange = (e) => EPWidgets.setProps(node, { buttonColor: e.target.value });
          tBtnColorRow.appendChild(tBtnColorInput);
          styleRow.appendChild(tBtnColorRow);

          // 样式设置：按下颜色
          const tPressColorRow = document.createElement('div');
          tPressColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tPressColorRow.innerHTML = '<div class="prop-label">按下颜色</div>';
          const tPressColorInput = document.createElement('input');
          tPressColorInput.id = 'propTogglePressColor';
          tPressColorInput.type = 'color';
          tPressColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tPressColorInput, props.pressColor || '#2a7bd8', '#2a7bd8');
          tPressColorInput.onchange = (e) => EPWidgets.setProps(node, { pressColor: e.target.value });
          tPressColorRow.appendChild(tPressColorInput);
          styleRow.appendChild(tPressColorRow);

          // 样式设置：激活颜色（切换后）
          const tActiveColorRow = document.createElement('div');
          tActiveColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tActiveColorRow.innerHTML = '<div class="prop-label">激活颜色</div>';
          const tActiveColorInput = document.createElement('input');
          tActiveColorInput.id = 'propToggleActiveColor';
          tActiveColorInput.type = 'color';
          tActiveColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tActiveColorInput, props.activeColor || '#3a8ee6', '#3a8ee6');
          tActiveColorInput.onchange = (e) => EPWidgets.setProps(node, { activeColor: e.target.value });
          tActiveColorRow.appendChild(tActiveColorInput);
          styleRow.appendChild(tActiveColorRow);

          // 样式设置：文字颜色
          const tTextColorRow = document.createElement('div');
          tTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const tTextColorInput = document.createElement('input');
          tTextColorInput.id = 'propToggleTextColor';
          tTextColorInput.type = 'color';
          tTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tTextColorInput, props.textColor || '#ffffff', '#ffffff');
          tTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          tTextColorRow.appendChild(tTextColorInput);
          styleRow.appendChild(tTextColorRow);

          // 样式设置：边框颜色
          const tBorderColorRow = document.createElement('div');
          tBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const tBorderColorInput = document.createElement('input');
          tBorderColorInput.id = 'propToggleBorderColor';
          tBorderColorInput.type = 'color';
          tBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tBorderColorInput, props.borderColor || '#409EFF', '#409EFF');
          tBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          tBorderColorRow.appendChild(tBorderColorInput);
          styleRow.appendChild(tBorderColorRow);

          // 样式设置：边框样式
          const tBorderStyleRow = document.createElement('div');
          tBorderStyleRow.className = 'mb-2';
          tBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const tBorderStyleSelect = document.createElement('select');
          tBorderStyleSelect.id = 'propToggleBorderStyle';
          tBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            tBorderStyleSelect.appendChild(opt);
          });
          tBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          tBorderStyleRow.appendChild(tBorderStyleSelect);
          styleRow.appendChild(tBorderStyleRow);
        }
        panel.appendChild(form);
        try { __upgradeColorInputs(panel); } catch {}

        let __autoApplyTimer = null;
        const __autoApply = () => {
          if (__autoApplyTimer) clearTimeout(__autoApplyTimer);
          __autoApplyTimer = setTimeout(() => {
            try { applyProperties({ noRefresh: true }); } catch {}
          }, 150);
        };
        try { form.addEventListener('change', __autoApply, true); } catch {}
        try { form.addEventListener('input', __autoApply, true); } catch {}
      }

      // 函数级注释：应用属性修改到选中控件
      function applyProperties(opts) {
        if (!currentSelected) return;
        const type = currentSelected.dataset.type;
        const props = {};
        const titleEl = document.getElementById('propTitle');
        if (titleEl) props.title = titleEl.value.trim();
        if (type === '按钮') {
          props.label = document.getElementById('propBtnLabel').value.trim();
          props.buttonColor = document.getElementById('propBtnColor').value;
          props.activeColor = document.getElementById('propBtnActiveColor').value;
          props.textColor = document.getElementById('propBtnTextColor').value;
          props.borderColor = document.getElementById('propBtnBorderColor').value;
          props.borderStyle = document.getElementById('propBtnBorderStyle').value;
        } else if (type === '超链接') {
          const lblEl = document.getElementById('propLinkLabel');
          const hrefEl = document.getElementById('propLinkHref');
          const tgtEl = document.getElementById('propLinkTargetBlank');
          if (lblEl) props.label = String(lblEl.value || '').trim();
          if (hrefEl) props.href = String(hrefEl.value || '').trim();
          if (tgtEl) props.targetBlank = !!tgtEl.checked;

          const btnColorEl = document.getElementById('propLinkBtnColor');
          const activeColorEl = document.getElementById('propLinkActiveColor');
          const textColorEl = document.getElementById('propLinkTextColor');
          const borderColorEl = document.getElementById('propLinkBorderColor');
          const borderStyleEl = document.getElementById('propLinkBorderStyle');
          if (btnColorEl) props.buttonColor = btnColorEl.value;
          if (activeColorEl) props.activeColor = activeColorEl.value;
          if (textColorEl) props.textColor = textColorEl.value;
          if (borderColorEl) props.borderColor = borderColorEl.value;
          if (borderStyleEl) props.borderStyle = borderStyleEl.value;
        } else if (type === '滑块') {
          props.min = Number(document.getElementById('propSliderMin').value);
          props.max = Number(document.getElementById('propSliderMax').value);
          props.value = Number(document.getElementById('propSliderValue').value);
          const sbEl = document.getElementById('propSliderBarColor');
          const sfEl = document.getElementById('propSliderFillColor');
          const svEl = document.getElementById('propSliderValueColor');
          const sbcEl = document.getElementById('propSliderBorderColor');
          const sbsEl = document.getElementById('propSliderBorderStyle');
          if (sbEl) props.barColor = sbEl.value;
          if (sfEl) props.fillColor = sfEl.value;
          if (svEl) props.valueColor = svEl.value;
          if (sbcEl) props.borderColor = sbcEl.value;
          if (sbsEl) props.borderStyle = sbsEl.value;
        } else if (type === '浮点滑块') {
          props.min = Number(document.getElementById('propFloatSliderMin').value);
          props.max = Number(document.getElementById('propFloatSliderMax').value);
          props.step = Number(document.getElementById('propFloatSliderStep').value);
          props.value = Number(document.getElementById('propFloatSliderValue').value);
          const fsbEl = document.getElementById('propFloatSliderBarColor');
          const fsfEl = document.getElementById('propFloatSliderFillColor');
          const fsvEl = document.getElementById('propFloatSliderValueColor');
          const fsbcEl = document.getElementById('propFloatSliderBorderColor');
          const fsbsEl = document.getElementById('propFloatSliderBorderStyle');
          if (fsbEl) props.barColor = fsbEl.value;
          if (fsfEl) props.fillColor = fsfEl.value;
          if (fsvEl) props.valueColor = fsvEl.value;
          if (fsbcEl) props.borderColor = fsbcEl.value;
          if (fsbsEl) props.borderStyle = fsbsEl.value;
        } else if (type === '勾选') {
          props.label = document.getElementById('propCheckLabel').value.trim();
          props.checked = document.getElementById('propCheckChecked').checked;
          const caEl = document.getElementById('propCheckActiveColor');
          const ctEl = document.getElementById('propCheckTextColor');
          const cbEl = document.getElementById('propCheckBorderColor');
          const cbsEl = document.getElementById('propCheckBorderStyle');
          if (caEl) props.activeColor = caEl.value;
          if (ctEl) props.textColor = ctEl.value;
          if (cbEl) props.borderColor = cbEl.value;
          if (cbsEl) props.borderStyle = cbsEl.value;
        } else if (type === '开关') {
          props.checked = document.getElementById('propSwitchChecked').checked;
          const soEl = document.getElementById('propSwitchOnColor');
          const sfEl = document.getElementById('propSwitchOffColor');
          const sbEl = document.getElementById('propSwitchBorderColor');
          const sbsEl = document.getElementById('propSwitchBorderStyle');
          if (soEl) props.onColor = soEl.value;
          if (sfEl) props.offColor = sfEl.value;
          if (sbEl) props.borderColor = sbEl.value;
          if (sbsEl) props.borderStyle = sbsEl.value;
        } else if (type === '输入框') {
          props.placeholder = document.getElementById('propInputPlaceholder').value.trim();
          props.value = document.getElementById('propInputValue').value;
          const itEl = document.getElementById('propInputTextColor');
          const ibEl = document.getElementById('propInputBorderColor');
          const ibsEl = document.getElementById('propInputBorderStyle');
          if (itEl) props.textColor = itEl.value;
          if (ibEl) props.borderColor = ibEl.value;
          if (ibsEl) props.borderStyle = ibsEl.value;
        } else if (type === '竖向分割线') {
          const vlcEl = document.getElementById('propVDividerLineColor');
          const vlwEl = document.getElementById('propVDividerLineWidth');
          const vbsEl = document.getElementById('propVDividerBorderStyle');
          if (vlcEl) props.lineColor = vlcEl.value;
          if (vlwEl) props.lineWidth = Number(vlwEl.value);
          if (vbsEl) props.borderStyle = vbsEl.value;
        } else if (type === '分割线') {
          props.text = document.getElementById('propDividerText').value.trim();
          const dtEl = document.getElementById('propDividerTextColor');
          const dlcEl = document.getElementById('propDividerLineColor');
          const dlwEl = document.getElementById('propDividerLineWidth');
          const dbsEl = document.getElementById('propDividerBorderStyle');
          if (dtEl) props.textColor = dtEl.value;
          if (dlcEl) props.lineColor = dlcEl.value;
          if (dlwEl) props.lineWidth = Number(dlwEl.value);
          if (dbsEl) props.borderStyle = dbsEl.value;
        } else if (type === '标签') {
          props.text = document.getElementById('propLabelText').value.trim();
          const tColorEl = document.getElementById('propLabelTextColor');
          const bColorEl = document.getElementById('propLabelBorderColor');
          const bStyleEl = document.getElementById('propLabelBorderStyle');
          if (tColorEl) props.textColor = tColorEl.value;
          if (bColorEl) props.borderColor = bColorEl.value;
          if (bStyleEl) props.borderStyle = bStyleEl.value;
        } else if (type === '时间线') {
          const cmdEl = document.getElementById('propTimelineCommandId');
          if (cmdEl) props.commandId = String(cmdEl.value || '').trim();

          const fpsEl = document.getElementById('propTimelineFps');
          const tickEl = document.getElementById('propTimelineTickStepSec');
          if (fpsEl) props.fps = Number(fpsEl.value);
          if (tickEl) props.tickStepSec = Number(tickEl.value);

          const plcEl = document.getElementById('propTimelineLineColor');
          const pcEl = document.getElementById('propTimelinePointColor');
          const ptEl = document.getElementById('propTimelineTextColor');
          const plbgEl = document.getElementById('propTimelineLabelBgColor');
          const pbcEl = document.getElementById('propTimelineBorderColor');
          const pbsEl = document.getElementById('propTimelineBorderStyle');
          if (plcEl) props.lineColor = plcEl.value;
          if (pcEl) props.pointColor = pcEl.value;
          if (ptEl) props.textColor = ptEl.value;
          if (plbgEl) props.labelBgColor = plbgEl.value;
          if (pbcEl) props.borderColor = pbcEl.value;
          if (pbsEl) props.borderStyle = pbsEl.value;

          const nextItems = [];
          const srcItems = Array.isArray(EPWidgets.getProps(currentSelected).items) ? EPWidgets.getProps(currentSelected).items : [];
          srcItems.forEach((it) => {
            const id = it && (it.id ?? it.commandId);
            const text = it && (it.text ?? it.name);
            const sid = (id !== undefined && id !== null) ? String(id) : '';
            const st = (text !== undefined && text !== null) ? String(text) : '';
            nextItems.push({ id: sid, text: st });
          });
          props.items = nextItems;
        } else if (type === '时间码') {
          const fpsEl = document.getElementById('propTimecodeFps');
          const framesEl = document.getElementById('propTimecodeFrames');
          const roEl = document.getElementById('propTimecodeReadOnly');
          const tColorEl = document.getElementById('propTimecodeTextColor');
          const bColorEl = document.getElementById('propTimecodeBorderColor');
          const bStyleEl = document.getElementById('propTimecodeBorderStyle');
          if (fpsEl) props.fps = Number(fpsEl.value);
          if (framesEl) props.value = Number(framesEl.value);
          if (roEl) props.readOnly = !!roEl.checked;
          if (tColorEl) props.textColor = tColorEl.value;
          if (bColorEl) props.borderColor = bColorEl.value;
          if (bStyleEl) props.borderStyle = bStyleEl.value;
        } else if (type === '竖向滑动条') {
          props.min = Number(document.getElementById('propVSliderMin').value);
          props.max = Number(document.getElementById('propVSliderMax').value);
          props.value = Number(document.getElementById('propVSliderValue').value);
          props.height = document.getElementById('propVSliderHeight').value.trim();
          const vbEl = document.getElementById('propVSliderBarColor');
          const vfEl = document.getElementById('propVSliderFillColor');
          const vvEl = document.getElementById('propVSliderValueColor');
          const vbcEl = document.getElementById('propVSliderBorderColor');
          const vbsEl = document.getElementById('propVSliderBorderStyle');
          if (vbEl) props.barColor = vbEl.value;
          if (vfEl) props.fillColor = vfEl.value;
          if (vvEl) props.valueColor = vvEl.value;
          if (vbcEl) props.borderColor = vbcEl.value;
          if (vbsEl) props.borderStyle = vbsEl.value;
        } else if (type === '竖向浮点滑块') {
          props.min = Number(document.getElementById('propVFloatSliderMin').value);
          props.max = Number(document.getElementById('propVFloatSliderMax').value);
          props.step = Number(document.getElementById('propVFloatSliderStep').value);
          props.value = Number(document.getElementById('propVFloatSliderValue').value);
          props.height = document.getElementById('propVFloatSliderHeight').value.trim();
          const vfbEl = document.getElementById('propVFloatSliderBarColor');
          const vffEl = document.getElementById('propVFloatSliderFillColor');
          const vfvEl = document.getElementById('propVFloatSliderValueColor');
          const vfbcEl = document.getElementById('propVFloatSliderBorderColor');
          const vfbsEl = document.getElementById('propVFloatSliderBorderStyle');
          if (vfbEl) props.barColor = vfbEl.value;
          if (vffEl) props.fillColor = vffEl.value;
          if (vfvEl) props.valueColor = vfvEl.value;
          if (vfbcEl) props.borderColor = vfbcEl.value;
          if (vfbsEl) props.borderStyle = vfbsEl.value;
        } else if (type === '旋钮') {
          props.min = Number(document.getElementById('propKnobMin').value);
          props.max = Number(document.getElementById('propKnobMax').value);
          props.value = Number(document.getElementById('propKnobValue').value);
          props.step = Number(document.getElementById('propKnobStep').value);
          props.valueColor = document.getElementById('propKnobValueColor').value;
          const ktEl = document.getElementById('propKnobTextColor');
          const kbEl = document.getElementById('propKnobBorderColor');
          const kbsEl = document.getElementById('propKnobBorderStyle');
          const krEl = document.getElementById('propKnobRangeColor');
          const kswEl = document.getElementById('propKnobStrokeWidth');
          const ksvEl = document.getElementById('propKnobShowValue');
          if (ktEl) props.textColor = ktEl.value;
          if (kbEl) props.borderColor = kbEl.value;
          if (kbsEl) props.borderStyle = kbsEl.value;
          if (krEl) props.rangeColor = krEl.value;
          if (kswEl) props.strokeWidth = Number(kswEl.value);
          if (ksvEl) props.showValue = ksvEl.checked;
        } else if (type === '切换按钮') {
          props.labelOn = document.getElementById('propToggleLabelOn').value.trim();
          props.labelOff = document.getElementById('propToggleLabelOff').value.trim();
          props.active = document.getElementById('propToggleActive').checked;
          // 样式属性
          const btnColorEl = document.getElementById('propToggleBtnColor');
          const pressColorEl = document.getElementById('propTogglePressColor');
          const activeColorEl = document.getElementById('propToggleActiveColor');
          const textColorEl = document.getElementById('propToggleTextColor');
          const borderColorEl = document.getElementById('propToggleBorderColor');
          const borderStyleEl = document.getElementById('propToggleBorderStyle');
          if (btnColorEl) props.buttonColor = btnColorEl.value;
          if (pressColorEl) props.pressColor = pressColorEl.value;
          if (activeColorEl) props.activeColor = activeColorEl.value;
          if (textColorEl) props.textColor = textColorEl.value;
          if (borderColorEl) props.borderColor = borderColorEl.value;
          if (borderStyleEl) props.borderStyle = borderStyleEl.value;
        }
        EPWidgets.setProps(currentSelected, props);
        try {
          if (typeof window.__saveLayoutLocal === 'function' && NS.activeTabId) {
            const info = NS.grids.get(NS.activeTabId);
            if (info && info.grid) window.__saveLayoutLocal(NS.activeTabId, info.grid);
          }
        } catch {}
        if (!(opts && opts.noRefresh)) updatePropPanel(currentSelected);
      }

    function attachAndSelect(node, type) {
      if (!node) return;
      const editModeSwitch = document.getElementById('editModeSwitch');
      if (editModeSwitch && !editModeSwitch.checked) {
        editModeSwitch.checked = true;
        editModeSwitch.dispatchEvent(new Event('change'));
      }
      node.dataset.type = type;
      NSInteract.attachOverlay(node);
      if (typeof window.__selectNode === 'function') window.__selectNode(node);
    }

    function saveLayout(grid) {
      try { saveAllLayouts(); } catch (e) { alert('保存失败: ' + e); }
    }

    function saveAllLayouts() {
      try {
        const pages = {};
        NS.grids.forEach((info, tid) => {
          const items = NSUtils.collectGridItems(info.grid);
          const design = (info && info.design) ? info.design : { width: EPWidgets.layoutDefaults.designWidth, height: EPWidgets.layoutDefaults.designHeight };
          pages[tid] = { design, items };
        });
        const payload = { tabs: NS.tabs, activeTabId: NS.activeTabId, pages };
        fetch('/api/layout/save', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        })
        .then(r => r.text())
        .then(text => {
          try {
            const j = JSON.parse(text);
            if (j.ok) { /* saved */ }
            else alert('保存失败: ' + (j.error || '未知错误'));
          } catch (e) {
            if (text.includes('"ok":true')) { /* saved */ }
            else throw new Error('服务器响应格式错误: ' + text.substring(0, 50) + '...');
          }
        })
        .catch(e => alert('保存请求失败: ' + e));
      } catch (err) { alert('保存异常: ' + err); }
    }

    // 暴露给 ns-interact 和 ns-prop-panel 使用的内部引用
    window.NSDashboard = {
      getActiveGrid,
      saveLayoutLocal,
      saveLayout,
      saveAllLayouts,
      attachAndSelect,
      getGlobalEditMode,
      applyEditModeAll,
      setGlobalEditMode,
      updatePropPanel,
      applyProperties,
      undoLayout,
      redoLayout,
      // 暴露闭包变量访问器
      _getSelectedNodes: () => selectedNodes,
      _getCurrentSelected: () => currentSelected,
      _getAnchorSelected: () => anchorSelected,
      _setCurrentSelected: (v) => { currentSelected = v; },
      _setAnchorSelected: (v) => { anchorSelected = v; },
      __initColorInput,
      __upgradeColorInputs,
      __closeEpColorPickers
    };
  }

  window.addEventListener('DOMContentLoaded', initDashboard);
})();
