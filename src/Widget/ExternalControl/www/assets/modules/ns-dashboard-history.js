// ns-dashboard-history.js —— 布局快照、撤销恢复、复制粘贴
(function() {
  'use strict';

  function init(ctx) {
    function snapshotGrid(tid, grid) {
      try {
        const info = ctx.services.NS.grids.get(tid);
        const design = (info && info.design) ? info.design : { width: EPWidgets.layoutDefaults.designWidth, height: EPWidgets.layoutDefaults.designHeight, bgColor: EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc' };
        const items = ctx.services.NSUtils.collectGridItems(grid);
        return { design, items };
      } catch { return { design: {}, items: [] }; }
    }

    function isEmptySnapshot(snap) {
      return !snap || !Array.isArray(snap.items) || snap.items.length === 0;
    }

    // 函数级注释：移除栈底因异步加载竞态产生的空快照（兼容旧历史数据）
    function normalizeHistoryStack(past) {
      const stack = Array.isArray(past) ? past.slice() : [];
      while (stack.length > 1 && isEmptySnapshot(stack[0]) && !isEmptySnapshot(stack[1])) {
        stack.shift();
      }
      return stack;
    }

    // 函数级注释：仅在历史为空时写入初始基线（布局加载完成后调用）
    function seedHistorySnapshot(tid, grid) {
      try {
        if (!tid || !grid) return;
        const key = 'ns_hist_' + tid;
        let past = Array.isArray(JSON.parse(localStorage.getItem(key) || '[]')) ? JSON.parse(localStorage.getItem(key) || '[]') : [];
        if (past.length > 0) return;
        past.push(snapshotGrid(tid, grid));
        localStorage.setItem(key, JSON.stringify(past));
        localStorage.removeItem(key + '_future');
      } catch {}
    }
    // 函数级注释：将当前页快照压入历史（去重 & 限制长度）
    function pushHistorySnapshot(tid, grid) {
      try {
        if (!tid || !grid) return;
        const snap = snapshotGrid(tid, grid);
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
    function beginTabRender(tid) {
      const info = tid ? ctx.services.NS.grids.get(tid) : null;
      if (!info) return 0;
      info.renderSeq = (Number(info.renderSeq) || 0) + 1;
      return info.renderSeq;
    }

    // 函数级注释：判断某次渲染回调是否仍为最新
    function isTabRenderCurrent(tid, seq) {
      const info = tid ? ctx.services.NS.grids.get(tid) : null;
      if (!info) return false;
      return (Number(info.renderSeq) || 0) === (Number(seq) || 0);
    }

    // 暴露到全局，供 loadLayout 等外部函数使用
    // 函数级注释：应用快照到当前页（带渲染序号防重复）
    function applySnapshot(tid, data) {
      try {
        const info = ctx.services.NS.grids.get(tid);
        if (!info) return;
        const grid = info.grid;
        ctx.services.NSCanvas.clearLayoutContainer(grid);
        const seq = beginTabRender(tid);
        ctx.services.NSCanvas.applyPageDesign(tid, data.design);
        ctx.services.NSUtils.ensureWidgetTypesReady((data.items || []).map(s => s && s.type), () => {
          if (!isTabRenderCurrent(tid, seq)) return;
          (data.items || []).forEach(spec => { ctx.services.NSUtils.createWidgetFromSpec(grid, spec); });
          try { ctx.services.NSWsSync.queryAllStatuses(); } catch {}
          try {
            const payload = { design: data.design || {}, items: data.items || [] };
            localStorage.setItem('ns_layout_' + tid, JSON.stringify(payload));
          } catch {}
          try { ctx.selection.clearSelection(); } catch {}
        });
        try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
      } catch {}
    }
    // 函数级注释：撤销/恢复（基于本地历史栈）
    function undoLayout() {
      try {
        const tid = ctx.services.NS.activeTabId; if (!tid) return;
        const key = 'ns_hist_' + tid; const fkey = key + '_future';
        let past = Array.isArray(JSON.parse(localStorage.getItem(key) || '[]')) ? JSON.parse(localStorage.getItem(key) || '[]') : [];
        const normalized = normalizeHistoryStack(past);
        if (normalized.length !== past.length) {
          past = normalized;
          localStorage.setItem(key, JSON.stringify(past));
        }
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
        const tid = ctx.services.NS.activeTabId; if (!tid) return;
        const key = 'ns_hist_' + tid; const fkey = key + '_future';
        const past = Array.isArray(JSON.parse(localStorage.getItem(key) || '[]')) ? JSON.parse(localStorage.getItem(key) || '[]') : [];
        const future = Array.isArray(JSON.parse(localStorage.getItem(fkey) || '[]')) ? JSON.parse(localStorage.getItem(fkey) || '[]') : [];
        if (future.length < 1) return;
        const snap = future.pop(); localStorage.setItem(fkey, JSON.stringify(future));
        past.push(snap); localStorage.setItem(key, JSON.stringify(past));
        applySnapshot(ctx.services.NS.activeTabId, snap);
      } catch {}
    }
    // 函数级注释：复制/粘贴当前选择
    function copySelection() {
      const wrap = Array.from(document.querySelectorAll('#tabsContent [data-tab-id]')).find(el => el.style.display !== 'none');
      const canvas = wrap ? wrap.querySelector('.pixel-canvas') : null;
      if (!canvas) return;
      let nodes = Array.from(ctx.state.selectedNodes);
      const gs = ctx.state.selectedGroupIds;
      if ((!nodes || nodes.length === 0) && gs && gs.size > 0) {
        nodes = [];
        Array.from(gs).forEach(gid => {
          const m = ctx.services.NSUtils.__getGroupMembers(canvas, gid);
          nodes.push(...m);
        });
      }
      if (!nodes || nodes.length === 0) return;
      const specs = nodes.map(n => {
        const props = EPWidgets.getProps(n);
        const r = ctx.services.NSUtils.__readRectPx(n);
        const type = String(n.dataset.type || '');
        const chain = ctx.services.NSUtils.__getNodeGroupChain(n);
        const gid = (chain && chain.length) ? String(chain[chain.length - 1] || '').trim() : '';
        const spec = { type, props, rect: { x: r.x, y: r.y, w: r.w, h: r.h } };
        if (gid) spec.groupId = gid;
        if (chain && chain.length > 1) spec.groupChain = chain.join(',');
        return spec;
      });
      const bbox = specs.reduce((b, s) => ({ x: Math.min(b.x, s.rect.x), y: Math.min(b.y, s.rect.y) }), { x: Infinity, y: Infinity });
      const clip = { items: specs, bbox, tabId: ctx.services.NS.activeTabId };
      ctx.state.clipboard = clip;
      try { localStorage.setItem('ns_clipboard', JSON.stringify(clip)); } catch {}
    }
    function pasteSelection() {
      try {
        if (ctx.state.pasteLock) return;
        ctx.state.pasteLock = true;
        setTimeout(() => { try { ctx.state.pasteLock = false; } catch {} }, 0);

        const clip = ctx.state.clipboard || JSON.parse(localStorage.getItem('ns_clipboard') || '{}');
        if (!clip || !Array.isArray(clip.items) || clip.items.length === 0) return;
        const grid = ctx.layout.getActiveGrid(); if (!grid) return;
        const pos = ctx.state.lastMouseCanvasPos || null;
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
        ctx.services.NSUtils.ensureWidgetTypesReady((clip.items || []).map(s => s && s.type), () => {
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
            ctx.services.NSUtils.createWidgetFromSpec(grid, s);
          });
        });
        try { ctx.layout.commitLayoutLocal(ctx.services.NS.activeTabId, grid); } catch {}
      } catch {}
    }
    return {
      seedHistorySnapshot,
      isEmptySnapshot,
      pushHistorySnapshot,
      beginTabRender,
      isTabRenderCurrent,
      applySnapshot,
      undoLayout,
      redoLayout,
      copySelection,
      pasteSelection
    };
  }

  window.NSDashboardHistory = { init };
})();
