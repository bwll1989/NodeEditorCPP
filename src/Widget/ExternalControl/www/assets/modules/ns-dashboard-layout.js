// ns-dashboard-layout.js —— 本地布局读写与服务端保存
(function() {
  'use strict';

  function init(ctx) {
    function getActiveGrid() {
      if (!ctx.services.NS.activeTabId) return null;
      const info = ctx.services.NS.grids.get(ctx.services.NS.activeTabId);
      return info ? info.grid : null;
    }

    // 函数级注释：将布局保存到本地存储（防抖；历史栈由 commit 触发）
    function saveLayoutLocal(tid, grid) {
      try {
        if (window.NSLayoutPersist && typeof window.NSLayoutPersist.scheduleSave === 'function') {
          window.NSLayoutPersist.scheduleSave(tid, grid);
        }
      } catch (err) {
        console.error('saveLayoutLocal failed', err);
      }
    }

    function commitLayoutLocal(tid, grid) {
      try {
        if (window.NSLayoutPersist && typeof window.NSLayoutPersist.commitSave === 'function') {
          window.NSLayoutPersist.commitSave(tid, grid);
        }
      } catch (err) {
        console.error('commitLayoutLocal failed', err);
      }
    }

    try {
      if (window.NSLayoutPersist && typeof window.NSLayoutPersist.init === 'function') {
        window.NSLayoutPersist.init({
          collectPage(tid, grid) {
            const items = ctx.services.NSUtils.collectGridItems(grid);
            const info = ctx.services.NS.grids.get(tid);
            const design = (info && info.design) ? info.design : {
              width: 320,
              height: 240,
              bgColor: EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc',
              auto: true
            };
            return { design, items };
          },
          pushHistory: (tid, grid) => ctx.history.pushHistorySnapshot(tid, grid),
          saveAllLayoutsSilent(payload) {
            fetch('/api/layout/save', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify(payload)
            }).catch(err => console.warn('NSLayoutPersist: auto-save failed', err));
          }
        });
      }
    } catch (err) {
      console.error('NSLayoutPersist init failed', err);
    }

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
        if (design) ctx.services.NSCanvas.applyPageDesign(tid, design);

        if (items.length === 0) {
          if (!silent) alert('当前页面本地布局为空');
          return false;
        }

        ctx.services.NSCanvas.clearLayoutContainer(grid);
        const seq = ctx.history.beginTabRender(tid);
        try { const info0 = ctx.services.NS.grids.get(tid); if (info0) info0.rendering = true; } catch {}
        ctx.services.NSUtils.ensureWidgetTypesReady((items || []).map(s => s && s.type), () => {
          if (!ctx.history.isTabRenderCurrent(tid, seq)) return;
          items.forEach(spec => { ctx.services.NSUtils.createWidgetFromSpec(grid, spec); });
          try { ctx.services.NSCanvas.fitCanvasToWidgets(tid); } catch {}
          try { const info2 = ctx.services.NS.grids.get(tid); if (info2) { info2.loaded = true; info2.rendering = false; } } catch {}
          try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
          try { ctx.services.NSWsSync.queryAllStatuses(); } catch {}
          try { ctx.history.seedHistorySnapshot(tid, grid); } catch {}
        });
        return true;
      } catch (err) {
        console.error('loadLayoutLocal failed', err);
        if (!silent) alert('本地布局加载失败: ' + err);
        return false;
      }
    }

    function saveLayout(grid) {
      try { saveAllLayouts(); } catch (e) { alert('保存失败: ' + e); }
    }

    function saveAllLayouts() {
      try {
        if (window.NSLayoutPersist && typeof window.NSLayoutPersist.flushAllPending === 'function') {
          window.NSLayoutPersist.flushAllPending();
        }
        if (window.NSLayoutPersist && typeof window.NSLayoutPersist.invalidateServerHash === 'function') {
          window.NSLayoutPersist.invalidateServerHash();
        }
        const pages = {};
        ctx.services.NS.grids.forEach((info, tid) => {
          const items = ctx.services.NSUtils.collectGridItems(info.grid);
          const design = (info && info.design) ? info.design : { width: 320, height: 240, auto: true };
          pages[tid] = { design, items };
        });
        const payload = { tabs: ctx.services.NS.tabs, activeTabId: ctx.services.NS.activeTabId, pages };
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

    return {
      getActiveGrid,
      saveLayoutLocal,
      commitLayoutLocal,
      loadLayoutLocal,
      saveLayout,
      saveAllLayouts
    };
  }

  window.NSDashboardLayout = { init };
})();
