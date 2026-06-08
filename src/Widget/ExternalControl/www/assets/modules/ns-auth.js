// ns-auth.js —— 设置页/编辑模式密码校验（Bootstrap 模态框，避免 window.prompt 被浏览器拦截）
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
    root.className = 'modal fade';
    root.tabIndex = -1;
    root.setAttribute('aria-hidden', 'true');
    root.setAttribute('aria-labelledby', 'nsAuthModalTitle');
    root.innerHTML =
      '<div class="modal-dialog modal-dialog-centered modal-sm">' +
        '<div class="modal-content">' +
          '<div class="modal-header py-2">' +
            '<h5 class="modal-title fs-6" id="nsAuthModalTitle">身份验证</h5>' +
            '<button type="button" class="btn-close" data-bs-dismiss="modal" aria-label="关闭"></button>' +
          '</div>' +
          '<div class="modal-body">' +
            '<label for="nsAuthPasswordInput" class="form-label small mb-1">密码</label>' +
            '<input type="password" id="nsAuthPasswordInput" class="form-control form-control-sm" placeholder="请输入密码" autocomplete="current-password" aria-label="密码" />' +
            '<div id="nsAuthError" class="text-danger small mt-2 d-none"></div>' +
          '</div>' +
          '<div class="modal-footer py-2">' +
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
      const btnCancel = root.querySelector('[data-ns-auth-cancel]');
      const btnClose = root.querySelector('.btn-close');
      if (!input || !btnOk || !btnCancel) {
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
      let modal = null;
      let backdrop = null;

      function cleanupFallback() {
        root.classList.remove('show');
        root.style.display = 'none';
        root.setAttribute('aria-hidden', 'true');
        document.body.classList.remove('modal-open');
        if (backdrop) {
          try { backdrop.remove(); } catch {}
          backdrop = null;
        }
      }

      function finish(value) {
        if (settled) return;
        settled = true;
        try { modal && modal.hide(); } catch {}
        cleanupFallback();
        resolve(value);
      }

      function submit() {
        finish(String(input.value || ''));
      }

      btnOk.onclick = submit;
      btnCancel.onclick = () => finish(null);
      if (btnClose) btnClose.onclick = () => finish(null);
      input.onkeydown = (e) => {
        if (e.key === 'Enter') {
          e.preventDefault();
          submit();
        } else if (e.key === 'Escape') {
          e.preventDefault();
          finish(null);
        }
      };

      root.addEventListener('hidden.bs.modal', () => finish(null), { once: true });

      try {
        if (window.bootstrap && window.bootstrap.Modal) {
          modal = window.bootstrap.Modal.getOrCreateInstance(root, { backdrop: 'static', keyboard: true });
          modal.show();
          setTimeout(() => { try { input.focus(); } catch {} }, 150);
          return;
        }
      } catch {}

      root.classList.add('show');
      root.style.display = 'block';
      root.removeAttribute('aria-hidden');
      document.body.classList.add('modal-open');
      backdrop = document.createElement('div');
      backdrop.className = 'modal-backdrop fade show';
      backdrop.onclick = () => finish(null);
      document.body.appendChild(backdrop);
      setTimeout(() => { try { input.focus(); } catch {} }, 50);
    });
  }

  async function requireSettingAuth(options) {
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
