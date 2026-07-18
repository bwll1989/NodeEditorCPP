// ns-dashboard-context-menu.js —— 编辑模式右键菜单（对齐/分组/撤销等）
(function() {
  'use strict';

  function init(ctx) {
    let menuEl = null;

    function ensureMenu() {
      if (menuEl) return menuEl;
      menuEl = document.createElement('div');
      menuEl.id = 'nsEditContextMenu';
      menuEl.className = 'ns-context-menu';
      menuEl.setAttribute('role', 'menu');
      menuEl.style.display = 'none';
      document.body.appendChild(menuEl);
      return menuEl;
    }

    function hideMenu() {
      if (menuEl) menuEl.style.display = 'none';
    }

    function isEditMode() {
      try { return !!(document.body && document.body.classList.contains('edit-mode')); } catch { return false; }
    }

    function inCanvasArea(target) {
      if (!target || !target.closest) return false;
      return !!(target.closest('.pixel-canvas') || target.closest('.canvas-viewport'));
    }

    function positionMenu(menu, clientX, clientY) {
      const pad = 8;
      menu.style.display = 'block';
      menu.style.visibility = 'hidden';
      menu.style.left = '0px';
      menu.style.top = '0px';
      const rect = menu.getBoundingClientRect();
      const vw = Math.max(document.documentElement.clientWidth || 0, window.innerWidth || 0);
      const vh = Math.max(document.documentElement.clientHeight || 0, window.innerHeight || 0);
      let x = Number(clientX) || 0;
      let y = Number(clientY) || 0;
      if (x + rect.width + pad > vw) x = Math.max(pad, vw - rect.width - pad);
      if (y + rect.height + pad > vh) y = Math.max(pad, vh - rect.height - pad);
      menu.style.left = x + 'px';
      menu.style.top = y + 'px';
      menu.style.visibility = '';
    }

    function buildMenuItems(hasWidgetTarget) {
      const sel = ctx.selection.getEditSelectionState();
      const entityCount = Number(sel.entityCount) || 0;
      const groupCount = Number(sel.groupCount) || 0;
      const hasSelection = entityCount > 0;
      const canAlign = entityCount >= 2;
      const canDistribute = entityCount >= 3;
      const canGroup = entityCount >= 2;
      const canUngroup = groupCount > 0 || (hasSelection && entityCount >= 1);

      const items = [
        { id: 'undo', label: '撤销', action: () => ctx.history.undoLayout() },
        { id: 'redo', label: '恢复', action: () => ctx.history.redoLayout() },
      ];

      if (hasSelection) {
        items.push('sep');
        items.push({ id: 'copy', label: '复制', action: () => ctx.history.copySelection() });
        items.push({ id: 'paste', label: '粘贴', action: () => ctx.history.pasteSelection() });
        items.push({ id: 'delete', label: '删除', action: () => ctx.selection.deleteSelected() });
      } else if (ctx.state.clipboard) {
        items.push('sep');
        items.push({ id: 'paste', label: '粘贴', action: () => ctx.history.pasteSelection() });
      }

      if (hasWidgetTarget || hasSelection) {
        items.push('sep');
        items.push({ id: 'alignLeft', label: '左对齐', disabled: !canAlign, action: () => ctx.selection.alignSelected('left') });
        items.push({ id: 'alignRight', label: '右对齐', disabled: !canAlign, action: () => ctx.selection.alignSelected('right') });
        items.push({ id: 'alignTop', label: '上对齐', disabled: !canAlign, action: () => ctx.selection.alignSelected('top') });
        items.push({ id: 'alignBottom', label: '下对齐', disabled: !canAlign, action: () => ctx.selection.alignSelected('bottom') });
        items.push({ id: 'alignCenterH', label: '水平居中', disabled: !canAlign, action: () => ctx.selection.alignSelected('hcenter') });
        items.push({ id: 'alignCenterV', label: '垂直居中', disabled: !canAlign, action: () => ctx.selection.alignSelected('vcenter') });
        items.push({ id: 'distributeH', label: '水平分布', disabled: !canDistribute, action: () => ctx.selection.distributeSelected('h') });
        items.push({ id: 'distributeV', label: '垂直分布', disabled: !canDistribute, action: () => ctx.selection.distributeSelected('v') });
        items.push({ id: 'sameSize', label: '统一尺寸', disabled: !canAlign, action: () => ctx.selection.sameSizeSelected() });
        items.push('sep');
        items.push({ id: 'group', label: '分组', disabled: !canGroup, action: () => ctx.selection.groupSelected() });
        items.push({ id: 'ungroup', label: '取消分组', disabled: !canUngroup, action: () => ctx.selection.ungroupSelected() });
      }

      return items;
    }

    function showMenu(clientX, clientY, items) {
      const menu = ensureMenu();
      menu.innerHTML = '';
      items.forEach(it => {
        if (it === 'sep') {
          const sep = document.createElement('div');
          sep.className = 'ns-context-menu-sep';
          sep.setAttribute('role', 'separator');
          menu.appendChild(sep);
          return;
        }
        const btn = document.createElement('button');
        btn.type = 'button';
        btn.className = 'ns-context-menu-item';
        btn.setAttribute('role', 'menuitem');
        btn.textContent = it.label;
        if (it.disabled) {
          btn.disabled = true;
          btn.classList.add('disabled');
        } else if (it.action) {
          btn.addEventListener('click', (e) => {
            e.preventDefault();
            e.stopPropagation();
            hideMenu();
            try { it.action(); } catch {}
          });
        }
        menu.appendChild(btn);
      });
      positionMenu(menu, clientX, clientY);
    }

    document.addEventListener('contextmenu', (e) => {
      try {
        if (!isEditMode()) return;
        if (!inCanvasArea(e.target)) return;
        e.preventDefault();
        e.stopPropagation();
        try { ctx.props.__closeEpColorPickers(); } catch {}

        const widget = e.target && e.target.closest ? e.target.closest('.grid-stack-item') : null;
        if (widget) {
          const inSelection = ctx.state.selectedNodes.has(widget)
            || !!(widget.classList && widget.classList.contains('grid-selected'));
          if (!inSelection) {
            ctx.selection.selectNode(widget);
          }
        }

        showMenu(e.clientX, e.clientY, buildMenuItems(!!widget));
      } catch {}
    }, true);

    document.addEventListener('pointerdown', (e) => {
      if (!menuEl || menuEl.style.display === 'none') return;
      if (menuEl.contains(e.target)) return;
      hideMenu();
    }, true);

    document.addEventListener('keydown', (e) => {
      if (!e || e.key !== 'Escape') return;
      hideMenu();
    }, true);

    window.addEventListener('resize', hideMenu);
    window.addEventListener('blur', hideMenu);
  }

  window.NSDashboardContextMenu = { init };
})();
