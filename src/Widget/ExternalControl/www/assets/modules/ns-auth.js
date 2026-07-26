// ns-auth.js —— 设置页/编辑模式密码校验（自研模态框，不依赖 Bootstrap）
(function() {
  'use strict';

  const AUTH_TS_KEY = 'ns_setting_auth_ts';
  const AUTH_TTL_MS = 10 * 60 * 1000;

  function markSettingAuthed() {
    try { sessionStorage.setItem(AUTH_TS_KEY, String(Date.now())); } catch {}
  }

  function isSettingAuthedRecently() {
    try {
      const ts = Number(sessionStorage.getItem(AUTH_TS_KEY) || 0);
      return ts > 0 && (Date.now() - ts) < AUTH_TTL_MS;
    } catch {
      return false;
    }
  }

  async function verifySettingPassword(password) {
    try {
      const resp = await fetch('/api/auth/setting', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ password: password || '' })
      });
      const j = await resp.json().catch(() => null);
      return !!(resp.ok && j && j.ok);
    } catch {
      return false;
    }
  }

  function ensureAuthModal() {
    let root = document.getElementById('nsAuthModal');
    if (root) return root;

    root = document.createElement('div');
    root.id = 'nsAuthModal';
    root.className = 'ns-modal';
    root.setAttribute('aria-hidden', 'true');
    root.setAttribute('role', 'dialog');
    root.setAttribute('aria-modal', 'true');
    root.setAttribute('aria-labelledby', 'nsAuthModalTitle');
    root.innerHTML =
      '<div class="ns-modal-backdrop" data-ns-auth-backdrop></div>' +
      '<div class="ns-modal-dialog modal-sm">' +
        '<div class="ns-modal-content modal-content">' +
          '<div class="ns-modal-header modal-header">' +
            '<h5 class="modal-title" id="nsAuthModalTitle">身份验证</h5>' +
            '<button type="button" class="btn-close" data-ns-auth-cancel aria-label="关闭"></button>' +
          '</div>' +
          '<div class="ns-modal-body modal-body">' +
            '<label for="nsAuthPasswordInput" class="form-label">密码</label>' +
            '<input type="password" id="nsAuthPasswordInput" class="form-control form-control-sm" placeholder="请输入密码" autocomplete="current-password" aria-label="密码" />' +
            '<div id="nsAuthError" class="text-danger small mt-2 d-none"></div>' +
          '</div>' +
          '<div class="ns-modal-footer modal-footer">' +
            '<button type="button" class="btn btn-secondary btn-sm" data-ns-auth-cancel>取消</button>' +
            '<button type="button" class="btn btn-primary btn-sm" data-ns-auth-ok>确定</button>' +
          '</div>' +
        '</div>' +
      '</div>';
    document.body.appendChild(root);
    return root;
  }

  function showPasswordDialog(title, errorMsg) {
    return new Promise((resolve) => {
      const root = ensureAuthModal();
      const titleEl = document.getElementById('nsAuthModalTitle');
      const input = document.getElementById('nsAuthPasswordInput');
      const errEl = document.getElementById('nsAuthError');
      const btnOk = root.querySelector('[data-ns-auth-ok]');
      const cancelBtns = root.querySelectorAll('[data-ns-auth-cancel]');
      if (!input || !btnOk) {
        resolve(null);
        return;
      }

      if (titleEl) titleEl.textContent = title || '请输入密码';
      input.value = '';
      if (errEl) {
        if (errorMsg) {
          errEl.textContent = errorMsg;
          errEl.classList.remove('d-none');
        } else {
          errEl.textContent = '';
          errEl.classList.add('d-none');
        }
      }

      let settled = false;

      function finish(value) {
        if (settled) return;
        settled = true;
        root.classList.remove('open', 'show');
        root.setAttribute('aria-hidden', 'true');
        document.body.classList.remove('modal-open');
        resolve(value);
      }

      function submit() {
        finish(String(input.value || ''));
      }

      btnOk.onclick = submit;
      cancelBtns.forEach(btn => {
        btn.onclick = () => finish(null);
      });
      input.onkeydown = (e) => {
        if (e.key === 'Enter') {
          e.preventDefault();
          submit();
        } else if (e.key === 'Escape') {
          e.preventDefault();
          finish(null);
        }
      };

      root.classList.add('open', 'show');
      root.setAttribute('aria-hidden', 'false');
      document.body.classList.add('modal-open');
      setTimeout(() => { try { input.focus(); input.select(); } catch {} }, 40);
    });
  }

  async function requireSettingAuth(options) {
    if (isSettingAuthedRecently()) return true;

    const title = (options && options.title) || '请输入密码';
    const okEmpty = await verifySettingPassword('');
    if (okEmpty) {
      markSettingAuthed();
      return true;
    }

    let lastError = '';
    while (true) {
      const pw = await showPasswordDialog(title, lastError);
      if (pw === null) return false;
      const ok = await verifySettingPassword(pw);
      if (ok) {
        markSettingAuthed();
        return true;
      }
      lastError = '密码错误，请重试';
    }
  }

  window.NSAuth = {
    verifySettingPassword,
    requireSettingAuth,
    markSettingAuthed,
    isSettingAuthedRecently,
    showPasswordDialog
  };
})();
