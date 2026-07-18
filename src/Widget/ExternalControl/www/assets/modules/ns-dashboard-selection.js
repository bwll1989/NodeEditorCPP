// ns-dashboard-selection.js —— 选区、对齐、分组
(function() {
  'use strict';

  function init(ctx) {
    function updateAlignHint() {
      const el = document.getElementById('alignSelectionHint');
      if (!el) return;
      const gs = ctx.state.selectedGroupIds;
      const gCount = gs ? Number(gs.size || 0) : 0;
      const nCount = Number(ctx.state.selectedNodes.size || 0);
      if (nCount > 0 && gCount > 0) { el.textContent = '已选组' + String(gCount) + ' 控件' + String(nCount); return; }
      if (nCount > 0) { el.textContent = '已选' + String(nCount); return; }
      if (gCount > 0) { el.textContent = '已选组' + String(gCount); return; }
      el.textContent = '已选0';
    }

    function clearSelection() {
      try { ctx.state.selectedNodes.forEach(n => { try { n.classList.remove('grid-selected'); } catch {} }); } catch {}
      ctx.state.selectedNodes.clear();
      ctx.state.currentSelected = null;
      ctx.state.anchorSelected = null;
      ctx.state.selectedGroupIds = new Set(); ctx.state.selectedGroupAnchor = ''; ctx.state.groupDrillId = '';
      updateAlignHint();
      ctx.props.updatePropPanel(null);
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function removeFromSelection(node) {
      if (!node) return;
      try { ctx.state.selectedNodes.delete(node); } catch {}
      try { node.classList.remove('grid-selected'); } catch {}
      if (ctx.state.currentSelected === node) ctx.state.currentSelected = null;
      if (ctx.state.anchorSelected === node) ctx.state.anchorSelected = null;
      if (!ctx.state.anchorSelected && ctx.state.selectedNodes.size > 0) ctx.state.anchorSelected = ctx.state.selectedNodes.values().next().value || null;
      if (!ctx.state.currentSelected) ctx.state.currentSelected = ctx.state.anchorSelected || ctx.state.selectedNodes.values().next().value || null;
      updateAlignHint();
      ctx.props.updatePropPanel(ctx.state.currentSelected);
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }
    function selectNode(node, opts) {
      if (!node) { clearSelection(); return; }
      const isToggle = !!(opts && opts.toggle);
      if (!isToggle) {
        ctx.state.selectedGroupIds = new Set(); ctx.state.selectedGroupAnchor = '';
        ctx.state.selectedNodes.forEach(n => { if (n !== node) n.classList.remove('grid-selected'); });
        ctx.state.selectedNodes.clear();
        ctx.state.selectedNodes.add(node);
        node.classList.add('grid-selected');
        ctx.state.currentSelected = node;
        ctx.state.anchorSelected = node;
        try { ctx.state.groupDrillId = String((node && node.dataset ? node.dataset.groupId : '') || '').trim(); } catch {}
        updateAlignHint();
        ctx.props.updatePropPanel(node);
        try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
        return;
      }
      if (ctx.state.selectedNodes.has(node)) {
        ctx.state.selectedNodes.delete(node);
        node.classList.remove('grid-selected');
        if (ctx.state.anchorSelected === node) ctx.state.anchorSelected = null;
        if (ctx.state.currentSelected === node) ctx.state.currentSelected = null;
        if (!ctx.state.anchorSelected && ctx.state.selectedNodes.size > 0) ctx.state.anchorSelected = ctx.state.selectedNodes.values().next().value || null;
        if (!ctx.state.currentSelected) ctx.state.currentSelected = ctx.state.anchorSelected || ctx.state.selectedNodes.values().next().value || null;
        try { ctx.state.groupDrillId = ctx.state.currentSelected ? String((ctx.state.currentSelected.dataset ? ctx.state.currentSelected.dataset.groupId : '') || '').trim() : ''; } catch {}
        updateAlignHint();
        ctx.props.updatePropPanel(ctx.state.currentSelected);
        return;
      }
      ctx.state.selectedNodes.add(node);
      node.classList.add('grid-selected');
      ctx.state.currentSelected = node;
      if (!ctx.state.anchorSelected) ctx.state.anchorSelected = ctx.state.selectedNodes.values().next().value || node;
      try { ctx.state.groupDrillId = String((node && node.dataset ? node.dataset.groupId : '') || '').trim(); } catch {}
      updateAlignHint();
      ctx.props.updatePropPanel(ctx.state.currentSelected);
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function selectGroup(groupId, opts) {
      const gid = String(groupId || '').trim();
      if (!gid) return;
      const isToggle = !!(opts && opts.toggle);
      if (!isToggle) {
        try {
          ctx.state.selectedNodes.forEach(n => { try { n.classList.remove('grid-selected'); } catch {} });
          ctx.state.selectedNodes.clear();
          ctx.state.currentSelected = null;
          ctx.state.anchorSelected = null;
          ctx.state.groupDrillId = '';
        } catch {}
      }
      try {
        const gs = ctx.state.selectedGroupIds;
        if (!isToggle) {
          gs.clear();
          gs.add(gid);
          ctx.state.selectedGroupIds = gs;
          ctx.state.selectedGroupAnchor = gid;
        } else {
          if (gs.has(gid)) gs.delete(gid); else gs.add(gid);
          ctx.state.selectedGroupIds = gs;
          if (String(ctx.state.selectedGroupAnchor || '').trim() && !gs.has(String(ctx.state.selectedGroupAnchor || '').trim())) {
            ctx.state.selectedGroupAnchor = (gs.size ? (gs.values().next().value || '') : '');
          } else if (!String(ctx.state.selectedGroupAnchor || '').trim() && gs.size) {
            ctx.state.selectedGroupAnchor = (gs.values().next().value || '');
          }
        }
      } catch {}
      updateAlignHint();
      if (!isToggle) ctx.props.updatePropPanel(null);
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function selectNodes(nodes) {
      const list = Array.from(nodes || []).filter(Boolean);
      ctx.state.selectedGroupIds = new Set();
      ctx.state.selectedGroupAnchor = '';
      ctx.state.groupDrillId = '';
      try {
        ctx.state.selectedNodes.forEach(n => { try { n.classList.remove('grid-selected'); } catch {} });
      } catch {}
      ctx.state.selectedNodes.clear();
      if (!list.length) {
        ctx.state.currentSelected = null;
        ctx.state.anchorSelected = null;
        updateAlignHint();
        ctx.props.updatePropPanel(null);
        try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
        return;
      }
      list.forEach(node => {
        ctx.state.selectedNodes.add(node);
        node.classList.add('grid-selected');
      });
      ctx.state.anchorSelected = list[0];
      ctx.state.currentSelected = list[list.length - 1];
      try { ctx.state.groupDrillId = String((list[list.length - 1].dataset && list[list.length - 1].dataset.groupId) || '').trim(); } catch {}
      updateAlignHint();
      ctx.props.updatePropPanel(ctx.state.currentSelected);
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function bindMarqueeSelection() {
      const host = document.getElementById('tabsContent');
      if (!host || host.dataset.marqueeBound) return;
      host.dataset.marqueeBound = '1';

      const MARQUEE_MIN = 4;

      function rectsIntersect(a, b) {
        if (!a || !b || a.w <= 0 || a.h <= 0 || b.w <= 0 || b.h <= 0) return false;
        return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
      }

      function resolveCanvasFromEvent(e) {
        let canvas = (e.target && e.target.closest) ? e.target.closest('.pixel-canvas') : null;
        if (!canvas) {
          const vp = (e.target && e.target.closest) ? e.target.closest('.canvas-viewport') : null;
          if (vp) canvas = vp.querySelector('.pixel-canvas');
        }
        return canvas;
      }

      let active = null;

      function removeMarquee() {
        if (active && active.marqueeEl && active.marqueeEl.parentElement) {
          try { active.marqueeEl.parentElement.removeChild(active.marqueeEl); } catch {}
        }
        active = null;
      }

      function finishMarquee(selectRect) {
        if (!active) return;
        const canvas = active.canvas;
        const hits = [];
        if (selectRect && selectRect.w >= MARQUEE_MIN && selectRect.h >= MARQUEE_MIN) {
          canvas.querySelectorAll('.grid-stack-item').forEach(node => {
            const r = ctx.services.NSUtils.__readRectPx(node);
            if (rectsIntersect(selectRect, r)) hits.push(node);
          });
        }
        removeMarquee();
        if (hits.length) selectNodes(hits);
        else if (selectRect && selectRect.w >= MARQUEE_MIN && selectRect.h >= MARQUEE_MIN) clearSelection();
      }

      host.addEventListener('pointerdown', (e) => {
        if (!e || e.button !== 0 || !e.shiftKey) return;
        if (e.ctrlKey || e.metaKey) return;
        try { if (!document.body.classList.contains('edit-mode')) return; } catch {}
        try { if (ctx.services.NSInteract.__isMobileLike && ctx.services.NSInteract.__isMobileLike()) return; } catch {}
        const canvas = resolveCanvasFromEvent(e);
        if (!canvas) return;
        if ((e.target && e.target.closest) ? e.target.closest('.grid-stack-item') : null) return;

        try { if (e.cancelable) e.preventDefault(); } catch {}
        try { e.stopPropagation(); } catch {}
        try { e.stopImmediatePropagation(); } catch {}

        const p0 = ctx.services.NSCanvas.__clientToCanvasDesignXY(canvas, e.clientX, e.clientY);
        const marqueeEl = document.createElement('div');
        marqueeEl.className = 'ns-marquee-select';
        marqueeEl.style.left = p0.x + 'px';
        marqueeEl.style.top = p0.y + 'px';
        marqueeEl.style.width = '0px';
        marqueeEl.style.height = '0px';
        canvas.appendChild(marqueeEl);

        active = {
          canvas,
          marqueeEl,
          startX: Number(p0.x) || 0,
          startY: Number(p0.y) || 0,
          pointerId: e.pointerId,
        };

        const onMove = (ev) => {
          if (!active || ev.pointerId !== active.pointerId) return;
          const p1 = ctx.services.NSCanvas.__clientToCanvasDesignXY(active.canvas, ev.clientX, ev.clientY);
          const x = Math.min(active.startX, p1.x);
          const y = Math.min(active.startY, p1.y);
          const w = Math.abs(p1.x - active.startX);
          const h = Math.abs(p1.y - active.startY);
          active.marqueeEl.style.left = x + 'px';
          active.marqueeEl.style.top = y + 'px';
          active.marqueeEl.style.width = w + 'px';
          active.marqueeEl.style.height = h + 'px';
        };

        const onUp = (ev) => {
          if (!active || ev.pointerId !== active.pointerId) return;
          document.removeEventListener('pointermove', onMove, true);
          document.removeEventListener('pointerup', onUp, true);
          document.removeEventListener('pointercancel', onUp, true);
          try { active.canvas.releasePointerCapture(ev.pointerId); } catch {}

          const p1 = ctx.services.NSCanvas.__clientToCanvasDesignXY(active.canvas, ev.clientX, ev.clientY);
          finishMarquee({
            x: Math.min(active.startX, p1.x),
            y: Math.min(active.startY, p1.y),
            w: Math.abs(p1.x - active.startX),
            h: Math.abs(p1.y - active.startY),
          });
        };

        try { canvas.setPointerCapture(e.pointerId); } catch {}
        document.addEventListener('pointermove', onMove, true);
        document.addEventListener('pointerup', onUp, true);
        document.addEventListener('pointercancel', onUp, true);
      }, true);
    }

    function bindGroupHitTestOnCanvas() {
      const host = document.getElementById('tabsContent');
      if (!host) return;
      if (host.dataset.groupHitBound) return;
      host.dataset.groupHitBound = '1';
      host.addEventListener('pointerdown', (e) => {
        if (!e || e.button !== 0) return;
        if (e.shiftKey && !e.ctrlKey && !e.metaKey) return;
        try { if (ctx.services.NSInteract.__isMobileLike && ctx.services.NSInteract.__isMobileLike()) return; } catch {}
        try { if (!document.body.classList.contains('edit-mode')) return; } catch {}
        const canvas = (e.target && e.target.closest) ? e.target.closest('.pixel-canvas') : null;
        if (!canvas) return;
        if ((e.target && e.target.closest) ? e.target.closest('.grid-stack-item') : null) return;
        try { if (e && e.cancelable) e.preventDefault(); } catch {}
        const p0 = ctx.services.NSCanvas.__clientToCanvasDesignXY(canvas, e.clientX, e.clientY);
        const px = Number(p0.x) || 0;
        const py = Number(p0.y) || 0;
        const nodes = Array.from(canvas.querySelectorAll('.grid-stack-item[data-group-id], .grid-stack-item[data-group-chain]'));
        const groups = new Map();
        nodes.forEach(n => {
          const chain = ctx.services.NSUtils.__getNodeGroupChain(n);
          if (!chain || !chain.length) return;
          const r = ctx.services.NSUtils.__readRectPx(n);
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
          if (!toggle) clearSelection();
          return;
        }
        const gs = ctx.state.selectedGroupIds;
        const wasGroupSelected = !!(gs && gs.has(hitGid));
        if (!wasGroupSelected) selectGroup(hitGid, { toggle });
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
          const gs2 = ctx.state.selectedGroupIds;
          const raw = (gs2 && gs2.size && gs2.has(hitGid)) ? Array.from(gs2) : [hitGid];
          const gids = ctx.services.NSUtils.__normalizeSelectedGroupIds(canvas, raw);
          const out = [];
          const seen = new Set();
          gids.forEach(g => {
            const g2 = String(g || '').trim();
            if (!g2) return;
            const members = ctx.services.NSUtils.__getGroupMembers(canvas, g2);
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
            if (targets && targets.length) ctx.services.NSInteract.__startDragNodes(canvas, targets, e);
          }
        };
        const onUp = () => { cleanup(); };
        try { canvas.setPointerCapture(e.pointerId); } catch {}
        canvas.addEventListener('pointermove', onMove);
        canvas.addEventListener('pointerup', onUp);
        canvas.addEventListener('pointercancel', onUp);
      }, true);
    }

    function __getActiveCanvasEl() {
      try {
        if (ctx.services.NS.activeTabId) {
          const w = document.querySelector('#tabsContent [data-tab-id="' + String(ctx.services.NS.activeTabId).replace(/"/g, '\\"') + '"]');
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
          const p = ctx.services.NSCanvas.__clientToCanvasDesignXY(canvas, e.clientX, e.clientY);
          const x = Number(p.x) || 0;
          const y = Number(p.y) || 0;
          ctx.state.lastMouseCanvasPos = { x, y };
        };
        host.addEventListener('pointermove', update, true);
        host.addEventListener('pointerdown', update, true);
      } catch {}
    }

    function __getSelectedEntities(canvas) {
      const nodes = Array.from(ctx.state.selectedNodes);
      const gs = ctx.state.selectedGroupIds;
      const gids = ctx.services.NSUtils.__normalizeSelectedGroupIds(canvas, gs ? Array.from(gs) : []);
      const independentNodes = nodes.filter(n => !gids.some(gid => ctx.services.NSUtils.__nodeInGroup(n, gid)));
      const entities = [];
      gids.forEach(gid => {
        const r = ctx.services.NSUtils.__getGroupRect(canvas, gid);
        if (r) entities.push({ type: 'group', id: gid, r });
      });
      independentNodes.forEach(n => {
        const r = ctx.services.NSUtils.__readRectPx(n);
        if (r) entities.push({ type: 'node', node: n, r });
      });
      return { gids, independentNodes, entities };
    }

    function alignSelected(mode) {
      if (!ctx.edit.getGlobalEditMode()) return;
      const grid = ctx.layout.getActiveGrid(); if (!grid) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { gids, independentNodes, entities } = __getSelectedEntities(canvas);
      if (entities.length < 2) return;
      let refEntity = null, refRect = null;
      let refGid = String(ctx.state.selectedGroupAnchor || '').trim();
      let refNode = ctx.state.anchorSelected;
      if (refNode && independentNodes.includes(refNode)) {
        refEntity = { type: 'node', node: refNode };
        refRect = ctx.services.NSUtils.__readRectPx(refNode);
      } else if (refGid && gids.includes(refGid)) {
        refEntity = { type: 'group', id: refGid };
        refRect = ctx.services.NSUtils.__getGroupRect(canvas, refGid);
      } else {
        refEntity = entities[0]; refRect = refEntity.r;
      }
      if (!refRect) return;
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
          ctx.services.NSUtils.__shiftGroup(canvas, it.id, dx, dy);
        } else {
          ctx.services.NSUtils.__writeRectPx(it.node, { x: Math.max(0, x), y: Math.max(0, y), w: it.r.w, h: it.r.h });
        }
      });
      try { if (ctx.services.NS.activeTabId) ctx.layout.commitLayoutLocal(ctx.services.NS.activeTabId, grid); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function groupSelected() {
      if (!ctx.edit.getGlobalEditMode()) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { gids, independentNodes, entities } = __getSelectedEntities(canvas);
      if (entities.length < 2) return;
      const targets = []; const seen = new Set();
      gids.forEach(gid => {
        const members = ctx.services.NSUtils.__getGroupMembers(canvas, gid);
        members.forEach(n => { if (n && !seen.has(n)) { seen.add(n); targets.push(n); } });
      });
      independentNodes.forEach(n => { if (n && !seen.has(n)) { seen.add(n); targets.push(n); } });
      if (targets.length < 2) return;
      const newGid = 'g_' + Math.random().toString(36).slice(2, 8) + Date.now().toString(36).slice(-4);
      let common = ctx.services.NSUtils.__getNodeGroupChain(targets[0]);
      targets.slice(1).forEach(n => {
        const c = ctx.services.NSUtils.__getNodeGroupChain(n);
        let i = 0;
        while (i < common.length && i < c.length && String(common[i]) === String(c[i])) i++;
        common = common.slice(0, i);
      });
      targets.forEach(n => {
        const chain = ctx.services.NSUtils.__getNodeGroupChain(n);
        const c2 = chain.slice(common.length);
        const next = common.concat([newGid], c2);
        ctx.services.NSUtils.__setNodeGroupChain(n, next);
      });
      try { clearSelection(); selectGroup(newGid); } catch {}
      try { ctx.props.updatePropPanel(ctx.state.currentSelected); } catch {}
      try { if (ctx.services.NS.activeTabId) ctx.layout.commitLayoutLocal(ctx.services.NS.activeTabId, ctx.layout.getActiveGrid()); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function ungroupSelected() {
      if (!ctx.edit.getGlobalEditMode()) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { gids, independentNodes, entities } = __getSelectedEntities(canvas);
      if (entities.length < 1) return;
      gids.forEach(gid => {
        const g = String(gid || '').trim(); if (!g) return;
        const members = ctx.services.NSUtils.__getGroupMembers(canvas, g);
        members.forEach(n => {
          const chain = ctx.services.NSUtils.__getNodeGroupChain(n);
          const idx = chain.indexOf(g);
          if (idx >= 0) chain.splice(idx, 1);
          ctx.services.NSUtils.__setNodeGroupChain(n, chain);
        });
      });
      independentNodes.forEach(n => {
        const chain = ctx.services.NSUtils.__getNodeGroupChain(n);
        if (chain && chain.length) chain.pop();
        ctx.services.NSUtils.__setNodeGroupChain(n, chain);
      });
      try { clearSelection(); } catch {}
      updateAlignHint();
      ctx.props.updatePropPanel(null);
      try { if (ctx.services.NS.activeTabId) ctx.layout.commitLayoutLocal(ctx.services.NS.activeTabId, ctx.layout.getActiveGrid()); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function distributeSelected(axis) {
      if (!ctx.edit.getGlobalEditMode()) return;
      const grid = ctx.layout.getActiveGrid(); if (!grid) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { entities } = __getSelectedEntities(canvas);
      if (entities.length < 3) return;
      if (axis === 'h') {
        entities.sort((a, b) => (a.r.x - b.r.x) || (a.r.y - b.r.y));
        const minX = Math.min.apply(null, entities.map(it => it.r.x));
        const maxR = Math.max.apply(null, entities.map(it => it.r.x + it.r.w));
        const sumW = entities.reduce((s, it) => s + it.r.w, 0);
        const gap = (maxR - minX - sumW) / (entities.length - 1);
        let cursor = minX;
        entities.forEach(it => {
          if (it.type === 'group') { const dx = cursor - it.r.x; ctx.services.NSUtils.__shiftGroup(canvas, it.id, dx, 0); }
          else { ctx.services.NSUtils.__writeRectPx(it.node, { x: cursor, y: it.r.y, w: it.r.w, h: it.r.h }); }
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
          if (it.type === 'group') { const dy = cursor - it.r.y; ctx.services.NSUtils.__shiftGroup(canvas, it.id, 0, dy); }
          else { ctx.services.NSUtils.__writeRectPx(it.node, { x: it.r.x, y: cursor, w: it.r.w, h: it.r.h }); }
          cursor = cursor + it.r.h + gap;
        });
      }
      try { if (ctx.services.NS.activeTabId) ctx.layout.commitLayoutLocal(ctx.services.NS.activeTabId, grid); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function sameSizeSelected() {
      if (!ctx.edit.getGlobalEditMode()) return;
      const grid = ctx.layout.getActiveGrid(); if (!grid) return;
      const canvas = __getActiveCanvasEl(); if (!canvas) return;
      const { gids, independentNodes, entities } = __getSelectedEntities(canvas);
      if (entities.length < 2) return;
      let refEntity = null, refRect = null;
      let refGid = String(ctx.state.selectedGroupAnchor || '').trim();
      let refNode = ctx.state.anchorSelected;
      if (refNode && independentNodes.includes(refNode)) {
        refEntity = { type: 'node', node: refNode }; refRect = ctx.services.NSUtils.__readRectPx(refNode);
      } else if (refGid && gids.includes(refGid)) {
        refEntity = { type: 'group', id: refGid }; refRect = ctx.services.NSUtils.__getGroupRect(canvas, refGid);
      } else {
        refEntity = entities[0]; refRect = refEntity.r;
      }
      if (!refRect) return;
      const targetW = Math.max(10, refRect.w); const targetH = Math.max(10, refRect.h);
      entities.forEach(it => {
        if (it.type === 'group' && it.id === refEntity.id) return;
        if (it.type === 'node' && it.node === refEntity.node) return;
        if (it.type === 'group') { ctx.services.NSUtils.__resizeGroupTo(canvas, it.id, targetW, targetH); }
        else {
          ctx.services.NSUtils.__writeRectPx(it.node, { x: Math.max(0, it.r.x), y: Math.max(0, it.r.y), w: targetW, h: targetH });
        }
      });
      try { if (ctx.services.NS.activeTabId) ctx.layout.commitLayoutLocal(ctx.services.NS.activeTabId, grid); } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }

    function bindAlignActions() {
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
      safe('undoBtn', () => ctx.history.undoLayout());
      safe('redoBtn', () => ctx.history.redoLayout());
    }

    function getEditSelectionState() {
      const canvas = __getActiveCanvasEl();
      if (!canvas) return { entityCount: 0, nodeCount: 0, groupCount: 0 };
      const { entities, gids } = __getSelectedEntities(canvas);
      return {
        entityCount: entities.length,
        nodeCount: ctx.state.selectedNodes.size,
        groupCount: gids.length,
      };
    }

    function deleteSelected() {
      if (!ctx.edit.getGlobalEditMode()) return;
      const canvas = __getActiveCanvasEl();
      if (!canvas) return;
      let targets = Array.from(canvas.querySelectorAll('.grid-stack-item.grid-selected'));
      if (!targets.length && ctx.state.selectedGroupIds && ctx.state.selectedGroupIds.size) {
        const seen = new Set();
        targets = [];
        ctx.state.selectedGroupIds.forEach(gid => {
          ctx.services.NSUtils.__getGroupMembers(canvas, String(gid || '').trim()).forEach(n => {
            if (n && !seen.has(n)) { seen.add(n); targets.push(n); }
          });
        });
      }
      if (!targets.length) return;
      targets.forEach(n => {
        try { if (window.EPWidgets && typeof EPWidgets.unindexNode === 'function') EPWidgets.unindexNode(n); } catch {}
        try {
          const c = n && n.closest ? n.closest('.pixel-canvas') : null;
          if (c && window.NSCanvas && typeof NSCanvas.scheduleFitCanvasToWidgets === 'function') {
            let tid = null;
            ctx.services.NS.grids.forEach((info, key) => { if (info && info.canvasEl === c) tid = key; });
            if (tid) NSCanvas.scheduleFitCanvasToWidgets(tid);
          }
        } catch {}
        try { if (n && n.parentElement) n.parentElement.removeChild(n); } catch {}
      });
      clearSelection();
      try {
        if (ctx.services.NS.activeTabId) ctx.layout.commitLayoutLocal(ctx.services.NS.activeTabId, ctx.layout.getActiveGrid());
      } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
    }


    return {
      updateAlignHint,
      clearSelection,
      removeFromSelection,
      selectNode,
      selectNodes,
      selectGroup,
      bindMarqueeSelection,
      bindGroupHitTestOnCanvas,
      __getActiveCanvasEl,
      bindMousePosTracking,
      alignSelected,
      groupSelected,
      ungroupSelected,
      distributeSelected,
      sameSizeSelected,
      deleteSelected,
      getEditSelectionState,
      bindAlignActions
    };
  }

  window.NSDashboardSelection = { init };
})();
