// ns-dashboard.js —— 仪表盘入口：组装 NSDashboardCtx 并暴露 NSDashboard API
(function() {
  'use strict';

  function initDashboard() {
    try {
      const ctx = window.NSDashboardCtxFactory.create();

      ctx.history = window.NSDashboardHistory.init(ctx);
      ctx.layout = window.NSDashboardLayout.init(ctx);
      ctx.props = window.NSDashboardPropPanel.init(ctx);
      ctx.selection = window.NSDashboardSelection.init(ctx);
      ctx.edit = window.NSDashboardEdit.init(ctx);
      ctx.tabs = window.NSDashboardTabs.init(ctx);

      try { ctx.services.NSUtils.mergeLeftSidebarIntoRight(); } catch {}

      try { ctx.selection.bindMousePosTracking(); } catch (err) { console.error('bindMousePosTracking failed', err); }
      try { ctx.selection.bindMarqueeSelection(); } catch (err) { console.error('bindMarqueeSelection failed', err); }
      try { ctx.selection.bindGroupHitTestOnCanvas(); } catch (err) { console.error('bindGroupHitTestOnCanvas failed', err); }
      try { ctx.selection.bindAlignActions(); } catch (err) { console.error('bindAlignActions failed', err); }
      try { window.NSDashboardContextMenu.init(ctx); } catch (err) { console.error('contextMenu init failed', err); }

      document.querySelectorAll('.sidebar-section-header').forEach(header => {
        header.onclick = () => { header.parentElement.classList.toggle('collapsed'); };
      });

      const addTabBtn = document.getElementById('sidebarAddTab');
      if (addTabBtn) {
        addTabBtn.onclick = () => {
          try { if (!ctx.edit.getGlobalEditMode()) return; } catch {}
          const tid = ctx.tabs.createTab('页面' + (ctx.services.NS.grids.size + 1));
          try {
            localStorage.setItem('ns_active_tab', tid);
            localStorage.setItem('ns_tabs', JSON.stringify(ctx.services.NS.tabs));
          } catch {}
        };
      }

      ctx.edit.bindEditModeControls();

      try {
        const rawTabs = localStorage.getItem('ns_tabs');
        ctx.services.NS.tabs = Array.isArray(JSON.parse(rawTabs || '[]')) ? JSON.parse(rawTabs || '[]') : [];
      } catch { ctx.services.NS.tabs = []; }

      if (ctx.services.NS.tabs.length > 0) {
        ctx.services.NS.tabs.forEach(t => ctx.tabs.createTab(t.name, t.id));
        const savedActive = localStorage.getItem('ns_active_tab');
        if (savedActive && ctx.services.NS.grids.has(savedActive)) ctx.tabs.switchTab(savedActive);
        else ctx.tabs.switchTab(ctx.services.NS.tabs[0].id);
      } else {
        const defaultId = ctx.tabs.createTab('页面1');
        try {
          localStorage.setItem('ns_tabs', JSON.stringify([{ id: defaultId, name: '页面1' }]));
          localStorage.setItem('ns_active_tab', defaultId);
        } catch {}
      }

      ctx.edit.restoreEditModeOnLoad();
      try { window.NSInteract.loadAllLayouts(); } catch {}

      function attachAndSelect(node, type) {
        if (!node) return;
        const editModeSwitch = document.getElementById('editModeSwitch');
        if (editModeSwitch && !editModeSwitch.checked) {
          editModeSwitch.checked = true;
          editModeSwitch.dispatchEvent(new Event('change'));
        }
        node.dataset.type = type;
        try { window.NSInteract.attachOverlay(node); } catch {}
        ctx.selection.selectNode(node);
        try {
          if (ctx.services.NS.activeTabId) ctx.layout.commitLayoutLocal(ctx.services.NS.activeTabId, ctx.layout.getActiveGrid());
        } catch {}
      }

      window.NSDashboardCtxFactory.buildPublicApi(ctx, { attachAndSelect });
      window.NSDashboardCtx = ctx;
      window.NSDashboard = ctx.api;

      try {
        if (window.NSWidgetLibraryInit) {
          window.NSWidgetLibraryInit({
            getActiveGrid: ctx.layout.getActiveGrid,
            attachAndSelect,
            setGlobalEditMode: ctx.edit.setGlobalEditMode,
            applyEditModeAll: ctx.edit.applyEditModeAll,
            getGlobalEditMode: ctx.edit.getGlobalEditMode
          });
        }
      } catch (err) { console.error('NSWidgetLibrary init failed', err); }
    } catch (err) {
      console.error('initDashboard failed', err);
    }
  }

  window.addEventListener('DOMContentLoaded', initDashboard);
})();
