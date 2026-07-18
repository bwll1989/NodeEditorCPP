// ns-canvas.js —— 画布缩放/平移/视图状态持久化/清空布局
(function() {
  'use strict';
  const NS = window.NS;

  const __fitCanvasTimers = new Map();

  function __canvasMinSize() {
    const defs = EPWidgets.layoutDefaults || {};
    return {
      w: Math.max(160, Math.floor(Number(defs.designWidthMin) || 320)),
      h: Math.max(120, Math.floor(Number(defs.designHeightMin) || 240)),
      pad: Math.max(0, Math.floor(Number(defs.canvasAutoPad) || 32)),
    };
  }

  function __defaultCanvasBg(info) {
    const raw = (info && info.design && info.design.bgColor !== undefined && info.design.bgColor !== null)
      ? String(info.design.bgColor)
      : String((EPWidgets.layoutDefaults && EPWidgets.layoutDefaults.canvasBgColor) || '#f8fafc');
    const bg = (raw || '').trim();
    return bg || String((EPWidgets.layoutDefaults && EPWidgets.layoutDefaults.canvasBgColor) || '#f8fafc');
  }

  // 函数级注释：根据控件包围盒计算画布尺寸（含 padding）
  function computeCanvasBoundsFromGrid(grid, pad) {
    const { w: minW, h: minH, pad: defaultPad } = __canvasMinSize();
    const padding = Math.max(0, Number(pad) || defaultPad);
    const el = grid && grid.el ? grid.el : null;
    if (!el) return { width: minW, height: minH };
    const items = el.querySelectorAll('.grid-stack-item');
    if (!items || items.length === 0) return { width: minW, height: minH };
    let maxR = 0;
    let maxB = 0;
    items.forEach(node => {
      try {
        const r = window.NSUtils && typeof NSUtils.__readRectPx === 'function' ? NSUtils.__readRectPx(node) : null;
        if (!r) return;
        maxR = Math.max(maxR, r.x + r.w);
        maxB = Math.max(maxB, r.y + r.h);
      } catch {}
    });
    return {
      width: Math.max(minW, Math.ceil(maxR + padding)),
      height: Math.max(minH, Math.ceil(maxB + padding)),
    };
  }

  function __clampCanvasTranslate(info, scale, tx, ty) {
    try {
      const rect = info.viewportEl.getBoundingClientRect();
      const vpW = Math.max(1, Number(rect.width) || 1);
      const vpH = Math.max(1, Number(rect.height) || 1);
      const d = info.design || {};
      const cw = Math.max(1, Number(d.width) || 320);
      const ch = Math.max(1, Number(d.height) || 240);
      const s = Math.max(0.1, Math.min(8, Number(scale) || 1));
      const canvasW = cw * s;
      const canvasH = ch * s;
      let nextTx = Number(tx) || 0;
      let nextTy = Number(ty) || 0;
      if (canvasW <= vpW) nextTx = (vpW - canvasW) / 2;
      else nextTx = Math.max(vpW - canvasW, Math.min(0, nextTx));
      if (canvasH <= vpH) nextTy = (vpH - canvasH) / 2;
      else nextTy = Math.max(vpH - canvasH, Math.min(0, nextTy));
      return { tx: nextTx, ty: nextTy };
    } catch {
      return { tx: Number(tx) || 0, ty: Number(ty) || 0 };
    }
  }

  // 函数级注释：按当前控件布局自动调整画布尺寸
  function fitCanvasToWidgets(tid) {
    const info = tid ? NS.grids.get(tid) : null;
    if (!info || !info.grid) return null;
    const bounds = computeCanvasBoundsFromGrid(info.grid);
    const bg = __defaultCanvasBg(info);
    info.design = { width: bounds.width, height: bounds.height, bgColor: bg, auto: true };
    if (info.canvasEl) {
      info.canvasEl.style.width = bounds.width + 'px';
      info.canvasEl.style.height = bounds.height + 'px';
      info.canvasEl.style.backgroundColor = bg;
    }
    if (info.viewportEl) {
      const v = __getCanvasViewState(tid);
      const t = __clampCanvasTranslate(info, v.scale, v.tx, v.ty);
      v.tx = t.tx;
      v.ty = t.ty;
      __applyCanvasViewTransform(tid);
    }
    return bounds;
  }

  function scheduleFitCanvasToWidgets(tid) {
    if (!tid) return;
    if (__fitCanvasTimers.has(tid)) clearTimeout(__fitCanvasTimers.get(tid));
    __fitCanvasTimers.set(tid, setTimeout(() => {
      __fitCanvasTimers.delete(tid);
      try { fitCanvasToWidgets(tid); } catch {}
    }, 32));
  }

  function __tidFromGrid(grid) {
    let tid = null;
    try {
      NS.grids.forEach((info, key) => {
        if (info && info.grid === grid) tid = key;
      });
    } catch {}
    return tid;
  }

  // 函数级注释：清空布局容器（像素画布），同时清理 commandId 索引
  function clearLayoutContainer(grid) {
    try {
      const el = grid && grid.el ? grid.el : null;
      if (el) {
        const items = el.querySelectorAll('.grid-stack-item');
        items.forEach(item => {
          try { EPWidgets.unindexNode(item); } catch {}
        });
        el.innerHTML = '';
      }
      const tid = __tidFromGrid(grid);
      if (tid) scheduleFitCanvasToWidgets(tid);
    } catch {}
  }

  // 函数级注释：应用画布背景色，尺寸随控件自动计算
  function applyPageDesign(tid, design) {
    const info = NS.grids.get(tid);
    if (!info) return;
    const d = design && typeof design === 'object' ? design : null;
    const bgRaw = (d && d.bgColor !== undefined && d.bgColor !== null)
      ? String(d.bgColor)
      : __defaultCanvasBg(info);
    const bg = (bgRaw || '').trim() || __defaultCanvasBg(info);
    if (!info.design || typeof info.design !== 'object') {
      info.design = { width: 320, height: 240, bgColor: bg, auto: true };
    } else {
      info.design.bgColor = bg;
    }
    if (info.canvasEl) info.canvasEl.style.backgroundColor = bg;
    scheduleFitCanvasToWidgets(tid);
  }

  // 函数级注释：获取指定分页的画布视图状态（缩放/平移），若不存在则创建默认值
  function __getCanvasViewState(tid) {
    const info = tid ? NS.grids.get(tid) : null;
    if (!info) return { scale: 1, tx: 0, ty: 0 };
    if (info.view && typeof info.view === 'object') return info.view;
    info.view = { scale: 1, tx: 0, ty: 0 };
    return info.view;
  }

  // 函数级注释：将视图状态应用到指定分页的像素画布 DOM（CSS transform）
  function __applyCanvasViewTransform(tid) {
    const info = tid ? NS.grids.get(tid) : null;
    if (!info || !info.canvasEl) return;
    const v = __getCanvasViewState(tid);
    const s = Math.max(0.1, Math.min(8, Number(v.scale) || 1));
    const tx = Number(v.tx) || 0;
    const ty = Number(v.ty) || 0;
    v.scale = s;
    v.tx = tx;
    v.ty = ty;
    info.canvasEl.style.transform = 'translate(' + tx + 'px,' + ty + 'px) scale(' + s + ')';
  }

  // 函数级注释：从本地存储恢复指定分页的视图状态（缩放/平移）
  function __loadCanvasViewState(tid) {
    const info = tid ? NS.grids.get(tid) : null;
    if (!info) return;
    try {
      const raw = localStorage.getItem('ns_view_' + tid);
      if (!raw) return;
      const obj = JSON.parse(raw);
      if (!obj || typeof obj !== 'object') return;
      const scale = Math.max(0.1, Math.min(8, Number(obj.scale) || 1));
      const tx = Number(obj.tx) || 0;
      const ty = Number(obj.ty) || 0;
      info.view = { scale, tx, ty };
    } catch {}
  }

  // 函数级注释：将指定分页的视图状态持久化到本地存储（带防抖）
  function __persistCanvasViewState(tid) {
    const info = tid ? NS.grids.get(tid) : null;
    if (!info) return;
    if (info.__viewSaveTimer) clearTimeout(info.__viewSaveTimer);
    info.__viewSaveTimer = setTimeout(() => {
      try {
        const v = __getCanvasViewState(tid);
        localStorage.setItem('ns_view_' + tid, JSON.stringify({ scale: v.scale, tx: v.tx, ty: v.ty }));
      } catch {}
    }, 200);
  }

  // 函数级注释：重置当前画布视图为居中状态
  function resetCanvasView(tid) {
    const info = (tid || NS.activeTabId) ? NS.grids.get(tid || NS.activeTabId) : null;
    if (!info || !info.viewportEl) return;
    const v = __getCanvasViewState(tid || NS.activeTabId);
    const vpRect = info.viewportEl.getBoundingClientRect();
    const d = info.design || { width: 320, height: 240 };
    const cw = Number(d.width) || 1280;
    const ch = Number(d.height) || 720;
    const s = Math.max(0.1, Math.min(8, Number(v.scale) || 1));
    v.scale = s;
    v.tx = Math.max(0, (vpRect.width - cw * s) / 2);
    v.ty = Math.max(0, (vpRect.height - ch * s) / 2);
    __applyCanvasViewTransform(tid || NS.activeTabId);
    __persistCanvasViewState(tid || NS.activeTabId);
  }

  // 函数级注释：将鼠标/触点 client 坐标转换为画布设计坐标系 px（考虑缩放/平移）
  function __clientToCanvasDesignXY(canvasEl, clientX, clientY) {
    const c = canvasEl;
    if (!c) return { x: 0, y: 0 };
    const vp = (c.closest && c.closest('.canvas-viewport')) ? c.closest('.canvas-viewport') : null;
    const rect = (vp || c).getBoundingClientRect();
    const tid = NS.activeTabId || (vp && vp.closest && vp.closest('[data-tab-id]') ? (vp.closest('[data-tab-id]').dataset.tabId || '') : '');
    const info = tid ? NS.grids.get(tid) : null;
    const v = tid ? __getCanvasViewState(tid) : { scale: 1, tx: 0, ty: 0 };
    const scale = Math.max(0.1, Math.min(8, Number(v.scale) || 1));

    const vx = (Number(clientX) || 0) - rect.left;
    const vy = (Number(clientY) || 0) - rect.top;
    const lx0 = (vx - (Number(v.tx) || 0)) / scale;
    const ly0 = (vy - (Number(v.ty) || 0)) / scale;
    const x = Math.max(0, lx0);
    const y = Math.max(0, ly0);
    return { x, y };
  }

  // 函数级注释：确保全局按键状态跟踪已初始化（用于按住空格拖动平移）
  function __ensureCanvasPanZoomKeyTracker() {
    if (window.__nsCanvasPanZoomKeyBound) return;
    window.__nsCanvasPanZoomKeyBound = true;
    window.__nsSpaceDown = false;
    window.addEventListener('keydown', (e) => { if (e && e.code === 'Space') window.__nsSpaceDown = true; }, true);
    window.addEventListener('keyup', (e) => { if (e && e.code === 'Space') window.__nsSpaceDown = false; }, true);
    window.addEventListener('blur', () => { window.__nsSpaceDown = false; }, true);
  }

  // 函数级注释：为指定分页初始化画布缩放/平移交互（编辑模式与视图模式通用）
  function __initCanvasPanZoom(tid) {
    const info = tid ? NS.grids.get(tid) : null;
    if (!info || !info.viewportEl || !info.canvasEl) return;
    if (info.viewportEl.dataset.panZoomBound) {
      __applyCanvasViewTransform(tid);
      return;
    }
    info.viewportEl.dataset.panZoomBound = '1';

    __ensureCanvasPanZoomKeyTracker();
    const hadSavedView = !!(info.view && typeof info.view === 'object' && (info.view.scale !== 1 || info.view.tx !== 0 || info.view.ty !== 0));
    __loadCanvasViewState(tid);
    __applyCanvasViewTransform(tid);

    // 函数级注释：根据画布设计尺寸与视口尺寸，约束平移范围，避免拖出画布边界出现空白区域
    function __clampTranslate(scale, tx, ty) {
      try {
        const rect = info.viewportEl.getBoundingClientRect();
        const vpW = Math.max(1, Number(rect.width) || 1);
        const vpH = Math.max(1, Number(rect.height) || 1);
        const d = info.design || { width: 320, height: 240 };
        const cw = Math.max(1, Number(d.width) || 1280);
        const ch = Math.max(1, Number(d.height) || 720);
        const s = Math.max(0.1, Math.min(8, Number(scale) || 1));
        const canvasW = cw * s;
        const canvasH = ch * s;

        let nextTx = Number(tx) || 0;
        let nextTy = Number(ty) || 0;

        if (canvasW <= vpW) {
          nextTx = (vpW - canvasW) / 2;
        } else {
          const minTx = vpW - canvasW;
          const maxTx = 0;
          nextTx = Math.max(minTx, Math.min(maxTx, nextTx));
        }

        if (canvasH <= vpH) {
          nextTy = (vpH - canvasH) / 2;
        } else {
          const minTy = vpH - canvasH;
          const maxTy = 0;
          nextTy = Math.max(minTy, Math.min(maxTy, nextTy));
        }

        return { tx: nextTx, ty: nextTy };
      } catch {
        return { tx: Number(tx) || 0, ty: Number(ty) || 0 };
      }
    }

    // 若无保存的视图状态，初始化时将画布居中于视口（消除顶部空白）
    if (!hadSavedView && info.view) {
      try {
        const vpRect = info.viewportEl.getBoundingClientRect();
        const d = info.design || { width: 320, height: 240 };
        const cw = Number(d.width) || 1280;
        const ch = Number(d.height) || 720;
        const nextTx = Math.max(0, (vpRect.width - cw * info.view.scale) / 2);
        const nextTy = Math.max(0, (vpRect.height - ch * info.view.scale) / 2);
        if (nextTx !== info.view.tx || nextTy !== info.view.ty) {
          info.view.tx = nextTx;
          info.view.ty = nextTy;
          __applyCanvasViewTransform(tid);
          __persistCanvasViewState(tid);
        }
      } catch {}
    }

    let isPanning = false;
    let panStart = { x: 0, y: 0, tx: 0, ty: 0 };
    const pointers = new Map();
    let pinchStart = null;

    const clampScale = (s) => Math.max(0.1, Math.min(8, Number(s) || 1));
    const getViewportRect = () => info.viewportEl.getBoundingClientRect();
    const setView = (next) => {
      const v = __getCanvasViewState(tid);
      const s = clampScale(next.scale);
      const t = __clampTranslate(s, next.tx, next.ty);
      v.scale = s;
      v.tx = t.tx;
      v.ty = t.ty;
      __applyCanvasViewTransform(tid);
      __persistCanvasViewState(tid);
    };

    const shouldIgnoreWheel = (e) => {
      if (!e) return true;
      const t = e.target;
      try { if (t && t.closest && t.closest('.grid-stack-item')) return true; } catch {}
      return false;
    };

    const onWheel = (e) => {
      try {
        if (!e) return;
        if (shouldIgnoreWheel(e)) return;
        const v = __getCanvasViewState(tid);
        const rect = getViewportRect();
        const mx = (Number(e.clientX) || 0) - rect.left;
        const my = (Number(e.clientY) || 0) - rect.top;

        const isZoom = !!(e.ctrlKey || e.metaKey);
        const delta = Number(e.deltaY) || 0;
        const deltaX = Number(e.deltaX) || 0;

        if (e.cancelable) e.preventDefault();

        if (isZoom) {
          const factor = Math.exp(-delta * 0.0015);
          const nextScale = clampScale((Number(v.scale) || 1) * factor);
          const worldX = (mx - (Number(v.tx) || 0)) / (Number(v.scale) || 1);
          const worldY = (my - (Number(v.ty) || 0)) / (Number(v.scale) || 1);
          const nextTx = mx - worldX * nextScale;
          const nextTy = my - worldY * nextScale;
          setView({ scale: nextScale, tx: nextTx, ty: nextTy });
        } else {
          setView({ scale: v.scale, tx: (Number(v.tx) || 0) - deltaX, ty: (Number(v.ty) || 0) - delta });
        }
      } catch {}
    };

    const onPointerDown = (e) => {
      try {
        if (!e) return;
        const isTouch = e.pointerType === 'touch';
        // 触摸：如果点在控件上则不处理（由网格系统接管）
        try { if (isTouch && e.target && e.target.closest && e.target.closest('.grid-stack-item')) return; } catch {}
        const isMiddle = e.button === 1;
        const isLeft = e.button === 0;
        try { if (isLeft && e.shiftKey && document.body.classList.contains('edit-mode')) return; } catch {}
        const isSpacePan = !!(window.__nsSpaceDown) && isLeft;
        // 左键：只有点击空白区域才触发平移；中键始终可以平移
        const onWidget = !!(e.target && e.target.closest && e.target.closest('.grid-stack-item'));
        const allowPan = isTouch || isMiddle || (isLeft && !onWidget) || isSpacePan;
        if (!allowPan) return;

        try { if (e.cancelable) e.preventDefault(); } catch {}
        try { info.viewportEl.setPointerCapture(e.pointerId); } catch {}

        pointers.set(e.pointerId, { x: Number(e.clientX) || 0, y: Number(e.clientY) || 0 });

        const v = __getCanvasViewState(tid);
        if (pointers.size === 1) {
          isPanning = true;
          panStart = { x: Number(e.clientX) || 0, y: Number(e.clientY) || 0, tx: Number(v.tx) || 0, ty: Number(v.ty) || 0 };
          try { info.viewportEl.style.cursor = 'grabbing'; } catch {}
        } else if (pointers.size === 2) {
          isPanning = false;
          const pts = Array.from(pointers.values());
          const dx = pts[1].x - pts[0].x;
          const dy = pts[1].y - pts[0].y;
          const dist = Math.max(1, Math.hypot(dx, dy));
          const rect = getViewportRect();
          const cx = ((pts[0].x + pts[1].x) / 2) - rect.left;
          const cy = ((pts[0].y + pts[1].y) / 2) - rect.top;
          const worldX = (cx - (Number(v.tx) || 0)) / (Number(v.scale) || 1);
          const worldY = (cy - (Number(v.ty) || 0)) / (Number(v.scale) || 1);
          pinchStart = { dist, cx, cy, worldX, worldY, scale: Number(v.scale) || 1 };
          try { info.viewportEl.style.cursor = 'grabbing'; } catch {}
        }
      } catch {}
    };

    const onPointerMove = (e) => {
      try {
        if (!e) return;
        if (!pointers.has(e.pointerId)) return;
        pointers.set(e.pointerId, { x: Number(e.clientX) || 0, y: Number(e.clientY) || 0 });
        const v = __getCanvasViewState(tid);

        if (pointers.size === 2 && pinchStart) {
          const pts = Array.from(pointers.values());
          const dx = pts[1].x - pts[0].x;
          const dy = pts[1].y - pts[0].y;
          const dist = Math.max(1, Math.hypot(dx, dy));
          const rect = getViewportRect();
          const cx = ((pts[0].x + pts[1].x) / 2) - rect.left;
          const cy = ((pts[0].y + pts[1].y) / 2) - rect.top;
          const nextScale = clampScale((Number(pinchStart.scale) || 1) * (dist / (Number(pinchStart.dist) || 1)));
          const nextTx = cx - (Number(pinchStart.worldX) || 0) * nextScale;
          const nextTy = cy - (Number(pinchStart.worldY) || 0) * nextScale;
          setView({ scale: nextScale, tx: nextTx, ty: nextTy });
          return;
        }

        if (!isPanning) return;
        const dx = (Number(e.clientX) || 0) - (Number(panStart.x) || 0);
        const dy = (Number(e.clientY) || 0) - (Number(panStart.y) || 0);
        setView({ scale: v.scale, tx: (Number(panStart.tx) || 0) + dx, ty: (Number(panStart.ty) || 0) + dy });
      } catch {}
    };

    const onPointerUp = (e) => {
      try {
        if (!e) return;
        pointers.delete(e.pointerId);
        try { info.viewportEl.releasePointerCapture(e.pointerId); } catch {}
        if (pointers.size < 2) pinchStart = null;
        if (pointers.size === 0) {
          isPanning = false;
          try { info.viewportEl.style.cursor = ''; } catch {}
        }
      } catch {}
    };

    info.viewportEl.addEventListener('wheel', onWheel, { passive: false });
    info.viewportEl.addEventListener('pointerdown', onPointerDown, true);
    info.viewportEl.addEventListener('pointermove', onPointerMove, true);
    info.viewportEl.addEventListener('pointerup', onPointerUp, true);
    info.viewportEl.addEventListener('pointercancel', onPointerUp, true);

    // 函数级注释：初始化完交互后，对当前视图做一次边界约束，避免保存的 tx/ty 越界导致空白
    try {
      const v0 = __getCanvasViewState(tid);
      setView({ scale: v0.scale, tx: v0.tx, ty: v0.ty });
    } catch {}
  }

  // 导出
  window.NSCanvas = {
    clearLayoutContainer,
    applyPageDesign,
    fitCanvasToWidgets,
    scheduleFitCanvasToWidgets,
    computeCanvasBoundsFromGrid,
    resetCanvasView,
    __getCanvasViewState,
    __applyCanvasViewTransform,
    __loadCanvasViewState,
    __persistCanvasViewState,
    __clientToCanvasDesignXY,
    __initCanvasPanZoom
  };
})();
