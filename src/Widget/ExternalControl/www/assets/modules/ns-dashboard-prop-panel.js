// ns-dashboard-prop-panel.js —— 属性面板入口（画布/分组 + 委托 NSPropSchema）
(function() {
  'use strict';

  function init(ctx) {
    function __initColorInput(inp, raw, fallbackHex) {
      try {
        const fb0 = String(fallbackHex || '#000000').trim();
        const fb = /^#[0-9a-fA-F]{6}$/.test(fb0) ? fb0 : '#000000';
        const s = (raw !== undefined && raw !== null) ? String(raw).trim() : '';
        const rawStr = s || fb;
        inp.dataset.colorRaw = rawStr;
        inp.value = fb;
      } catch {}
    }

    function __upgradeColorInputs(rootEl) {
      try {
        const root = rootEl || document;
        const VueG = (window && window.Vue) ? window.Vue : null;
        const EPG = (window && window.ElementPlus) ? window.ElementPlus : null;
        if (!VueG || !VueG.createApp || !EPG) return;
        const inputs = root && root.querySelectorAll ? Array.from(root.querySelectorAll('input[type="color"]')) : [];
        inputs.forEach(inp => {
          if (!inp || (inp.dataset && inp.dataset.alphaUpgraded === '1')) return;
          const parent = inp.parentElement;
          if (!parent) return;
          const disabled = !!inp.disabled;
          const raw = (inp.dataset && inp.dataset.colorRaw) ? String(inp.dataset.colorRaw || '').trim() : '';
          const baseCss = String(raw || inp.value || '').trim() || 'rgba(0,0,0,1)';
          inp.dataset.alphaUpgraded = '1';
          inp.type = 'hidden';
          try { inp.style.display = 'none'; } catch {}
          inp.value = baseCss;
          inp.dataset.colorRaw = baseCss;
          const ctl = document.createElement('div');
          ctl.className = 'd-flex align-items-center';
          ctl.style.gap = '8px';
          ctl.style.position = 'relative';
          const mountEl = document.createElement('div');
          mountEl.style.maxWidth = '260px';
          let mounted = false;
          const ensureMounted = () => {
            if (mounted) return;
            const initColor = String(inp.value || baseCss).trim() || baseCss;
            const app = VueG.createApp({
              data() { return { color: initColor, disabled: disabled }; },
              watch: { color(v) {
                const s = (v !== undefined && v !== null) ? String(v).trim() : '';
                if (!s) return;
                inp.value = s; inp.dataset.colorRaw = s;
                try { inp.dispatchEvent(new Event('input', { bubbles: true })); } catch {}
                try { inp.dispatchEvent(new Event('change', { bubbles: true })); } catch {}
              }},
              template: '<el-color-picker v-model="color" :disabled="disabled" teleported popper-class="ns-color-popper" show-alpha color-format="rgb" clearable />'
            });
            try { app.use(EPG); } catch {}
            app.mount(mountEl);
            mounted = true;
          };
          let replaced = false;
          try { parent.replaceChild(ctl, inp); replaced = true; } catch {}
          if (!replaced) return;
          if (disabled) { try { inp.disabled = true; } catch {} }
          ctl.appendChild(mountEl);
          ctl.appendChild(inp);
          ensureMounted();
        });
        try {
          if (window.NSA11y && typeof window.NSA11y.fixColorPickerLabels === 'function') {
            window.NSA11y.fixColorPickerLabels(root);
            setTimeout(() => { try { window.NSA11y.fixColorPickerLabels(root); } catch {} }, 0);
          }
        } catch {}
      } catch {}
    }

    function __closeEpColorPickers() {
      try { if (document && document.activeElement && document.activeElement.blur) document.activeElement.blur(); } catch {}
      try {
        const poppers = Array.from(document.querySelectorAll('.el-popper, .ns-color-popper'));
        poppers.forEach(p => {
          try {
            if (!p) return;
            const hasColor = !!(p.querySelector && (p.querySelector('.el-color-dropdown') || p.querySelector('.el-color-picker__panel') || p.querySelector('.el-color-svpanel')));
            if (!hasColor) return;
            try { p.style.display = 'none'; } catch {}
          } catch {}
        });
      } catch {}
    }

    function renderUnknownTypeFallback(form, type, multiCount) {
      if (multiCount > 1) {
        const multiRow = document.createElement('div');
        multiRow.className = 'ns-prop-section';
        multiRow.innerHTML =
          '<div class="ns-prop-section-body">' +
            '<div class="prop-label">多选提示</div>' +
            '<div class="prop-value text-muted">已选择 ' + multiCount + ' 个控件，属性面板仅显示最后选中控件</div>' +
          '</div>';
        form.appendChild(multiRow);
      }

      const typeRow = document.createElement('div');
      typeRow.className = 'ns-prop-section';
      typeRow.innerHTML =
        '<div class="ns-prop-section-body">' +
          '<div class="prop-label">类型</div>' +
          '<div class="prop-value text-muted">' + String(type || '未知') + '</div>' +
          '<div class="mt-2 text-muted" style="font-size:12px">该控件类型尚未配置属性 Schema（ns-prop-schema.js），无法编辑专用属性。</div>' +
        '</div>';
      form.appendChild(typeRow);
    }

    function updatePropPanel(node) {
      try { __closeEpColorPickers(); } catch {}
      const panel = document.getElementById('propPanel');
      if (!panel) return;
      panel.innerHTML = '';

      if (!node) {
        const gs = ctx.state.selectedGroupIds;
        const gCount = gs ? Number(gs.size || 0) : 0;
        if (gCount > 0) {
          const gids = Array.from(gs);
          const wrap = Array.from(document.querySelectorAll('#tabsContent [data-tab-id]')).find(el => el.style.display !== 'none') || null;
          const canvas = wrap ? wrap.querySelector('.pixel-canvas') : null;

          if (gids.length === 1) {
            const gid = String(gids[0] || '').trim();
            let memberCount = 0;
            try {
              if (canvas) memberCount = ctx.services.NSUtils.__getGroupMembers(canvas, gid).length;
            } catch {}
            panel.innerHTML = '<div class="text-muted">已选择分组</div>'
              + '<div class="mt-1">Group ID：<span class="text-muted">' + gid + '</span></div>'
              + '<div class="mt-1">成员数量：<span class="text-muted">' + String(memberCount || 0) + '</span></div>';
            return;
          }

          panel.innerHTML = '<div class="text-muted">已选择分组</div>'
            + '<div class="mt-1">分组数量：<span class="text-muted">' + String(gids.length) + '</span></div>'
            + '<div class="mt-2" style="max-height: 140px; overflow:auto;">'
            + gids.map(g => '<div class="text-muted">• ' + String(g) + '</div>').join('')
            + '</div>';
          return;
        }

        const info = ctx.services.NS.activeTabId ? ctx.services.NS.grids.get(ctx.services.NS.activeTabId) : null;
        const d = (info && info.design) ? info.design : { bgColor: EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc' };
        const canEdit = (function(){
          try { return !!ctx.edit.getGlobalEditMode(); } catch {}
          try { return !!(document.body && document.body.classList && document.body.classList.contains('edit-mode')); } catch {}
          return false;
        })();

        const form = document.createElement('div');

        const emptyHint = document.createElement('div');
        emptyHint.className = 'ns-prop-empty';
        emptyHint.innerHTML =
          '<div class="ns-prop-empty-title">未选择控件</div>' +
          '<div class="ns-prop-empty-desc">' +
            (canEdit
              ? '点击画布上的控件以编辑属性；也可在下方调整画布背景。'
              : '运行模式下仅可查看。切换到编辑模式后可修改属性。') +
          '</div>';
        form.appendChild(emptyHint);

        const boxWrap = document.createElement('div');
        boxWrap.className = 'ns-prop-section';
        boxWrap.innerHTML = '<div class="ns-prop-section-head" style="cursor:default"><span>画布设置</span></div>';
        const box = document.createElement('div');
        box.className = 'ns-prop-section-body';
        boxWrap.appendChild(box);

        const autoRow = document.createElement('div');
        autoRow.className = 'mb-2 text-muted';
        autoRow.style.fontSize = '12px';
        const curW = Math.floor(Number(d.width) || 320);
        const curH = Math.floor(Number(d.height) || 240);
        autoRow.textContent = '画布尺寸随控件自动调整（当前约 ' + curW + ' × ' + curH + ' px）';
        box.appendChild(autoRow);

        const bgRow = document.createElement('div');
        bgRow.className = 'mb-1 d-flex align-items-center justify-content-between';
        bgRow.innerHTML = '<div class="prop-label">背景颜色</div>';
        const bgInput = document.createElement('input');
        bgInput.type = 'color';
        bgInput.className = 'form-control form-control-color form-control-sm';
        __initColorInput(bgInput, (d && d.bgColor !== undefined && d.bgColor !== null) ? d.bgColor : (EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc'), '#f8fafc');
        if (!canEdit) bgInput.disabled = true;
        bgRow.appendChild(bgInput);
        box.appendChild(bgRow);
        try { if (window.NSA11y) window.NSA11y.wirePropRow(bgRow, bgInput, '背景颜色'); } catch {}

        if (!canEdit) {
          const hint = document.createElement('div');
          hint.className = 'mt-2 text-muted';
          hint.style.fontSize = '12px';
          hint.textContent = '切换到编辑模式后可修改画布属性';
          box.appendChild(hint);
        }

        form.appendChild(boxWrap);

        if (canEdit && info) {
          const applyCanvasProps = () => {
            const nextBg = String(bgInput.value || '').trim() || String(EPWidgets.layoutDefaults.canvasBgColor || '#f8fafc');
            ctx.services.NSCanvas.applyPageDesign(ctx.services.NS.activeTabId, { bgColor: nextBg });
            try { ctx.layout.saveLayoutLocal(ctx.services.NS.activeTabId, info.grid); } catch {}
          };
          bgInput.onchange = () => applyCanvasProps();
        }

        panel.appendChild(form);
        try { __upgradeColorInputs(panel); } catch {}
        return;
      }

      const type = node.dataset.type || '未知';
      const props = EPWidgets.getProps(node);
      const form = document.createElement('div');
      const multiCount = ctx.state.selectedNodes.size;

      if (window.NSPropSchema && NSPropSchema.supportsType(type) && typeof NSPropSchema.renderWidgetPanel === 'function') {
        const ok = NSPropSchema.renderWidgetPanel(form, {
          node,
          type,
          props,
          multiCount,
          initColorInput: __initColorInput,
          refreshPanel() { updatePropPanel(node); },
          onChange(patch) {
            EPWidgets.setProps(node, patch);
            try { if (ctx.services.NS.activeTabId) ctx.layout.saveLayoutLocal(ctx.services.NS.activeTabId, ctx.layout.getActiveGrid()); } catch {}
          }
        });
        if (ok) {
          panel.appendChild(form);
          try { __upgradeColorInputs(panel); } catch {}
          return;
        }
      }

      renderUnknownTypeFallback(form, type, multiCount);
      panel.appendChild(form);
    }

    // 兼容旧 API：属性写入已由 NSPropSchema.onChange 即时完成，此处不再读 DOM 批量 apply
    function applyProperties() {}

    return {
      __initColorInput,
      __upgradeColorInputs,
      __closeEpColorPickers,
      updatePropPanel,
      applyProperties
    };
  }

  window.NSDashboardPropPanel = { init };
})();
