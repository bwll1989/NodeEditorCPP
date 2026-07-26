// ns-empty-state.js —— 画布空态：无控件时显示引导
(function() {
  'use strict';

  const observers = new WeakMap();

  const EMPTY_HTML =
    '<div class="ns-canvas-empty-card">' +
      '<div class="ns-canvas-empty-icon" aria-hidden="true">' +
        '<svg viewBox="0 0 24 24"><rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/><path d="M14 17.5h7M17.5 14v7"/></svg>' +
      '</div>' +
      '<div class="ns-canvas-empty-title">画布为空</div>' +
      '<div class="ns-canvas-empty-desc edit-hint">从右侧「控件库」拖入控件开始设计</div>' +
      '<div class="ns-canvas-empty-desc view-hint">打开菜单 →「编辑模式」后可添加控件</div>' +
    '</div>';

  function ensureEmptyEl(viewportEl) {
    if (!viewportEl) return null;
    let el = viewportEl.querySelector(':scope > .ns-canvas-empty');
    if (!el) {
      el = document.createElement('div');
      el.className = 'ns-canvas-empty';
      el.setAttribute('aria-hidden', 'true');
      el.innerHTML = EMPTY_HTML;
      viewportEl.appendChild(el);
    }
    return el;
  }

  function countWidgets(canvasEl) {
    if (!canvasEl || !canvasEl.querySelectorAll) return 0;
    try {
      return canvasEl.querySelectorAll(':scope > .grid-stack-item').length;
    } catch {
      return canvasEl.querySelectorAll('.grid-stack-item').length;
    }
  }

  function refreshFor(viewportEl, canvasEl) {
    const empty = ensureEmptyEl(viewportEl);
    if (!empty) return;
    const n = countWidgets(canvasEl);
    const show = n <= 0;
    empty.classList.toggle('ns-empty-visible', show);
    empty.setAttribute('aria-hidden', show ? 'false' : 'true');
  }

  function refreshTab(tid) {
    try {
      const NS = window.NS;
      const id = tid || (NS && NS.activeTabId);
      const info = id && NS && NS.grids ? NS.grids.get(id) : null;
      if (!info) return;
      refreshFor(info.viewportEl, info.canvasEl || (info.grid && info.grid.el));
    } catch {}
  }

  function refreshAll() {
    try {
      const NS = window.NS;
      if (!NS || !NS.grids) return;
      NS.grids.forEach((_info, tid) => refreshTab(tid));
    } catch {}
  }

  function observeTab(tid) {
    try {
      const NS = window.NS;
      const info = tid && NS && NS.grids ? NS.grids.get(tid) : null;
      if (!info || !info.canvasEl) return;
      ensureEmptyEl(info.viewportEl);
      refreshFor(info.viewportEl, info.canvasEl);
      if (observers.has(info.canvasEl)) return;
      const mo = new MutationObserver(() => {
        refreshFor(info.viewportEl, info.canvasEl);
      });
      mo.observe(info.canvasEl, { childList: true });
      observers.set(info.canvasEl, mo);
    } catch {}
  }

  window.NSEmptyState = { refreshTab, refreshAll, observeTab, ensureEmptyEl };
})();
