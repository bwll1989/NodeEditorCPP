// ns-menu.js —— 轻量下拉菜单（替代 Bootstrap dropdown）
(function() {
  'use strict';

  function closeAll(except) {
    document.querySelectorAll('.ns-menu.open, .dropdown.open').forEach(el => {
      if (except && el === except) return;
      el.classList.remove('open');
      const btn = el.querySelector('[data-ns-menu-toggle], [data-bs-toggle="dropdown"], .top-nav-brand-btn');
      if (btn) btn.setAttribute('aria-expanded', 'false');
    });
  }

  function toggleMenu(root) {
    if (!root) return;
    const willOpen = !root.classList.contains('open');
    closeAll(willOpen ? root : null);
    root.classList.toggle('open', willOpen);
    const btn = root.querySelector('[data-ns-menu-toggle], [data-bs-toggle="dropdown"], .top-nav-brand-btn');
    if (btn) btn.setAttribute('aria-expanded', willOpen ? 'true' : 'false');
  }

  function bindRoot(root) {
    if (!root || root.dataset.nsMenuBound) return;
    root.dataset.nsMenuBound = '1';
    root.classList.add('ns-menu');
    const btn = root.querySelector('[data-ns-menu-toggle], [data-bs-toggle="dropdown"], .top-nav-brand-btn');
    const panel = root.querySelector('.ns-menu-panel, .dropdown-menu');
    if (!btn || !panel) return;
    btn.setAttribute('aria-haspopup', 'true');
    btn.setAttribute('aria-expanded', 'false');
    btn.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      toggleMenu(root);
    });
    panel.addEventListener('click', (e) => {
      const item = e.target && e.target.closest ? e.target.closest('.dropdown-item, [data-ns-menu-item]') : null;
      if (!item || item.disabled || item.classList.contains('disabled')) return;
      // 点击菜单项后关闭（保留 disabled 项）
      closeAll();
    });
  }

  function init(selector) {
    const roots = document.querySelectorAll(selector || '.ns-menu, .dropdown');
    roots.forEach(bindRoot);
  }

  document.addEventListener('click', () => closeAll());
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') closeAll();
  });
  document.addEventListener('DOMContentLoaded', () => init());

  window.NSMenu = { init, closeAll, toggleMenu, bindRoot };
})();
