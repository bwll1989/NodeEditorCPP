// ns-boot.js —— 首屏骨架显示 / 隐藏
(function() {
  'use strict';

  let hidden = false;
  let timer = null;

  function hide() {
    if (hidden) return;
    hidden = true;
    try { if (timer) clearTimeout(timer); } catch {}
    const el = document.getElementById('nsBootSkeleton');
    if (!el) return;
    el.classList.add('ns-boot-skeleton-hide');
    el.setAttribute('aria-busy', 'false');
    setTimeout(() => {
      try { if (el.parentNode) el.parentNode.removeChild(el); } catch {}
    }, 280);
  }

  function armFallback(ms) {
    try { if (timer) clearTimeout(timer); } catch {}
    timer = setTimeout(hide, Math.max(800, Number(ms) || 2500));
  }

  window.NSBoot = { hide, armFallback };
  window.addEventListener('DOMContentLoaded', () => {
    try { armFallback(2800); } catch {}
  });
})();
