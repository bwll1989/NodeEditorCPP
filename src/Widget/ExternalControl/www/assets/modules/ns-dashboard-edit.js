// ns-dashboard-edit.js —— 编辑模式切换与侧栏状态
(function() {
  'use strict';

  function init(ctx) {
    function isMobileEditDisabled() {
      try {
        const ua = String(navigator.userAgent || '').toLowerCase();
        const mobileUa = /iphone|ipad|ipod|android|mobile/.test(ua);
        const coarse = !!(window.matchMedia && window.matchMedia('(pointer: coarse)').matches);
        const narrow = !!(window.matchMedia && window.matchMedia('(max-width: 900px)').matches);
        return mobileUa || (coarse && narrow);
      } catch { return false; }
    }

    function getGlobalEditMode() {
      if (isMobileEditDisabled()) return false;
      try {
        const raw = localStorage.getItem('ns_edit_mode');
        if (raw === null || raw === undefined) return false;
        return raw === 'true';
      } catch { return false; }
    }

    function applySidebarsCollapsed(collapsed) {
      const sideRight = document.querySelector('.sidebar-right');
      if (collapsed) {
        if (sideRight) sideRight.classList.add('collapsed');
      } else {
        if (sideRight) sideRight.classList.remove('collapsed');
      }
    }

    function syncModeBadge(isEdit) {
      try {
        const badge = document.getElementById('modeBadge');
        if (!badge) return;
        badge.dataset.mode = isEdit ? 'edit' : 'view';
        badge.title = isEdit ? '当前为编辑模式' : '当前为运行模式';
        const label = badge.querySelector('.ns-mode-badge-label');
        if (label) label.textContent = isEdit ? '编辑中' : '运行';
        else badge.textContent = isEdit ? '编辑中' : '运行';
      } catch {}
    }

    function syncEditChrome(isEdit) {
      const editing = !!isEdit;
      const btn = document.getElementById('toggleEditMode');
      if (btn) {
        if (isMobileEditDisabled()) {
          btn.style.display = 'none';
        } else {
          btn.style.display = '';
          btn.dataset.editing = editing ? '1' : '0';
          btn.classList.toggle('active', editing);
          btn.title = editing ? '完成编辑并返回运行模式' : '进入编辑模式';
          const label = btn.querySelector('[data-edit-label]');
          if (label) label.textContent = editing ? '完成编辑' : '编辑';
        }
      }
      const banner = document.getElementById('editModeBanner');
      if (banner) {
        banner.dataset.visible = (editing && !isMobileEditDisabled()) ? '1' : '0';
      }
    }

    function applyEditModeAll(isEdit) {
      try {
        ctx.services.NS.grids.forEach(info => {
          if (info && info.grid && typeof info.grid.setStatic === 'function') info.grid.setStatic(!isEdit);
        });
      } catch {}
      if (isEdit) {
        document.body.classList.add('edit-mode');
        document.body.classList.remove('view-mode');
      } else {
        document.body.classList.add('view-mode');
        document.body.classList.remove('edit-mode');
        ctx.selection.clearSelection();
      }
      syncModeBadge(!!isEdit);
      syncEditChrome(!!isEdit);
      applySidebarsCollapsed(!isEdit);
      try {
        if (isEdit) {
          if (!window.__splitInstance) ctx.services.NSWsSync.setupSplitPanels();
        } else {
          if (window.__splitInstance && typeof window.__splitInstance.destroy === 'function') {
            try { window.__splitInstance.destroy(false, false); } catch {}
            window.__splitInstance = null;
          }
        }
      } catch {}
      try { if (typeof NSInteract !== 'undefined' && typeof ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate === 'function') ctx.services.NSInteract.__scheduleGroupIndicatorsUpdate(); } catch {}
      try {
        document.querySelectorAll('.tab-close').forEach(el => {
          el.style.display = isEdit ? '' : 'none';
        });
      } catch {}
      try {
        const addBtn = document.getElementById('sidebarAddTab');
        if (addBtn) addBtn.style.display = isEdit ? '' : 'none';
      } catch {}
      try { ctx.props.updatePropPanel(ctx.state.currentSelected); } catch {}
      try { if (window.NSEmptyState) window.NSEmptyState.refreshAll(); } catch {}
    }

    function setGlobalEditMode(isEdit) {
      const next = isMobileEditDisabled() ? false : !!isEdit;
      try { localStorage.setItem('ns_edit_mode', next ? 'true' : 'false'); } catch {}
      applyEditModeAll(next);
      if (next) {
        try {
          if (window.NSUtils && typeof window.NSUtils.ensureWidgetReady === 'function') {
            window.NSUtils.ensureWidgetReady('Trigger 按钮', () => {});
          }
        } catch {}
      }
    }

    async function toggleEditModeFromUi() {
      const current = getGlobalEditMode();
      if (!current) {
        const ok = await ctx.services.NSWsSync.requireSettingAuth();
        if (!ok) return;
      }
      setGlobalEditMode(!current);
      if (current) { ctx.layout.saveAllLayouts(); }
      try {
        if (window.NSToast) {
          window.NSToast.info(current ? '已退出编辑' : '已进入编辑模式', { duration: 1600 });
        }
      } catch {}
    }

    function bindEditModeControls() {
      const editBtn = document.getElementById('toggleEditMode');
      if (editBtn) editBtn.onclick = () => { toggleEditModeFromUi(); };

      const bannerDone = document.getElementById('editBannerDone');
      if (bannerDone) {
        bannerDone.onclick = () => {
          if (getGlobalEditMode()) toggleEditModeFromUi();
        };
      }

      const openSettingsBtn = document.getElementById('openSettings');
      if (openSettingsBtn) {
        openSettingsBtn.addEventListener('click', async (e) => {
          try { e.preventDefault(); e.stopPropagation(); } catch {}
          try { await ctx.services.NSWsSync.openSettingsPage(); } catch {}
        });
      }
    }

    function restoreEditModeOnLoad() {
      (async () => {
        const mobileDisableEdit = isMobileEditDisabled();
        if (mobileDisableEdit) {
          try { localStorage.setItem('ns_edit_mode', 'false'); } catch {}
        }
        let initialEdit = mobileDisableEdit ? false : getGlobalEditMode();
        if (initialEdit) {
          const ok = await ctx.services.NSWsSync.requireSettingAuth();
          if (!ok) {
            initialEdit = false;
            try { localStorage.setItem('ns_edit_mode', 'false'); } catch {}
          }
        }
        applyEditModeAll(initialEdit);
      })();
    }

    return {
      isMobileEditDisabled,
      getGlobalEditMode,
      applySidebarsCollapsed,
      applyEditModeAll,
      setGlobalEditMode,
      bindEditModeControls,
      restoreEditModeOnLoad
    };
  }

  window.NSDashboardEdit = { init };
})();
