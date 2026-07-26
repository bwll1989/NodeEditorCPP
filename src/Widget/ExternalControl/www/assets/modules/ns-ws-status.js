// ns-ws-status.js —— WebSocket 连接状态（顶栏胶囊 + 断线时顶条）
(function() {
  'use strict';

  let barEl = null;

  function ensureBar() {
    if (barEl && barEl.isConnected) return barEl;
    barEl = document.getElementById('wsStatusBar');
    if (barEl) return barEl;
    barEl = document.createElement('div');
    barEl.id = 'wsStatusBar';
    barEl.className = 'ws-status-bar';
    barEl.setAttribute('role', 'status');
    barEl.setAttribute('aria-live', 'polite');
    const shell = document.querySelector('.app-shell');
    const tabsBar = document.getElementById('tabsBar');
    const banner = document.getElementById('editModeBanner');
    if (shell && tabsBar && tabsBar.parentNode === shell) {
      const after = banner && banner.parentNode === shell ? banner : tabsBar;
      shell.insertBefore(barEl, after.nextSibling);
    } else if (shell) {
      shell.insertBefore(barEl, shell.firstChild);
    } else {
      document.body.appendChild(barEl);
    }
    return barEl;
  }

  function updatePill(stateText) {
    const pill = document.getElementById('wsStatusPill');
    if (!pill) return;
    const txt = String(stateText || '').trim() || '未知';
    const textEl = pill.querySelector('.ns-ws-text');
    if (textEl) textEl.textContent = txt;
    pill.title = '主机：' + txt;
    if (txt === '已连接') pill.dataset.level = 'ok';
    else if (txt === '连接中…') pill.dataset.level = 'warn';
    else if (txt === '未知') pill.dataset.level = 'unknown';
    else pill.dataset.level = 'error';
  }

  function setStatus(stateText) {
    const txt = String(stateText || '').trim();
    updatePill(txt || '未知');

    const el = ensureBar();
    if (txt === '已连接' || !txt) {
      el.classList.remove('ws-status-visible');
      el.textContent = '';
      el.removeAttribute('data-level');
      return;
    }
    el.textContent = '主机连接异常：' + txt + '（将自动重试）';
    el.classList.add('ws-status-visible');
    if (txt === '连接中…') el.dataset.level = 'warn';
    else el.dataset.level = 'error';
  }

  window.NSWsStatus = { setStatus, ensureBar, updatePill };
})();
