// ns-dashboard-tabs.js —— 标签页创建、切换、删除
(function() {
  'use strict';

  function init(ctx) {
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
        const info = ctx.services.NS.grids.get(tid);
        const oldName = (info && info.name) || name || '';
        const input = document.createElement('input');
        input.type = 'text';
        input.className = 'form-control form-control-sm';
        input.style.width = '140px';
        input.value = oldName;
        input.setAttribute('aria-label', '页面名称');
        input.title = '页面名称';
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
          const idx = ctx.services.NS.tabs.findIndex(t => t.id === tid);
          if (idx >= 0) {
            ctx.services.NS.tabs[idx].name = finalName;
            try { localStorage.setItem('ns_tabs', JSON.stringify(ctx.services.NS.tabs)); } catch {}
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
      ctx.services.NS.grids.set(tid, { name, tabBtn, pageWrap, viewportEl, grid, canvasEl, view: null, design: { width: EPWidgets.layoutDefaults.designWidth, height: EPWidgets.layoutDefaults.designHeight, bgColor: EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc' }, loaded: false, renderSeq: 0, rendering: false });
      ctx.services.NSCanvas.applyPageDesign(tid);
      try { ctx.services.NSCanvas.__initCanvasPanZoom(tid); } catch {}

      if (!ctx.services.NS.tabs.find(t => t.id === tid)) {
        ctx.services.NS.tabs.push({ id: tid, name });
        try {
          localStorage.setItem('ns_tabs', JSON.stringify(ctx.services.NS.tabs));
          if (!(opts && opts.skipSwitch)) localStorage.setItem('ns_active_tab', tid);
        } catch (err) { console.warn('Persist tabs failed', err); }
      }
      const skipLocal = opts && opts.skipLocalLoad;
      const skipSwitch = opts && opts.skipSwitch;
      if (!skipLocal) ctx.layout.loadLayoutLocal(grid, tid, true);
      if (!skipSwitch) switchTab(tid);
      return tid;
    }
    function renderTabLabel(tabBtn, name, tid) {
      tabBtn.textContent = name;
      const close = document.createElement('span');
      close.className = 'tab-close';
      close.title = '删除页面';
      close.textContent = '\u00d7';
      close.onclick = (e) => { e.stopPropagation(); deleteTab(tid); };
      try { close.style.display = ctx.edit.getGlobalEditMode() ? '' : 'none'; } catch {}
      tabBtn.appendChild(close);
    }

    function deleteTab(tid) {
      try {
        if (!ctx.edit.getGlobalEditMode()) { alert('请切换到编辑模式后删除页面'); return; }
        if (ctx.services.NS.grids.size <= 1) { alert('至少保留一个页面'); return; }
        const info = ctx.services.NS.grids.get(tid);
        if (!info) return;
        const nameLabel = (info && info.name) ? String(info.name) : String(tid);
        if (!window.confirm('确认删除页面"' + nameLabel + '"？删除后不可恢复')) return;
        if (info.pageWrap && info.pageWrap.parentElement) {
          info.pageWrap.parentElement.removeChild(info.pageWrap);
        }
        if (info.tabBtn && info.tabBtn.parentElement) {
          info.tabBtn.parentElement.removeChild(info.tabBtn);
        }
        ctx.services.NS.grids.delete(tid);
        const idx = ctx.services.NS.tabs.findIndex(t => t.id === tid);
        if (idx >= 0) ctx.services.NS.tabs.splice(idx, 1);
        try { localStorage.removeItem('ns_layout_' + tid); } catch {}
        try { localStorage.setItem('ns_tabs', JSON.stringify(ctx.services.NS.tabs)); } catch {}
        if (ctx.services.NS.activeTabId === tid) {
          const next = ctx.services.NS.tabs[0] ? ctx.services.NS.tabs[0].id : (ctx.services.NS.grids.keys().next().value || null);
          if (next) {
            try { localStorage.setItem('ns_active_tab', next); } catch {}
            switchTab(next);
          } else {
            ctx.services.NS.activeTabId = null;
          }
        }
      } catch (err) {
        alert('删除页面失败: ' + err);
      }
    }

    function switchTab(id) {
      ctx.selection.clearSelection();
      ctx.services.NS.activeTabId = id;
      ctx.services.NS.grids.forEach((info, tid) => {
        const active = tid === id;
        info.pageWrap.style.display = active ? 'block' : 'none';
        info.tabBtn.classList.toggle('active', active);
      });
      const info = ctx.services.NS.grids.get(id);
      if (info && !info.loaded && !info.rendering) {
        const ok = ctx.layout.loadLayoutLocal(info.grid, id, true);
        if (!ok) {
          info.loaded = true;
          try { ctx.services.NSInteract.loadLayout(info.grid, true); } catch {}
        }
      }
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
      try {
        if (info && info.loaded && info.grid) ctx.history.seedHistorySnapshot(id, info.grid);
      } catch {}
      try { ctx.services.NSWsSync.queryAllStatuses(); } catch {}
    }
    return {
      createTab,
      renderTabLabel,
      deleteTab,
      switchTab
    };
  }

  window.NSDashboardTabs = { init };
})();
