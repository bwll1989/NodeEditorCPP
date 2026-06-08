// ns-ws-status.js —— WebSocket 连接状态条（断开/重连时顶部提示）
(function() {
  'use strict';

  let barEl = null;

  function ensureBar() {
    if (barEl) return barEl;
    barEl = document.getElementById('wsStatusBar');
    if (barEl) return barEl;
    barEl = document.createElement('div');
    barEl.id = 'wsStatusBar';
    barEl.className = 'ws-status-bar';
    barEl.setAttribute('role', 'status');
    barEl.setAttribute('aria-live', 'polite');
    const main = document.querySelector('.main-area');
    if (main) main.insertBefore(barEl, main.firstChild);
    else document.body.appendChild(barEl);
    return barEl;
  }

  function setStatus(stateText) {
    const el = ensureBar();
    const txt = String(stateText || '').trim();
    if (txt === '已连接') {
      el.classList.remove('ws-status-visible');
      el.textContent = '';
      return;
    }
    el.textContent = '主机：' + (txt || '未知');
    el.classList.add('ws-status-visible');
    if (txt === '连接中…') el.dataset.level = 'warn';
    else el.dataset.level = 'error';
  }

  window.NSWsStatus = { setStatus, ensureBar };
})();
