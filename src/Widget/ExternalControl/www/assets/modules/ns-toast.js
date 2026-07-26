// ns-toast.js —— 轻量 Toast，替代打断式 alert
(function() {
  'use strict';

  let host = null;

  function ensureHost() {
    if (host && host.isConnected) return host;
    host = document.getElementById('nsToastHost');
    if (!host) {
      host = document.createElement('div');
      host.id = 'nsToastHost';
      host.className = 'ns-toast-host';
      host.setAttribute('aria-live', 'polite');
      host.setAttribute('aria-relevant', 'additions');
      document.body.appendChild(host);
    }
    return host;
  }

  function show(message, opts) {
    const options = opts || {};
    const type = String(options.type || 'info');
    const duration = Math.max(1200, Number(options.duration) || 2800);
    const text = String(message == null ? '' : message).trim();
    if (!text) return null;

    const el = document.createElement('div');
    el.className = 'ns-toast';
    el.dataset.type = type;
    el.setAttribute('role', type === 'error' ? 'alert' : 'status');
    el.innerHTML =
      '<span class="ns-toast-dot" aria-hidden="true"></span>' +
      '<div class="ns-toast-msg"></div>';
    el.querySelector('.ns-toast-msg').textContent = text;
    ensureHost().appendChild(el);

    requestAnimationFrame(() => {
      try { el.classList.add('ns-toast-show'); } catch {}
    });

    const remove = () => {
      try { el.classList.remove('ns-toast-show'); } catch {}
      setTimeout(() => {
        try { if (el.parentNode) el.parentNode.removeChild(el); } catch {}
      }, 200);
    };
    const timer = setTimeout(remove, duration);
    el.addEventListener('click', () => {
      clearTimeout(timer);
      remove();
    });
    return el;
  }

  function success(message, opts) {
    return show(message, Object.assign({}, opts, { type: 'success' }));
  }
  function error(message, opts) {
    return show(message, Object.assign({}, opts, { type: 'error', duration: (opts && opts.duration) || 4200 }));
  }
  function warn(message, opts) {
    return show(message, Object.assign({}, opts, { type: 'warn', duration: (opts && opts.duration) || 3600 }));
  }
  function info(message, opts) {
    return show(message, Object.assign({}, opts, { type: 'info' }));
  }

  window.NSToast = { show, success, error, warn, info };
})();
