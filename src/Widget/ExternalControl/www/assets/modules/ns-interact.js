// ns-interact.js —— 交互/快捷键/分组指示器/拖拽/缩放/输入交互/布局IO
(function() {
  'use strict';
  const NS = window.NS;
  const NSCanvas = window.NSCanvas;
  const NSWsSync = window.NSWsSync;
  const NSUtils = window.NSUtils;

  // ===== 工具函数 =====

  // 函数级注释：判断是否为移动端设备
  function __isMobileLike() {
    try {
      const ua = String(navigator.userAgent || '').toLowerCase();
      const mobileUa = /iphone|ipad|ipod|android|mobile/.test(ua);
      const coarse = !!(window.matchMedia && window.matchMedia('(pointer: coarse)').matches);
      const narrow = !!(window.matchMedia && window.matchMedia('(max-width: 900px)').matches);
      return mobileUa || (coarse && narrow);
    } catch { return false; }
  }

  // 函数级注释：获取当前活跃的分页容器
  function __getActiveWrap() {
    return Array.from(document.querySelectorAll('#tabsContent [data-tab-id]')).find(el => el.style.display !== 'none') || null;
  }

  // 函数级注释：获取当前活跃分页中被选中的控件节点
  function __getSelectedInActiveWrap() {
    const wrap = __getActiveWrap();
    if (!wrap) return [];
    return Array.from(wrap.querySelectorAll('.grid-stack-item.grid-selected'));
  }

  // ===== 分组指示器 =====

  // 函数级注释：调度刷新当前页的分组虚线框（避免频繁 DOM 重排）
  let __groupIndicatorRaf = 0;
  function __scheduleGroupIndicatorsUpdate() {
    if (__groupIndicatorRaf) return;
    __groupIndicatorRaf = requestAnimationFrame(() => {
      __groupIndicatorRaf = 0;
      __updateGroupIndicators();
    });
  }

  // 函数级注释：根据当前页控件的 groupId 计算包围盒并绘制整体虚线框（仅像素画布 + 编辑模式显示）
  function __updateGroupIndicators() {
    const wrap = __getActiveWrap();
    if (!wrap) return;
    const canvas = wrap.querySelector('.pixel-canvas');
    if (!canvas) return;

    const isEdit = document.body.classList.contains('edit-mode');
    const nodes = Array.from(canvas.querySelectorAll('.grid-stack-item[data-group-id], .grid-stack-item[data-group-chain]'));

    const existing = new Map();
    Array.from(canvas.querySelectorAll('.group-indicator[data-group-id]')).forEach(el => {
      existing.set(String(el.dataset.groupId || ''), el);
    });

    if (!isEdit || nodes.length === 0) {
      existing.forEach(el => { try { el.remove(); } catch {} });
      return;
    }

    const meta = NSUtils.__buildGroupMeta(canvas);
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

    const padding = 8;
    groups.forEach((rects, gid) => {
      if (!rects || rects.length < 2) {
        const old = existing.get(gid);
        if (old) { try { old.remove(); } catch {} }
        existing.delete(gid);
        return;
      }

      let minX = Infinity, minY = Infinity, maxR = -Infinity, maxB = -Infinity;
      rects.forEach(r => {
        minX = Math.min(minX, r.x);
        minY = Math.min(minY, r.y);
        maxR = Math.max(maxR, r.x + r.w);
        maxB = Math.max(maxB, r.y + r.h);
      });

      const x = Math.max(0, Math.round(minX - padding));
      const y = Math.max(0, Math.round(minY - padding));
      const w = Math.max(10, Math.round((maxR - minX) + padding * 2));
      const h = Math.max(10, Math.round((maxB - minY) + padding * 2));

      let box = existing.get(gid);
      if (!box) {
        box = document.createElement('div');
        box.className = 'group-indicator';
        box.dataset.groupId = gid;
        try { canvas.insertBefore(box, canvas.firstChild); } catch { try { canvas.appendChild(box); } catch {} }
      }

      try {
        const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.has === 'function') ? window.__selectedGroupIds : null;
        const isSel = !!(gs && gs.has(gid));
        box.classList.toggle('group-selected', !!isSel);
      } catch {}

      box.style.left = x + 'px';
      box.style.top = y + 'px';
      box.style.width = w + 'px';
      box.style.height = h + 'px';
      try {
        const depth = meta && meta.depthOf && meta.depthOf.has(gid) ? Number(meta.depthOf.get(gid)) : 0;
        box.style.zIndex = String(1 + Math.max(0, depth));
      } catch {}
      existing.delete(gid);
    });

    existing.forEach(el => { try { el.remove(); } catch {} });
  }

  // ===== 拖拽/缩放 =====

  // 函数级注释：确保遮罩层上存在四个方向的缩放手柄
  function __ensureResizeHandles(overlay) {
    if (!overlay) return;
    if (overlay.querySelector('.px-resize-handle')) return;
    ['nw','ne','sw','se'].forEach(dir => {
      const h = document.createElement('div');
      h.className = 'px-resize-handle';
      h.dataset.dir = dir;
      overlay.appendChild(h);
    });
  }

  // 函数级注释：拖拽指定节点集合（用于多选控件拖拽、整组拖拽等）
  function __startDragNodes(captureEl, targets, ev) {
    if (!captureEl || !targets || !targets.length || !ev) return;

    const startX = Number(ev.clientX) || 0;
    const startY = Number(ev.clientY) || 0;
    const startRects = new Map();
    targets.forEach(n => { try { startRects.set(n, NSUtils.__readRectPx(n)); } catch {} });

    try { captureEl.setPointerCapture(ev.pointerId); } catch {}

    const onMove = (e) => {
      try { if (e && e.cancelable) e.preventDefault(); } catch {}
      const dx0 = (Number(e.clientX) || 0) - startX;
      const dy0 = (Number(e.clientY) || 0) - startY;
      const step = e && e.shiftKey ? 1 : 5;
      const snap = !(e && e.altKey);

      const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
      const boundW = info && info.design ? Number(info.design.width) : NaN;
      const boundH = info && info.design ? Number(info.design.height) : NaN;

      const dx = snap ? Math.round(dx0 / step) * step : dx0;
      const dy = snap ? Math.round(dy0 / step) * step : dy0;

      startRects.forEach((r, n) => {
        let x = r.x + dx;
        let y = r.y + dy;
        if (Number.isFinite(boundW)) x = Math.max(0, Math.min(boundW - r.w, x));
        if (Number.isFinite(boundH)) y = Math.max(0, Math.min(boundH - r.h, y));
        NSUtils.__writeRectPx(n, { x, y, w: r.w, h: r.h });
      });
      try { __scheduleGroupIndicatorsUpdate(); } catch {}
    };

    const onUp = (e) => {
      try { captureEl.releasePointerCapture(e.pointerId); } catch {}
      captureEl.removeEventListener('pointermove', onMove);
      captureEl.removeEventListener('pointerup', onUp);
      captureEl.removeEventListener('pointercancel', onUp);
      try {
        if (NS.activeTabId && typeof window.__saveLayoutLocal === 'function') {
          const info = NS.grids.get(NS.activeTabId);
          if (info && info.grid) window.__saveLayoutLocal(NS.activeTabId, info.grid);
        }
      } catch {}
    };

    captureEl.addEventListener('pointermove', onMove);
    captureEl.addEventListener('pointerup', onUp);
    captureEl.addEventListener('pointercancel', onUp);
  }

  // 函数级注释：从当前选择或单个节点开始拖拽（不自动推断"整组拖拽"，由上层交互决定）
  function __startDrag(overlay, node, ev) {
    if (!overlay || !node || !ev) return;
    const selected = __getSelectedInActiveWrap();
    const targets = (selected && selected.length) ? selected : [node];
    __startDragNodes(overlay, targets, ev);
  }

  // 函数级注释：开始缩放操作
  function __startResize(overlay, node, dir, ev) {
    if (!overlay || !node || !ev) return;
    const startX = Number(ev.clientX) || 0;
    const startY = Number(ev.clientY) || 0;
    const r0 = NSUtils.__readRectPx(node);

    try { overlay.setPointerCapture(ev.pointerId); } catch {}

    const onMove = (e) => {
      try { if (e && e.cancelable) e.preventDefault(); } catch {}
      const dx0 = (Number(e.clientX) || 0) - startX;
      const dy0 = (Number(e.clientY) || 0) - startY;
      const step = e && e.shiftKey ? 1 : 5;
      const snap = !(e && e.altKey);

      const minW = 40;
      const minH = 30;

      const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
      const boundW = info && info.design ? Number(info.design.width) : NaN;
      const boundH = info && info.design ? Number(info.design.height) : NaN;

      const dx = snap ? Math.round(dx0 / step) * step : dx0;
      const dy = snap ? Math.round(dy0 / step) * step : dy0;

      const right0 = r0.x + r0.w;
      const bottom0 = r0.y + r0.h;

      let x = r0.x, y = r0.y, w = r0.w, h = r0.h;
      if (dir === 'se') {
        w = r0.w + dx;
        h = r0.h + dy;
        w = Math.max(minW, w);
        h = Math.max(minH, h);
        if (Number.isFinite(boundW)) w = Math.min(w, boundW - x);
        if (Number.isFinite(boundH)) h = Math.min(h, boundH - y);
      } else if (dir === 'sw') {
        x = r0.x + dx;
        h = r0.h + dy;
        x = Math.max(0, x);
        h = Math.max(minH, h);
        w = right0 - x;
        if (w < minW) { x = right0 - minW; w = minW; }
        if (Number.isFinite(boundH)) h = Math.min(h, boundH - y);
      } else if (dir === 'ne') {
        y = r0.y + dy;
        w = r0.w + dx;
        y = Math.max(0, y);
        w = Math.max(minW, w);
        h = bottom0 - y;
        if (h < minH) { y = bottom0 - minH; h = minH; }
        if (Number.isFinite(boundW)) w = Math.min(w, boundW - x);
      } else if (dir === 'nw') {
        x = r0.x + dx;
        y = r0.y + dy;
        x = Math.max(0, x);
        y = Math.max(0, y);
        w = right0 - x;
        h = bottom0 - y;
        if (w < minW) { x = right0 - minW; w = minW; }
        if (h < minH) { y = bottom0 - minH; h = minH; }
      }

      NSUtils.__writeRectPx(node, { x, y, w, h });
      try { __scheduleGroupIndicatorsUpdate(); } catch {}
    };

    const onUp = (e) => {
      try { overlay.releasePointerCapture(e.pointerId); } catch {}
      overlay.removeEventListener('pointermove', onMove);
      overlay.removeEventListener('pointerup', onUp);
      overlay.removeEventListener('pointercancel', onUp);
      try {
        if (NS.activeTabId && typeof window.__saveLayoutLocal === 'function') {
          const info = NS.grids.get(NS.activeTabId);
          if (info && info.grid) window.__saveLayoutLocal(NS.activeTabId, info.grid);
        }
      } catch {}
    };

    overlay.addEventListener('pointermove', onMove);
    overlay.addEventListener('pointerup', onUp);
    overlay.addEventListener('pointercancel', onUp);
  }

  // ===== 输入交互模式 =====

  // 函数级注释：判断节点是否为可输入控件（输入框/含 input/textarea/contenteditable）
  function __isTextInputNode(node) {
    try {
      const t = String((node && node.dataset ? node.dataset.type : '') || '').trim();
      if (t === '输入框') return true;
      return !!(node && node.querySelector && node.querySelector('input,textarea,[contenteditable="true"]'));
    } catch { return false; }
  }

  // 函数级注释：查找节点内部可聚焦的输入元素
  function __findTextInputEl(node) {
    try {
      return (node && node.querySelector) ? node.querySelector('input,textarea,[contenteditable="true"]') : null;
    } catch { return null; }
  }

  // 函数级注释：进入/退出输入交互模式（编辑模式下让点击穿透遮罩层）
  function __setNodeInteract(node, on) {
    try {
      const prev = window.__interactNode;
      if (prev && prev !== node) {
        try { prev.classList.remove('ns-widget-interact'); } catch {}
      }
      if (!node) {
        if (prev) { try { prev.classList.remove('ns-widget-interact'); } catch {} }
        window.__interactNode = null;
        return;
      }
      if (on) {
        try { node.classList.add('ns-widget-interact'); } catch {}
        window.__interactNode = node;
      } else {
        try { node.classList.remove('ns-widget-interact'); } catch {}
        if (window.__interactNode === node) window.__interactNode = null;
      }
    } catch {}
  }

  // 函数级注释：聚焦输入控件内部的输入元素
  function __focusNodeTextInput(node) {
    const el = __findTextInputEl(node);
    if (!el) return;
    try { el.focus({ preventScroll: true }); } catch { try { el.focus(); } catch {} }
    try { if (typeof el.select === 'function') el.select(); } catch {}
  }

  // 函数级注释：在满足条件时进入输入交互模式并聚焦（避免 Ctrl/Shift 多选时误触）
  function __enterInputInteractIfNeeded(node, opts) {
    try { if (document.body.classList.contains('edit-mode')) return; } catch {}
    const toggle = !!(opts && opts.toggle);
    if (toggle) return;
    if (!__isTextInputNode(node)) return;
    try {
      const NSDashboard = window.NSDashboard;
      const sn = NSDashboard && NSDashboard._getSelectedNodes ? NSDashboard._getSelectedNodes() : null;
      if (sn && sn.size > 1) return;
    } catch {}
    __setNodeInteract(node, true);
    setTimeout(() => __focusNodeTextInput(node), 0);
  }

  // 函数级注释：编辑模式下为输入类控件提供"点击聚焦/拖拽移动"兼容行为
  function __startDragOrEnterInput(overlay, node, e, opts) {
    const toggle = !!(opts && opts.toggle);
    const handle = (e.target && e.target.closest) ? e.target.closest('.px-resize-handle') : null;
    if (handle && handle.dataset && handle.dataset.dir) {
      __setNodeInteract(node, false);
      __startResize(overlay, node, String(handle.dataset.dir), e);
      return;
    }

    if (!__isTextInputNode(node) || toggle) {
      __setNodeInteract(node, false);
      __startDrag(overlay, node, e);
      return;
    }

    __setNodeInteract(node, false);
    let moved = false;
    const sx = Number(e.clientX) || 0;
    const sy = Number(e.clientY) || 0;

    const cleanup = () => {
      overlay.removeEventListener('pointermove', onMove);
      overlay.removeEventListener('pointerup', onUp);
      overlay.removeEventListener('pointercancel', onUp);
      try { overlay.releasePointerCapture(e.pointerId); } catch {}
    };

    const onMove = (ev) => {
      const dx = (Number(ev.clientX) || 0) - sx;
      const dy = (Number(ev.clientY) || 0) - sy;
      if (!moved && (Math.abs(dx) > 3 || Math.abs(dy) > 3)) {
        moved = true;
        cleanup();
        __startDrag(overlay, node, e);
      }
    };

    const onUp = () => {
      cleanup();
      if (!moved) __enterInputInteractIfNeeded(node, { toggle: false });
    };

    try { overlay.setPointerCapture(e.pointerId); } catch {}
    overlay.addEventListener('pointermove', onMove);
    overlay.addEventListener('pointerup', onUp);
    overlay.addEventListener('pointercancel', onUp);
  }

  // 函数级注释：绑定一次全局退出交互模式事件（Esc 或点击控件外部）
  (function bindInteractExitHandlers(){
    if (window.__interactExitBound) return;
    window.__interactExitBound = true;

    document.addEventListener('pointerdown', (e) => {
      try {
        const n = window.__interactNode;
        if (!n) return;
        if (n.contains(e.target)) return;
        __setNodeInteract(n, false);
      } catch {}
    }, true);

    document.addEventListener('keydown', (e) => {
      try {
        if (!e || e.key !== 'Escape') return;
        const n = window.__interactNode;
        if (!n) return;
        const ae = document.activeElement;
        if (ae && n.contains(ae)) {
          __setNodeInteract(n, false);
          try { e.preventDefault(); } catch {}
          try { e.stopPropagation(); } catch {}
        }
      } catch {}
    }, true);
  })();

  // ===== Overlay 绑定 =====

  // 函数级注释：绑定 overlay 拖拽/缩放以触发选中与编辑（全局函数，供加载与添加时复用）
  function attachOverlay(node) {
    const overlay = node.querySelector('.widget-overlay-mask');
    if (!overlay) return;

    __ensureResizeHandles(overlay);

    overlay.addEventListener('pointerdown', (e) => {
      try { if (e && e.cancelable) e.preventDefault(); } catch {}
      try { e.stopPropagation(); } catch {}

      if (__isMobileLike()) return;
      if (!document.body.classList.contains('edit-mode')) return;
      if (!node.closest || !node.closest('.pixel-canvas')) return;

      const toggle = !!(e && (e.ctrlKey || e.metaKey || e.shiftKey));
      const chain = NSUtils.__getNodeGroupChain(node);
      const leafGid = (chain && chain.length) ? String(chain[chain.length - 1] || '').trim() : '';
      let gid = leafGid;
      try {
        const gs0 = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.has === 'function') ? window.__selectedGroupIds : null;
        if (gs0 && gs0.size && chain && chain.length) {
          for (let i = chain.length - 1; i >= 0; i--) {
            const g = String(chain[i] || '').trim();
            if (g && gs0.has(g)) { gid = g; break; }
          }
        }
      } catch {}

      if (gid) {
        const nodeIsSelected = !!(node && node.classList && node.classList.contains('grid-selected'));
        const drill = String((window && window.__groupDrillId) || '').trim();
        const inDrill = !!(drill && drill === gid);
        const NSDashboard = window.NSDashboard;
        const sn = NSDashboard && NSDashboard._getSelectedNodes ? NSDashboard._getSelectedNodes() : null;
        const hasNodeSelection = !!(sn && sn.size > 0);

        // 处于组内钻取/控件选择模式时：点击组内控件直接走控件选择（支持 Ctrl/Shift 多选）
        if (nodeIsSelected || inDrill || hasNodeSelection) {
          if (typeof window.__selectNode === 'function') window.__selectNode(node, { toggle });
          __startDragOrEnterInput(overlay, node, e, { toggle });
          return;
        }

        const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.has === 'function') ? window.__selectedGroupIds : null;
        const wasGroupSelected = !!(gs && gs.has(gid));

        // 第一次点击：只选中分组，不选中组内控件
        if (typeof window.__selectGroup === 'function') {
          if (!wasGroupSelected) window.__selectGroup(gid, { toggle });
        }

        let moved = false;
        const sx = Number(e.clientX) || 0;
        const sy = Number(e.clientY) || 0;

        const cleanup = () => {
          overlay.removeEventListener('pointermove', onMove);
          overlay.removeEventListener('pointerup', onUp);
          overlay.removeEventListener('pointercancel', onUp);
          try { overlay.releasePointerCapture(e.pointerId); } catch {}
        };

        const collectGroupTargets = () => {
          const wrap = __getActiveWrap();
          if (!wrap) return [node];
          const canvas = wrap.querySelector('.pixel-canvas');
          if (!canvas) return [node];

          const gs2 = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.has === 'function') ? window.__selectedGroupIds : null;
          const raw = (gs2 && gs2.size && gs2.has(gid)) ? Array.from(gs2) : [gid];
          const gids = NSUtils.__normalizeSelectedGroupIds(canvas, raw);

          const out = [];
          const seen = new Set();
          gids.forEach(g => {
            const g2 = String(g || '').trim();
            if (!g2) return;
            const members = NSUtils.__getGroupMembers(canvas, g2);
            members.forEach(n => {
              if (!n) return;
              if (seen.has(n)) return;
              seen.add(n);
              out.push(n);
            });
          });
          return out.length ? out : [node];
        };

        const onMove = (ev) => {
          const dx = (Number(ev.clientX) || 0) - sx;
          const dy = (Number(ev.clientY) || 0) - sy;
          if (!moved && (Math.abs(dx) > 3 || Math.abs(dy) > 3)) {
            moved = true;
            cleanup();
            __startDragNodes(overlay, collectGroupTargets(), e);
          }
        };

        const onUp = () => {
          cleanup();
          // 分组已选中时的再次点击：进入控件选择模式（支持 Ctrl/Shift 多选）
          if (!moved && wasGroupSelected) {
            if (typeof window.__selectNode === 'function') window.__selectNode(node, { toggle });
            __enterInputInteractIfNeeded(node, { toggle });
          }
        };

        try { overlay.setPointerCapture(e.pointerId); } catch {}
        overlay.addEventListener('pointermove', onMove);
        overlay.addEventListener('pointerup', onUp);
        overlay.addEventListener('pointercancel', onUp);
        return;
      }

      if (typeof window.__selectNode === 'function') window.__selectNode(node, { toggle });
      __startDragOrEnterInput(overlay, node, e, { toggle });
    });
  }

  // ===== 键盘快捷键 =====

  // 函数级注释：键盘编辑支持（删除、多选移动；仅编辑模式；像素画布优先）
  window.addEventListener('keydown', function(e){
    try {
      if (e && e.defaultPrevented) return;
      const tag = (e.target && e.target.tagName) || '';
      const isInput = ['INPUT','TEXTAREA','SELECT'].includes(tag);
      const inProp = (function(){ try { const p = document.getElementById('propPanel'); return !!(p && p.contains(e.target)); } catch { return false; } })();
      const isEdit = document.body.classList.contains('edit-mode');
      if (!isEdit) return;
      if (isInput || inProp) return;
      if (__isMobileLike()) return;

      const ctrl = !!(e && (e.ctrlKey || e.metaKey));
      if (ctrl) {
        const k = String(e.key || '').toLowerCase();
        const code = String(e.code || '');
        const isC = (k === 'c') || (code === 'KeyC');
        const isV = (k === 'v') || (code === 'KeyV');
        const isZ = (k === 'z') || (code === 'KeyZ');
        const isY = (k === 'y') || (code === 'KeyY');
        if (isC) {
          e.preventDefault();
          try { e.stopPropagation(); } catch {}
          try { e.stopImmediatePropagation(); } catch {}
          if (typeof window.__copySelection === 'function') window.__copySelection();
          return;
        }
        if (isV) {
          e.preventDefault();
          try { e.stopPropagation(); } catch {}
          try { e.stopImmediatePropagation(); } catch {}
          if (typeof window.__pasteSelection === 'function') window.__pasteSelection();
          return;
        }
        if (isZ) {
          e.preventDefault();
          try { e.stopPropagation(); } catch {}
          try { e.stopImmediatePropagation(); } catch {}
          if (window.NSDashboard && typeof window.NSDashboard.undoLayout === 'function') window.NSDashboard.undoLayout();
          return;
        }
        if (isY) {
          e.preventDefault();
          try { e.stopPropagation(); } catch {}
          try { e.stopImmediatePropagation(); } catch {}
          if (window.NSDashboard && typeof window.NSDashboard.redoLayout === 'function') window.NSDashboard.redoLayout();
          return;
        }
      }

      const wrap = __getActiveWrap();
      const canvas = wrap ? wrap.querySelector('.pixel-canvas') : null;
      const isPx = !!canvas;

      // 函数级注释：收集当前页被选中的分组成员节点（用于键盘移动/删除）
      function __collectSelectedGroupNodes(canvasEl) {
        const gs = (window && window.__selectedGroupIds && typeof window.__selectedGroupIds.has === 'function') ? window.__selectedGroupIds : null;
        if (!gs || !gs.size || !canvasEl) return [];
        const out = [];
        const seen = new Set();
        Array.from(gs).forEach(gid => {
          const g = String(gid || '').trim();
          if (!g) return;
          const members = NSUtils.__getGroupMembers(canvasEl, g);
          members.forEach(n => {
            if (!n) return;
            if (seen.has(n)) return;
            seen.add(n);
            out.push(n);
          });
        });
        return out;
      }

      if (isPx && (e.key === 'ArrowLeft' || e.key === 'ArrowRight' || e.key === 'ArrowUp' || e.key === 'ArrowDown')) {
        const nodes = __getSelectedInActiveWrap();
        const gNodes = (!nodes || nodes.length === 0) ? __collectSelectedGroupNodes(canvas) : [];
        const targets = (nodes && nodes.length) ? nodes : gNodes;
        if (!targets || targets.length === 0) return;
        e.preventDefault();

        const step = e.shiftKey ? 10 : 1;
        const dx = (e.key === 'ArrowLeft') ? -step : (e.key === 'ArrowRight') ? step : 0;
        const dy = (e.key === 'ArrowUp') ? -step : (e.key === 'ArrowDown') ? step : 0;

        const info = NS.activeTabId ? NS.grids.get(NS.activeTabId) : null;
        const boundW = info && info.design ? Number(info.design.width) : NaN;
        const boundH = info && info.design ? Number(info.design.height) : NaN;

        targets.forEach(n => {
          const r = NSUtils.__readRectPx(n);
          let x = r.x + dx;
          let y = r.y + dy;
          if (Number.isFinite(boundW)) x = Math.max(0, Math.min(boundW - r.w, x));
          if (Number.isFinite(boundH)) y = Math.max(0, Math.min(boundH - r.h, y));
          NSUtils.__writeRectPx(n, { x, y, w: r.w, h: r.h });
        });

        try { __scheduleGroupIndicatorsUpdate(); } catch {}
        try {
          if (NS.activeTabId && typeof window.__saveLayoutLocal === 'function') {
            const info2 = NS.grids.get(NS.activeTabId);
            if (info2 && info2.grid) window.__saveLayoutLocal(NS.activeTabId, info2.grid);
          }
        } catch {}
        return;
      }

      if ((e.key === 'Delete' || e.key === 'Backspace') && typeof window.__clearSelection === 'function') {
        const activeWrap = Array.from(document.querySelectorAll('#tabsContent [data-tab-id]')).find(el => el.style.display !== 'none');
        const selectedList = activeWrap ? Array.from(activeWrap.querySelectorAll('.grid-stack-item.grid-selected')) : [];
        const groupList = (!selectedList || selectedList.length === 0) ? __collectSelectedGroupNodes(canvas) : [];
        const targets = (selectedList && selectedList.length) ? selectedList : groupList;
        if (targets && targets.length) {
          e.preventDefault();
          targets.forEach(n => { try { if (n && n.parentElement) n.parentElement.removeChild(n); } catch {} });
          try { window.__clearSelection(); } catch {}
          const NSDashboard = window.NSDashboard;
          if (NSDashboard && typeof NSDashboard.updatePropPanel === 'function') NSDashboard.updatePropPanel(null);
          try {
            if (NS.activeTabId && typeof window.__saveLayoutLocal === 'function') {
              const info3 = NS.grids.get(NS.activeTabId);
              if (info3 && info3.grid) window.__saveLayoutLocal(NS.activeTabId, info3.grid);
            }
          } catch {}
          try { __scheduleGroupIndicatorsUpdate(); } catch {}
        }
      }
    } catch {}
  });

  // ===== 布局 IO =====

  // 函数级注释：从后端恢复所有分页布局（仅支持新格式；带渲染序号防重复）
  function loadLayout(grid, silent = false) {
    fetch('/api/layout/load')
      .then(r => r.json())
      .then(data => {
        let items = [];
        // 仅支持新格式（多页）
        if (data && data.pages && typeof data.pages === 'object' && Array.isArray(data.tabs)) {
           // 查找当前 grid 对应的 tid
           let tid = null;
           NS.grids.forEach((info, key) => { if (info.grid === grid) tid = key; });
           if (tid && data.pages[tid]) {
               items = data.pages[tid].items || [];
           }

           if (tid) {
             NSCanvas.clearLayoutContainer(grid);
             const seq = (window && typeof window.__beginTabRender === 'function') ? window.__beginTabRender(tid) : 0;
             NSUtils.ensureWidgetTypesReady((items || []).map(s => s && s.type), () => {
               if (seq && window && typeof window.__isTabRenderCurrent === 'function' && !window.__isTabRenderCurrent(tid, seq)) return;
               items.forEach(spec => { NSUtils.createWidgetFromSpec(grid, spec); });
               try { NSWsSync.queryAllStatuses(); } catch {}
             });
           } else {
             NSCanvas.clearLayoutContainer(grid);
           }
        } else {
          NSCanvas.clearLayoutContainer(grid);
        }

        try {
          NS.grids.forEach(info => { if (info.grid === grid) info.loaded = true; });
        } catch {}
        if (items.length > 0) {
          if (!silent) alert('布局加载成功');
        } else {
          console.log('当前没有保存的布局，显示空页面');
        }
      })
      .catch(e => {
          // 失败则尝试从本地存储按页面加载
          try {
            if (NS.activeTabId) {
              const raw = localStorage.getItem('ns_layout_' + NS.activeTabId);
              if (raw) {
                const data = JSON.parse(raw);
                const items = data.items || [];
                if (items.length > 0) {
                  NSCanvas.clearLayoutContainer(grid);
                  const seq = (window && typeof window.__beginTabRender === 'function') ? window.__beginTabRender(NS.activeTabId) : 0;
                  NSUtils.ensureWidgetTypesReady((items || []).map(s => s && s.type), () => {
                    if (seq && window && typeof window.__isTabRenderCurrent === 'function' && !window.__isTabRenderCurrent(NS.activeTabId, seq)) return;
                    items.forEach(spec => { NSUtils.createWidgetFromSpec(grid, spec); });
                    try { NSWsSync.queryAllStatuses(); } catch {}
                  });
                  if (!silent) alert('离线布局加载成功');
                  return;
                }
              }
            }
          } catch(err) {
            console.error('Local load failed:', err);
          }
          if (!silent) alert('加载失败: ' + e);
          else console.error('Auto-load failed:', e);
      });
  }

  // 函数级注释：加载所有分页布局（若后端返回新格式则重建分页与内容）
  function loadAllLayouts() {
    fetch('/api/layout/load')
      .then(r => r.json())
      .then(data => {
        const hasPages = data && data.pages && typeof data.pages === 'object';
        const hasTabs = Array.isArray(data.tabs);

        // 函数级注释：清空前端所有分页/本地缓存布局，并回到一个空白默认页（用于服务端布局被清空时同步到前端）
        function __resetToEmptyFromServer() {
          try {
            // 清空命令索引，防止残留控件继续被 queryAllStatuses 命中
            try { if (EPWidgets && typeof EPWidgets.clearCommandIndex === 'function') EPWidgets.clearCommandIndex(); } catch {}

            // 先清空所有现有页的内容（触发 unindex），再移除 DOM
            try {
              NS.grids.forEach(info => {
                try { if (info && info.grid) NSCanvas.clearLayoutContainer(info.grid); } catch {}
              });
            } catch {}

            // 移除现有分页 DOM
            try {
              NS.grids.forEach(info => {
                if (info && info.pageWrap && info.pageWrap.parentElement) {
                  info.pageWrap.parentElement.removeChild(info.pageWrap);
                }
                if (info && info.tabBtn && info.tabBtn.parentElement) {
                  info.tabBtn.parentElement.removeChild(info.tabBtn);
                }
              });
            } catch {}

            // 清空内存状态
            NS.tabs = [];
            NS.activeTabId = null;
            NS.grids.clear();

            // 清空容器
            try {
              const tabsList = document.getElementById('tabsList');
              const tabsContent = document.getElementById('tabsContent');
              if (tabsList) tabsList.innerHTML = '';
              if (tabsContent) tabsContent.innerHTML = '';
            } catch {}

            // 清空与布局相关的本地缓存（避免刷新后再次恢复旧布局）
            try {
              const keys = [];
              for (let i = 0; i < localStorage.length; i++) {
                const k = localStorage.key(i);
                if (k) keys.push(k);
              }
              keys.forEach(k => {
                if (k === 'ns_tabs' || k === 'ns_active_tab' || k === 'ns_clipboard') {
                  localStorage.removeItem(k);
                  return;
                }
                if (k.startsWith('ns_layout_') || k.startsWith('ns_hist_') || k.startsWith('ns_view_')) {
                  localStorage.removeItem(k);
                }
              });
            } catch {}

            // 重建一个空白默认页，并标记为已加载（避免切换时再次触发本地/服务端加载）
            let defaultId = null;
            try {
              if (window.__createTab) {
                defaultId = window.__createTab('页面1', null, { skipLocalLoad: true, skipSwitch: true });
              }
            } catch {}
            if (!defaultId) {
              // 兜底：若 dashboard 还没初始化完，至少保证不会抛错
              console.warn('createTab not ready, skipped rebuilding empty page');
              return;
            }

            try {
              const info = NS.grids.get(defaultId);
              if (info) info.loaded = true;
            } catch {}
            try { localStorage.setItem('ns_tabs', JSON.stringify([{ id: defaultId, name: '页面1' }])); } catch {}
            try { localStorage.setItem('ns_active_tab', defaultId); } catch {}
            try { if (window.__switchTab) window.__switchTab(defaultId); } catch {}
          } catch (e) {
            console.error('resetToEmptyFromServer failed', e);
          }
        }

        // 兼容旧格式：{"ok":true,"items":[...]}；当 items 为空时代表服务端已清空布局
        if (!hasPages || !hasTabs) {
          const ok = !!(data && data.ok);
          const hasItems = !!(data && Array.isArray(data.items));
          if (ok && hasItems) {
            const items = data.items || [];
            if (items.length === 0) {
              __resetToEmptyFromServer();
              return;
            }
            // 旧格式非空：只渲染到当前激活页
            try {
              const tid = NS.activeTabId;
              const info = tid ? NS.grids.get(tid) : null;
              if (info && info.grid) {
                NSCanvas.clearLayoutContainer(info.grid);
                const seq = (window && typeof window.__beginTabRender === 'function') ? window.__beginTabRender(tid) : 0;
                NSUtils.ensureWidgetTypesReady((items || []).map(s => s && s.type), () => {
                  if (seq && window && typeof window.__isTabRenderCurrent === 'function' && !window.__isTabRenderCurrent(tid, seq)) return;
                  items.forEach(spec => { NSUtils.createWidgetFromSpec(info.grid, spec); });
                  try { NSWsSync.queryAllStatuses(); } catch {}
                });
                try { info.loaded = true; } catch {}
              }
            } catch {}
            return;
          }
          return;
        }

        // 新格式但 tabs 为空：视为服务端已清空布局
        if (!data.tabs || data.tabs.length === 0) {
          __resetToEmptyFromServer();
          return;
        }

        if (hasPages && hasTabs) {
          // 清空现有分页
          try {
            NS.tabs = [];
            NS.grids.forEach(info => {
              if (info && info.pageWrap && info.pageWrap.parentElement) {
                info.pageWrap.parentElement.removeChild(info.pageWrap);
              }
            });
            NS.grids.clear();
            const tabsList = document.getElementById('tabsList');
            const tabsContent = document.getElementById('tabsContent');
            tabsList.innerHTML = '';
            tabsContent.innerHTML = '';
          } catch {}
          // 重建分页
          data.tabs.forEach(t => {
            if (window.__createTab) window.__createTab(t.name, t.id, { skipLocalLoad: true, skipSwitch: true });
          });
          // 渲染各页内容（带渲染序号防重复 & 渲染中禁止 switchTab 再触发加载）
          Object.keys(data.pages).forEach(tid => {
            const info = NS.grids.get(tid);
            if (!info) return;
            const page = data.pages[tid] || {};
            const items = page.items || [];
            if (page.design) NSCanvas.applyPageDesign(tid, page.design);
            NSCanvas.clearLayoutContainer(info.grid);
            try { info.rendering = true; } catch {}
            const seq = (window && typeof window.__beginTabRender === 'function') ? window.__beginTabRender(tid) : 0;
            NSUtils.ensureWidgetTypesReady((items || []).map(s => s && s.type), () => {
              if (seq && window && typeof window.__isTabRenderCurrent === 'function' && !window.__isTabRenderCurrent(tid, seq)) return;
              items.forEach(spec => { NSUtils.createWidgetFromSpec(info.grid, spec); });
              const info2 = NS.grids.get(tid);
              if (info2) { info2.loaded = true; info2.rendering = false; }
            });
          });
          const targetActive = data.activeTabId && NS.grids.has(data.activeTabId) ? data.activeTabId : (data.tabs[0] && data.tabs[0].id);
          if (targetActive) {
            if (window.__switchTab) window.__switchTab(targetActive);
          }
          
          // 布局加载完成后，延迟查询状态以确保控件已创建
          setTimeout(NSWsSync.queryAllStatuses, 500);
         
        }
      })
      .catch(e => alert('加载失败: ' + e));
  }

  // ===== 导出 =====
  window.NSInteract = {
    __isMobileLike,
    __getActiveWrap,
    __getSelectedInActiveWrap,
    __scheduleGroupIndicatorsUpdate,
    __updateGroupIndicators,
    __ensureResizeHandles,
    __startDragNodes,
    __startDrag,
    __startResize,
    __isTextInputNode,
    __findTextInputEl,
    __setNodeInteract,
    __focusNodeTextInput,
    __enterInputInteractIfNeeded,
    __startDragOrEnterInput,
    attachOverlay,
    loadLayout,
    loadAllLayouts
  };
})();
