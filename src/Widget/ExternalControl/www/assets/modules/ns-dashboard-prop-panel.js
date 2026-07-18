// ns-dashboard-prop-panel.js —— 属性面板渲染与应用
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
          const originalOnChange = inp.onchange;
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

      function updatePropPanel(node) {
        try { __closeEpColorPickers(); } catch {}
        const panel = document.getElementById('propPanel');
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

          const head = document.createElement('div');
          head.className = 'mb-2';
          head.innerHTML = '<div class="prop-label">画布属性</div><div class="prop-value text-muted">未选择控件</div>';
          form.appendChild(head);

          const box = document.createElement('div');
          box.className = 'mb-2 p-2 bg-light border rounded';
          box.innerHTML = '<div class="prop-label fw-bold mb-1">画布设置</div>';

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

          form.appendChild(box);

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

        if (window.NSPropSchema && NSPropSchema.supportsType(type) && typeof NSPropSchema.renderWidgetPanel === 'function') {
          const ok = NSPropSchema.renderWidgetPanel(form, {
            node,
            type,
            props,
            multiCount: ctx.state.selectedNodes.size,
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

        if (ctx.state.selectedNodes.size > 1) {
          const multiRow = document.createElement('div');
          multiRow.className = 'mb-2 p-2 bg-light border rounded';
          multiRow.innerHTML = '<div class="prop-label">提示</div><div class="prop-value">当前已选择 ' + ctx.state.selectedNodes.size + ' 个控件，属性面板仅显示最后选中控件</div>';
          form.appendChild(multiRow);
        }

        // 类型显示
        const typeRow = document.createElement('div');
        typeRow.className = 'mb-2';
        typeRow.innerHTML = '<div class="prop-label">类型</div><div class="prop-value text-muted">'+ type +'</div>';
        form.appendChild(typeRow);
        
        // 基础ID设置（时间线控件的 Command ID 用作“游标”绑定，其余控件为通用 Command ID；Frame 不需要）
        if (type !== 'Frame') {
          const idRow = document.createElement('div');
          idRow.className = 'mb-2 p-2 bg-light border rounded';
          idRow.innerHTML = '<div class="prop-label fw-bold mb-1">ID 设置</div>';

          const cmdIdDiv = document.createElement('div');
          cmdIdDiv.className = 'mb-1';
          cmdIdDiv.innerHTML = '<div class="prop-label">' + (type === '时间线' ? '游标 Command ID' : 'Command ID') + '</div>';
          const cmdIdInput = document.createElement('input');
          if (type === '时间线') cmdIdInput.id = 'propTimelineCommandId';
          cmdIdInput.className = 'form-control form-control-sm';
          cmdIdInput.value = props.commandId || '/cmd/demo';
          cmdIdInput.onchange = (e) => EPWidgets.setProps(node, { commandId: e.target.value });
          cmdIdDiv.appendChild(cmdIdInput);
          idRow.appendChild(cmdIdDiv);

          form.appendChild(idRow);
        }

        // 样式设置
        const styleRow = document.createElement('div');
        styleRow.className = 'mb-2 p-2 bg-light border rounded';
        styleRow.innerHTML = '<div class="prop-label fw-bold mb-1">样式设置</div>';
        
        const bgColorDiv = document.createElement('div');
        bgColorDiv.className = 'mb-1 d-flex align-items-center justify-content-between';
        bgColorDiv.innerHTML = '<div class="prop-label">背景色</div>';
        const bgColorInput = document.createElement('input');
        bgColorInput.type = 'color';
        bgColorInput.className = 'form-control form-control-color form-control-sm';
        __initColorInput(bgColorInput, (props.bgColor !== undefined && props.bgColor !== null) ? props.bgColor : 'transparent', '#ffffff');
        bgColorInput.onchange = (e) => {
             EPWidgets.setProps(node, { bgColor: e.target.value });
             try { node.querySelector('.grid-stack-item-content').style.backgroundColor = e.target.value; } catch {}
        };
        bgColorDiv.appendChild(bgColorInput);
        styleRow.appendChild(bgColorDiv);

        if (type !== 'Frame') {
          const fontSizeDiv = document.createElement('div');
          fontSizeDiv.className = 'mb-1';
          fontSizeDiv.innerHTML = '<div class="prop-label">字体大小 (px)</div>';
          const fontSizeInput = document.createElement('input');
          fontSizeInput.type = 'number';
          fontSizeInput.className = 'form-control form-control-sm';
          fontSizeInput.value = props.fontSize || '14';
          fontSizeInput.onchange = (e) => {
               EPWidgets.setProps(node, { fontSize: e.target.value });
               node.style.fontSize = e.target.value + 'px';
          };
          fontSizeDiv.appendChild(fontSizeInput);
          styleRow.appendChild(fontSizeDiv);
        }
        
        form.appendChild(styleRow);

        // 根据类型渲染特定属性
        if (type === '按钮') {
          const labelRow = document.createElement('div');
          labelRow.className = 'mb-2';
          labelRow.innerHTML = '<div class="prop-label">文本</div>';
          const labelInput = document.createElement('input');
          labelInput.id = 'propBtnLabel';
          labelInput.className = 'form-control form-control-sm';
          labelInput.value = props.label || '执行';
          labelInput.onchange = (e) => EPWidgets.setProps(node, { label: e.target.value });
          labelRow.appendChild(labelInput);
          form.appendChild(labelRow);

          // 按钮颜色
          const btnColorRow = document.createElement('div');
          btnColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          btnColorRow.innerHTML = '<div class="prop-label">按钮颜色</div>';
          const btnColorInput = document.createElement('input');
          btnColorInput.id = 'propBtnColor';
          btnColorInput.type = 'color';
          btnColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(btnColorInput, props.buttonColor || '#409EFF', '#409EFF');
          btnColorInput.onchange = (e) => EPWidgets.setProps(node, { buttonColor: e.target.value });
          btnColorRow.appendChild(btnColorInput);
          styleRow.appendChild(btnColorRow);

          // 按下颜色
          const activeColorRow = document.createElement('div');
          activeColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          activeColorRow.innerHTML = '<div class="prop-label">按下颜色</div>';
          const activeColorInput = document.createElement('input');
          activeColorInput.id = 'propBtnActiveColor';
          activeColorInput.type = 'color';
          activeColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(activeColorInput, props.activeColor || '#3a8ee6', '#3a8ee6');
          activeColorInput.onchange = (e) => EPWidgets.setProps(node, { activeColor: e.target.value });
          activeColorRow.appendChild(activeColorInput);
          styleRow.appendChild(activeColorRow);

          // 文字颜色
          const txtColorRow = document.createElement('div');
          txtColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          txtColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const txtColorInput = document.createElement('input');
          txtColorInput.id = 'propBtnTextColor';
          txtColorInput.type = 'color';
          txtColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(txtColorInput, props.textColor || '#ffffff', '#ffffff');
          txtColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          txtColorRow.appendChild(txtColorInput);
          styleRow.appendChild(txtColorRow);

          // 边框颜色
          const borderColorRow = document.createElement('div');
          borderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          borderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const borderColorInput = document.createElement('input');
          borderColorInput.id = 'propBtnBorderColor';
          borderColorInput.type = 'color';
          borderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(borderColorInput, props.borderColor || '#409EFF', '#409EFF');
          borderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          borderColorRow.appendChild(borderColorInput);
          styleRow.appendChild(borderColorRow);

          // 边框样式
          const borderStyleRow = document.createElement('div');
          borderStyleRow.className = 'mb-2';
          borderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const borderStyleSelect = document.createElement('select');
          borderStyleSelect.id = 'propBtnBorderStyle';
          borderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
              const opt = document.createElement('option');
              opt.value = s;
              opt.textContent = s;
              if (s === (props.borderStyle || 'none')) opt.selected = true;
              borderStyleSelect.appendChild(opt);
          });
          borderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          borderStyleRow.appendChild(borderStyleSelect);
          styleRow.appendChild(borderStyleRow);

        } else if (type === '超链接') {
          const labelRow = document.createElement('div');
          labelRow.className = 'mb-2';
          labelRow.innerHTML = '<div class="prop-label">文本</div>';
          const labelInput = document.createElement('input');
          labelInput.id = 'propLinkLabel';
          labelInput.className = 'form-control form-control-sm';
          labelInput.value = props.label || '打开链接';
          labelInput.onchange = (e) => EPWidgets.setProps(node, { label: e.target.value });
          labelRow.appendChild(labelInput);
          form.appendChild(labelRow);

          const hrefRow = document.createElement('div');
          hrefRow.className = 'mb-2';
          hrefRow.innerHTML = '<div class="prop-label">链接</div>';
          const hrefInput = document.createElement('input');
          hrefInput.id = 'propLinkHref';
          hrefInput.className = 'form-control form-control-sm';
          hrefInput.value = props.href || 'https://example.com';
          hrefInput.onchange = (e) => EPWidgets.setProps(node, { href: e.target.value });
          hrefRow.appendChild(hrefInput);
          form.appendChild(hrefRow);

          const targetRow = document.createElement('div');
          targetRow.className = 'mb-2 form-check form-switch';
          const targetInput = document.createElement('input');
          targetInput.id = 'propLinkTargetBlank';
          targetInput.type = 'checkbox';
          targetInput.className = 'form-check-input';
          targetInput.checked = (props.targetBlank !== false);
          targetInput.onchange = (e) => EPWidgets.setProps(node, { targetBlank: !!e.target.checked });
          const targetLbl = document.createElement('label');
          targetLbl.className = 'form-check-label ms-2';
          targetLbl.textContent = '新窗口打开';
          targetRow.appendChild(targetInput);
          targetRow.appendChild(targetLbl);
          form.appendChild(targetRow);

          const btnColorRow = document.createElement('div');
          btnColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          btnColorRow.innerHTML = '<div class="prop-label">按钮颜色</div>';
          const btnColorInput = document.createElement('input');
          btnColorInput.id = 'propLinkBtnColor';
          btnColorInput.type = 'color';
          btnColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(btnColorInput, props.buttonColor || '#409EFF', '#409EFF');
          btnColorInput.onchange = (e) => EPWidgets.setProps(node, { buttonColor: e.target.value });
          btnColorRow.appendChild(btnColorInput);
          styleRow.appendChild(btnColorRow);

          const activeColorRow = document.createElement('div');
          activeColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          activeColorRow.innerHTML = '<div class="prop-label">按下颜色</div>';
          const activeColorInput = document.createElement('input');
          activeColorInput.id = 'propLinkActiveColor';
          activeColorInput.type = 'color';
          activeColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(activeColorInput, props.activeColor || '#3a8ee6', '#3a8ee6');
          activeColorInput.onchange = (e) => EPWidgets.setProps(node, { activeColor: e.target.value });
          activeColorRow.appendChild(activeColorInput);
          styleRow.appendChild(activeColorRow);

          const txtColorRow = document.createElement('div');
          txtColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          txtColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const txtColorInput = document.createElement('input');
          txtColorInput.id = 'propLinkTextColor';
          txtColorInput.type = 'color';
          txtColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(txtColorInput, props.textColor || '#ffffff', '#ffffff');
          txtColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          txtColorRow.appendChild(txtColorInput);
          styleRow.appendChild(txtColorRow);

          const borderColorRow = document.createElement('div');
          borderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          borderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const borderColorInput = document.createElement('input');
          borderColorInput.id = 'propLinkBorderColor';
          borderColorInput.type = 'color';
          borderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(borderColorInput, props.borderColor || '#409EFF', '#409EFF');
          borderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          borderColorRow.appendChild(borderColorInput);
          styleRow.appendChild(borderColorRow);

          const borderStyleRow = document.createElement('div');
          borderStyleRow.className = 'mb-2';
          borderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const borderStyleSelect = document.createElement('select');
          borderStyleSelect.id = 'propLinkBorderStyle';
          borderStyleSelect.className = 'form-select form-select-sm';
          ['solid','dashed','dotted','double','none'].forEach(s=>{
            const opt = document.createElement('option');
            opt.value = s; opt.textContent = s; if (s === (props.borderStyle || 'none')) opt.selected = true; borderStyleSelect.appendChild(opt);
          });
          borderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          borderStyleRow.appendChild(borderStyleSelect);
          styleRow.appendChild(borderStyleRow);

        } else if (type === '滑块') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propSliderMin';
          minInput.type = 'number';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propSliderMax';
          maxInput.type = 'number';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 100;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propSliderValue';
          valInput.type = 'number';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          // 样式设置
          const sBarColorRow = document.createElement('div');
          sBarColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sBarColorRow.innerHTML = '<div class="prop-label">轨道颜色</div>';
          const sBarColorInput = document.createElement('input');
          sBarColorInput.id = 'propSliderBarColor';
          sBarColorInput.type = 'color';
          sBarColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sBarColorInput, props.barColor || '#e5e7eb', '#e5e7eb');
          sBarColorInput.onchange = (e) => EPWidgets.setProps(node, { barColor: e.target.value });
          sBarColorRow.appendChild(sBarColorInput);
          styleRow.appendChild(sBarColorRow);

          const sFillColorRow = document.createElement('div');
          sFillColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sFillColorRow.innerHTML = '<div class="prop-label">进度颜色</div>';
          const sFillColorInput = document.createElement('input');
          sFillColorInput.id = 'propSliderFillColor';
          sFillColorInput.type = 'color';
          sFillColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sFillColorInput, props.fillColor || '#2b6cb0', '#2b6cb0');
          sFillColorInput.onchange = (e) => EPWidgets.setProps(node, { fillColor: e.target.value });
          sFillColorRow.appendChild(sFillColorInput);
          styleRow.appendChild(sFillColorRow);

          const sValColorRow = document.createElement('div');
          sValColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sValColorRow.innerHTML = '<div class="prop-label">数值颜色</div>';
          const sValColorInput = document.createElement('input');
          sValColorInput.id = 'propSliderValueColor';
          sValColorInput.type = 'color';
          sValColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sValColorInput, props.valueColor || '#111827', '#111827');
          sValColorInput.onchange = (e) => EPWidgets.setProps(node, { valueColor: e.target.value });
          sValColorRow.appendChild(sValColorInput);
          styleRow.appendChild(sValColorRow);

          const sBorderColorRow = document.createElement('div');
          sBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const sBorderColorInput = document.createElement('input');
          sBorderColorInput.id = 'propSliderBorderColor';
          sBorderColorInput.type = 'color';
          sBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          sBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          sBorderColorRow.appendChild(sBorderColorInput);
          styleRow.appendChild(sBorderColorRow);

          const sBorderStyleRow = document.createElement('div');
          sBorderStyleRow.className = 'mb-2';
          sBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const sBorderStyleSelect = document.createElement('select');
          sBorderStyleSelect.id = 'propSliderBorderStyle';
          sBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            sBorderStyleSelect.appendChild(opt);
          });
          sBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          sBorderStyleRow.appendChild(sBorderStyleSelect);
          styleRow.appendChild(sBorderStyleRow);
        } else if (type === '浮点滑块') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propFloatSliderMin';
          minInput.type = 'number';
          minInput.step = '0.01';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propFloatSliderMax';
          maxInput.type = 'number';
          maxInput.step = '0.01';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 1;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const stepRow = document.createElement('div');
          stepRow.className = 'mb-2';
          stepRow.innerHTML = '<div class="prop-label">步进</div>';
          const stepInput = document.createElement('input');
          stepInput.id = 'propFloatSliderStep';
          stepInput.type = 'number';
          stepInput.step = '0.001';
          stepInput.className = 'form-control form-control-sm';
          stepInput.value = props.step ?? 0.01;
          stepRow.appendChild(stepInput);
          form.appendChild(stepRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propFloatSliderValue';
          valInput.type = 'number';
          valInput.step = '0.01';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          // 样式设置（浮点）
          const fsBarColorRow = document.createElement('div');
          fsBarColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fsBarColorRow.innerHTML = '<div class="prop-label">轨道颜色</div>';
          const fsBarColorInput = document.createElement('input');
          fsBarColorInput.id = 'propFloatSliderBarColor';
          fsBarColorInput.type = 'color';
          fsBarColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fsBarColorInput, props.barColor || '#e5e7eb', '#e5e7eb');
          fsBarColorInput.onchange = (e) => EPWidgets.setProps(node, { barColor: e.target.value });
          fsBarColorRow.appendChild(fsBarColorInput);
          styleRow.appendChild(fsBarColorRow);

          const fsFillColorRow = document.createElement('div');
          fsFillColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fsFillColorRow.innerHTML = '<div class="prop-label">进度颜色</div>';
          const fsFillColorInput = document.createElement('input');
          fsFillColorInput.id = 'propFloatSliderFillColor';
          fsFillColorInput.type = 'color';
          fsFillColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fsFillColorInput, props.fillColor || '#2b6cb0', '#2b6cb0');
          fsFillColorInput.onchange = (e) => EPWidgets.setProps(node, { fillColor: e.target.value });
          fsFillColorRow.appendChild(fsFillColorInput);
          styleRow.appendChild(fsFillColorRow);

          const fsValColorRow = document.createElement('div');
          fsValColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fsValColorRow.innerHTML = '<div class="prop-label">数值颜色</div>';
          const fsValColorInput = document.createElement('input');
          fsValColorInput.id = 'propFloatSliderValueColor';
          fsValColorInput.type = 'color';
          fsValColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fsValColorInput, props.valueColor || '#111827', '#111827');
          fsValColorInput.onchange = (e) => EPWidgets.setProps(node, { valueColor: e.target.value });
          fsValColorRow.appendChild(fsValColorInput);
          styleRow.appendChild(fsValColorRow);

          const fsBorderColorRow = document.createElement('div');
          fsBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          fsBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const fsBorderColorInput = document.createElement('input');
          fsBorderColorInput.id = 'propFloatSliderBorderColor';
          fsBorderColorInput.type = 'color';
          fsBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(fsBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          fsBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          fsBorderColorRow.appendChild(fsBorderColorInput);
          styleRow.appendChild(fsBorderColorRow);

          const fsBorderStyleRow = document.createElement('div');
          fsBorderStyleRow.className = 'mb-2';
          fsBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const fsBorderStyleSelect = document.createElement('select');
          fsBorderStyleSelect.id = 'propFloatSliderBorderStyle';
          fsBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            fsBorderStyleSelect.appendChild(opt);
          });
          fsBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          fsBorderStyleRow.appendChild(fsBorderStyleSelect);
          styleRow.appendChild(fsBorderStyleRow);
        } else if (type === '勾选') {
          const labelRow = document.createElement('div');
          labelRow.className = 'mb-2';
          labelRow.innerHTML = '<div class="prop-label">文本</div>';
          const labelInput = document.createElement('input');
          labelInput.id = 'propCheckLabel';
          labelInput.className = 'form-control form-control-sm';
          labelInput.value = props.label || '启用';
          labelRow.appendChild(labelInput);
          form.appendChild(labelRow);
          const chkRow = document.createElement('div');
          chkRow.className = 'mb-2';
          chkRow.innerHTML = '<div class="prop-label">选中</div>';
          const chkInput = document.createElement('input');
          chkInput.id = 'propCheckChecked';
          chkInput.type = 'checkbox';
          chkInput.className = 'form-check-input';
          chkInput.checked = !!props.checked;
          chkRow.appendChild(chkInput);
          form.appendChild(chkRow);
          const cActiveColorRow = document.createElement('div');
          cActiveColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          cActiveColorRow.innerHTML = '<div class="prop-label">选中颜色</div>';
          const cActiveColorInput = document.createElement('input');
          cActiveColorInput.id = 'propCheckActiveColor';
          cActiveColorInput.type = 'color';
          cActiveColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(cActiveColorInput, props.activeColor || '#409EFF', '#409EFF');
          cActiveColorRow.appendChild(cActiveColorInput);
          styleRow.appendChild(cActiveColorRow);
          const cTextColorRow = document.createElement('div');
          cTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          cTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const cTextColorInput = document.createElement('input');
          cTextColorInput.id = 'propCheckTextColor';
          cTextColorInput.type = 'color';
          cTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(cTextColorInput, props.textColor || '#111827', '#111827');
          cTextColorRow.appendChild(cTextColorInput);
          styleRow.appendChild(cTextColorRow);
          const cBorderColorRow = document.createElement('div');
          cBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          cBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const cBorderColorInput = document.createElement('input');
          cBorderColorInput.id = 'propCheckBorderColor';
          cBorderColorInput.type = 'color';
          cBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(cBorderColorInput, props.borderColor || '#409EFF', '#409EFF');
          cBorderColorRow.appendChild(cBorderColorInput);
          styleRow.appendChild(cBorderColorRow);
          const cBorderStyleRow = document.createElement('div');
          cBorderStyleRow.className = 'mb-2';
          cBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const cBorderStyleSelect = document.createElement('select');
          cBorderStyleSelect.id = 'propCheckBorderStyle';
          cBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            cBorderStyleSelect.appendChild(opt);
          });
          cBorderStyleRow.appendChild(cBorderStyleSelect);
          styleRow.appendChild(cBorderStyleRow);
        } else if (type === '开关') {
          const chkRow = document.createElement('div');
          chkRow.className = 'mb-2';
          chkRow.innerHTML = '<div class="prop-label">选中</div>';
          const chkInput = document.createElement('input');
          chkInput.id = 'propSwitchChecked';
          chkInput.type = 'checkbox';
          chkInput.className = 'form-check-input';
          chkInput.checked = !!props.checked;
          chkRow.appendChild(chkInput);
          form.appendChild(chkRow);
          const sOnColorRow = document.createElement('div');
          sOnColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sOnColorRow.innerHTML = '<div class="prop-label">开启颜色</div>';
          const sOnColorInput = document.createElement('input');
          sOnColorInput.id = 'propSwitchOnColor';
          sOnColorInput.type = 'color';
          sOnColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sOnColorInput, props.onColor || '#13ce66', '#13ce66');
          sOnColorRow.appendChild(sOnColorInput);
          styleRow.appendChild(sOnColorRow);
          const sOffColorRow = document.createElement('div');
          sOffColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sOffColorRow.innerHTML = '<div class="prop-label">关闭颜色</div>';
          const sOffColorInput = document.createElement('input');
          sOffColorInput.id = 'propSwitchOffColor';
          sOffColorInput.type = 'color';
          sOffColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sOffColorInput, props.offColor || '#ff4949', '#ff4949');
          sOffColorRow.appendChild(sOffColorInput);
          styleRow.appendChild(sOffColorRow);
          const sBorderColorRow = document.createElement('div');
          sBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          sBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const sBorderColorInput = document.createElement('input');
          sBorderColorInput.id = 'propSwitchBorderColor';
          sBorderColorInput.type = 'color';
          sBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(sBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          sBorderColorRow.appendChild(sBorderColorInput);
          styleRow.appendChild(sBorderColorRow);
          const sBorderStyleRow = document.createElement('div');
          sBorderStyleRow.className = 'mb-2';
          sBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const sBorderStyleSelect = document.createElement('select');
          sBorderStyleSelect.id = 'propSwitchBorderStyle';
          sBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            sBorderStyleSelect.appendChild(opt);
          });
          sBorderStyleRow.appendChild(sBorderStyleSelect);
          styleRow.appendChild(sBorderStyleRow);
        } else if (type === '输入框') {
          const phRow = document.createElement('div');
          phRow.className = 'mb-2';
          phRow.innerHTML = '<div class="prop-label">占位符</div>';
          const phInput = document.createElement('input');
          phInput.id = 'propInputPlaceholder';
          phInput.className = 'form-control form-control-sm';
          phInput.value = props.placeholder || '请输入';
          phRow.appendChild(phInput);
          form.appendChild(phRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propInputValue';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value || '';
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          const iTextColorRow = document.createElement('div');
          iTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          iTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const iTextColorInput = document.createElement('input');
          iTextColorInput.id = 'propInputTextColor';
          iTextColorInput.type = 'color';
          iTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(iTextColorInput, props.textColor || '#111827', '#111827');
          iTextColorRow.appendChild(iTextColorInput);
          styleRow.appendChild(iTextColorRow);
          const iBorderColorRow = document.createElement('div');
          iBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          iBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const iBorderColorInput = document.createElement('input');
          iBorderColorInput.id = 'propInputBorderColor';
          iBorderColorInput.type = 'color';
          iBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(iBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          iBorderColorRow.appendChild(iBorderColorInput);
          styleRow.appendChild(iBorderColorRow);
          const iBorderStyleRow = document.createElement('div');
          iBorderStyleRow.className = 'mb-2';
          iBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const iBorderStyleSelect = document.createElement('select');
          iBorderStyleSelect.id = 'propInputBorderStyle';
          iBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            iBorderStyleSelect.appendChild(opt);
          });
          iBorderStyleRow.appendChild(iBorderStyleSelect);
          styleRow.appendChild(iBorderStyleRow);
        } else if (type === '数值') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propNumberMin';
          minInput.type = 'number';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propNumberMax';
          maxInput.type = 'number';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 100;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const stepRow = document.createElement('div');
          stepRow.className = 'mb-2';
          stepRow.innerHTML = '<div class="prop-label">步进</div>';
          const stepInput = document.createElement('input');
          stepInput.id = 'propNumberStep';
          stepInput.type = 'number';
          stepInput.className = 'form-control form-control-sm';
          stepInput.step = '0.001';
          stepInput.value = props.step ?? 1;
          stepRow.appendChild(stepInput);
          form.appendChild(stepRow);
          const precRow = document.createElement('div');
          precRow.className = 'mb-2';
          precRow.innerHTML = '<div class="prop-label">精度 (小数位)</div>';
          const precInput = document.createElement('input');
          precInput.id = 'propNumberPrecision';
          precInput.type = 'number';
          precInput.className = 'form-control form-control-sm';
          precInput.min = '0';
          precInput.step = '1';
          precInput.value = props.precision ?? 0;
          precRow.appendChild(precInput);
          form.appendChild(precRow);
          const valRow2 = document.createElement('div');
          valRow2.className = 'mb-2';
          valRow2.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput2 = document.createElement('input');
          valInput2.id = 'propNumberValue';
          valInput2.type = 'number';
          valInput2.className = 'form-control form-control-sm';
          valInput2.value = props.value ?? 0;
          valRow2.appendChild(valInput2);
          form.appendChild(valRow2);
          const roRow = document.createElement('div');
          roRow.className = 'mb-2';
          roRow.innerHTML = '<div class="prop-label">只读</div>';
          const roInput = document.createElement('input');
          roInput.id = 'propNumberReadOnly';
          roInput.type = 'checkbox';
          roInput.className = 'form-check-input';
          roInput.checked = !!props.readOnly;
          roRow.appendChild(roInput);
          form.appendChild(roRow);
          // 样式设置
          const nTextColorRow = document.createElement('div');
          nTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          nTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const nTextColorInput = document.createElement('input');
          nTextColorInput.id = 'propNumberTextColor';
          nTextColorInput.type = 'color';
          nTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(nTextColorInput, props.textColor || '#111827', '#111827');
          nTextColorRow.appendChild(nTextColorInput);
          styleRow.appendChild(nTextColorRow);
          const nBorderColorRow = document.createElement('div');
          nBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          nBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const nBorderColorInput = document.createElement('input');
          nBorderColorInput.id = 'propNumberBorderColor';
          nBorderColorInput.type = 'color';
          nBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(nBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          nBorderColorRow.appendChild(nBorderColorInput);
          styleRow.appendChild(nBorderColorRow);
          const nBorderStyleRow = document.createElement('div');
          nBorderStyleRow.className = 'mb-2';
          nBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const nBorderStyleSelect = document.createElement('select');
          nBorderStyleSelect.id = 'propNumberBorderStyle';
          nBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            nBorderStyleSelect.appendChild(opt);
          });
          nBorderStyleRow.appendChild(nBorderStyleSelect);
          styleRow.appendChild(nBorderStyleRow);
          // 事件绑定
          minInput.onchange = (e) => EPWidgets.setProps(node, { min: Number(e.target.value) });
          maxInput.onchange = (e) => EPWidgets.setProps(node, { max: Number(e.target.value) });
          stepInput.onchange = (e) => EPWidgets.setProps(node, { step: Number(e.target.value) });
          precInput.onchange = (e) => EPWidgets.setProps(node, { precision: Math.max(0, Math.floor(Number(e.target.value)||0)) });
          valInput2.onchange = (e) => EPWidgets.setProps(node, { value: Number(e.target.value) });
          roInput.onchange = (e) => EPWidgets.setProps(node, { readOnly: e.target.checked });
          nTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          nBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          nBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
        } else if (type === '分割线') {
          const textRow = document.createElement('div');
          textRow.className = 'mb-2';
          textRow.innerHTML = '<div class="prop-label">文本</div>';
          const textInput = document.createElement('input');
          textInput.id = 'propDividerText';
          textInput.className = 'form-control form-control-sm';
          textInput.value = props.text || '分割线';
          textRow.appendChild(textInput);
          form.appendChild(textRow);
          const dTextColorRow = document.createElement('div');
          dTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          dTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const dTextColorInput = document.createElement('input');
          dTextColorInput.id = 'propDividerTextColor';
          dTextColorInput.type = 'color';
          dTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(dTextColorInput, props.textColor || '#64748b', '#64748b');
          dTextColorRow.appendChild(dTextColorInput);
          styleRow.appendChild(dTextColorRow);
          const dLineColorRow = document.createElement('div');
          dLineColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          dLineColorRow.innerHTML = '<div class="prop-label">线条颜色</div>';
          const dLineColorInput = document.createElement('input');
          dLineColorInput.id = 'propDividerLineColor';
          dLineColorInput.type = 'color';
          dLineColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(dLineColorInput, props.lineColor || '#e5e7eb', '#e5e7eb');
          dLineColorRow.appendChild(dLineColorInput);
          styleRow.appendChild(dLineColorRow);
          const dLineWidthRow = document.createElement('div');
          dLineWidthRow.className = 'mb-2';
          dLineWidthRow.innerHTML = '<div class="prop-label">线宽 (px)</div>';
          const dLineWidthInput = document.createElement('input');
          dLineWidthInput.id = 'propDividerLineWidth';
          dLineWidthInput.type = 'number';
          dLineWidthInput.className = 'form-control form-control-sm';
          dLineWidthInput.value = props.lineWidth || 2;
          dLineWidthRow.appendChild(dLineWidthInput);
          styleRow.appendChild(dLineWidthRow);
          const dBorderStyleRow = document.createElement('div');
          dBorderStyleRow.className = 'mb-2';
          dBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const dBorderStyleSelect = document.createElement('select');
          dBorderStyleSelect.id = 'propDividerBorderStyle';
          dBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'solid')) opt.selected = true;
            dBorderStyleSelect.appendChild(opt);
          });
          dBorderStyleRow.appendChild(dBorderStyleSelect);
          styleRow.appendChild(dBorderStyleRow);
        } else if (type === '竖向分割线') {
          const vLineColorRow = document.createElement('div');
          vLineColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vLineColorRow.innerHTML = '<div class="prop-label">线条颜色</div>';
          const vLineColorInput = document.createElement('input');
          vLineColorInput.id = 'propVDividerLineColor';
          vLineColorInput.type = 'color';
          vLineColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vLineColorInput, props.lineColor || '#e5e7eb', '#e5e7eb');
          vLineColorRow.appendChild(vLineColorInput);
          styleRow.appendChild(vLineColorRow);
          const vLineWidthRow = document.createElement('div');
          vLineWidthRow.className = 'mb-2';
          vLineWidthRow.innerHTML = '<div class="prop-label">线宽 (px)</div>';
          const vLineWidthInput = document.createElement('input');
          vLineWidthInput.id = 'propVDividerLineWidth';
          vLineWidthInput.type = 'number';
          vLineWidthInput.className = 'form-control form-control-sm';
          vLineWidthInput.value = props.lineWidth || 2;
          vLineWidthRow.appendChild(vLineWidthInput);
          styleRow.appendChild(vLineWidthRow);
          const vBorderStyleRow = document.createElement('div');
          vBorderStyleRow.className = 'mb-2';
          vBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const vBorderStyleSelect = document.createElement('select');
          vBorderStyleSelect.id = 'propVDividerBorderStyle';
          vBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            vBorderStyleSelect.appendChild(opt);
          });
          vBorderStyleRow.appendChild(vBorderStyleSelect);
          styleRow.appendChild(vBorderStyleRow);
        } else if (type === '标签') {
          const textRow = document.createElement('div');
          textRow.className = 'mb-2';
          textRow.innerHTML = '<div class="prop-label">文本</div>';
          const textInput = document.createElement('input');
          textInput.id = 'propLabelText';
          textInput.className = 'form-control form-control-sm';
          textInput.value = props.text || '标签';
          textRow.appendChild(textInput);
          form.appendChild(textRow);
          
          // 样式设置：文字颜色
          const lblTextColorRow = document.createElement('div');
          lblTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          lblTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const lblTextColorInput = document.createElement('input');
          lblTextColorInput.id = 'propLabelTextColor';
          lblTextColorInput.type = 'color';
          lblTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(lblTextColorInput, props.textColor || '#111827', '#111827');
          lblTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          lblTextColorRow.appendChild(lblTextColorInput);
          styleRow.appendChild(lblTextColorRow);

          // 样式设置：边框颜色
          const lblBorderColorRow = document.createElement('div');
          lblBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          lblBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const lblBorderColorInput = document.createElement('input');
          lblBorderColorInput.id = 'propLabelBorderColor';
          lblBorderColorInput.type = 'color';
          lblBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(lblBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          lblBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          lblBorderColorRow.appendChild(lblBorderColorInput);
          styleRow.appendChild(lblBorderColorRow);

          // 样式设置：边框样式
          const lblBorderStyleRow = document.createElement('div');
          lblBorderStyleRow.className = 'mb-2';
          lblBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const lblBorderStyleSelect = document.createElement('select');
          lblBorderStyleSelect.id = 'propLabelBorderStyle';
          lblBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            lblBorderStyleSelect.appendChild(opt);
          });
          lblBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          lblBorderStyleRow.appendChild(lblBorderStyleSelect);
          styleRow.appendChild(lblBorderStyleRow);
        } else if (type === '竖向滑动条') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propVSliderMin';
          minInput.type = 'number';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propVSliderMax';
          maxInput.type = 'number';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 100;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propVSliderValue';
          valInput.type = 'number';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          const heightRow = document.createElement('div');
          heightRow.className = 'mb-2';
          heightRow.innerHTML = '<div class="prop-label">高度</div>';
          const heightInput = document.createElement('input');
          heightInput.id = 'propVSliderHeight';
          heightInput.type = 'text';
          heightInput.placeholder = '例如 140px';
          heightInput.className = 'form-control form-control-sm';
          heightInput.value = props.height || '100%';
          heightRow.appendChild(heightInput);
          form.appendChild(heightRow);
          const vsBarColorRow = document.createElement('div');
          vsBarColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vsBarColorRow.innerHTML = '<div class="prop-label">轨道颜色</div>';
          const vsBarColorInput = document.createElement('input');
          vsBarColorInput.id = 'propVSliderBarColor';
          vsBarColorInput.type = 'color';
          vsBarColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vsBarColorInput, props.barColor || '#e5e7eb', '#e5e7eb');
          vsBarColorRow.appendChild(vsBarColorInput);
          styleRow.appendChild(vsBarColorRow);
          const vsFillColorRow = document.createElement('div');
          vsFillColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vsFillColorRow.innerHTML = '<div class="prop-label">进度颜色</div>';
          const vsFillColorInput = document.createElement('input');
          vsFillColorInput.id = 'propVSliderFillColor';
          vsFillColorInput.type = 'color';
          vsFillColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vsFillColorInput, props.fillColor || '#2b6cb0', '#2b6cb0');
          vsFillColorRow.appendChild(vsFillColorInput);
          styleRow.appendChild(vsFillColorRow);
          const vsValColorRow = document.createElement('div');
          vsValColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vsValColorRow.innerHTML = '<div class="prop-label">数值颜色</div>';
          const vsValColorInput = document.createElement('input');
          vsValColorInput.id = 'propVSliderValueColor';
          vsValColorInput.type = 'color';
          vsValColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vsValColorInput, props.valueColor || '#111827', '#111827');
          vsValColorRow.appendChild(vsValColorInput);
          styleRow.appendChild(vsValColorRow);
          const vsBorderColorRow = document.createElement('div');
          vsBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vsBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const vsBorderColorInput = document.createElement('input');
          vsBorderColorInput.id = 'propVSliderBorderColor';
          vsBorderColorInput.type = 'color';
          vsBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vsBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          vsBorderColorRow.appendChild(vsBorderColorInput);
          styleRow.appendChild(vsBorderColorRow);
          const vsBorderStyleRow = document.createElement('div');
          vsBorderStyleRow.className = 'mb-2';
          vsBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const vsBorderStyleSelect = document.createElement('select');
          vsBorderStyleSelect.id = 'propVSliderBorderStyle';
          vsBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            vsBorderStyleSelect.appendChild(opt);
          });
          vsBorderStyleRow.appendChild(vsBorderStyleSelect);
          styleRow.appendChild(vsBorderStyleRow);
        } else if (type === '竖向浮点滑块') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propVFloatSliderMin';
          minInput.type = 'number';
          minInput.step = '0.01';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propVFloatSliderMax';
          maxInput.type = 'number';
          maxInput.step = '0.01';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 1;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const stepRow = document.createElement('div');
          stepRow.className = 'mb-2';
          stepRow.innerHTML = '<div class="prop-label">步进</div>';
          const stepInput = document.createElement('input');
          stepInput.id = 'propVFloatSliderStep';
          stepInput.type = 'number';
          stepInput.step = '0.001';
          stepInput.className = 'form-control form-control-sm';
          stepInput.value = props.step ?? 0.01;
          stepRow.appendChild(stepInput);
          form.appendChild(stepRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propVFloatSliderValue';
          valInput.type = 'number';
          valInput.step = '0.01';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          const heightRow = document.createElement('div');
          heightRow.className = 'mb-2';
          heightRow.innerHTML = '<div class="prop-label">高度</div>';
          const heightInput = document.createElement('input');
          heightInput.id = 'propVFloatSliderHeight';
          heightInput.type = 'text';
          heightInput.placeholder = '例如 140px';
          heightInput.className = 'form-control form-control-sm';
          heightInput.value = props.height || '100%';
          heightRow.appendChild(heightInput);
          form.appendChild(heightRow);
          const vfsBarColorRow = document.createElement('div');
          vfsBarColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vfsBarColorRow.innerHTML = '<div class="prop-label">轨道颜色</div>';
          const vfsBarColorInput = document.createElement('input');
          vfsBarColorInput.id = 'propVFloatSliderBarColor';
          vfsBarColorInput.type = 'color';
          vfsBarColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vfsBarColorInput, props.barColor || '#e5e7eb', '#e5e7eb');
          vfsBarColorRow.appendChild(vfsBarColorInput);
          styleRow.appendChild(vfsBarColorRow);
          const vfsFillColorRow = document.createElement('div');
          vfsFillColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vfsFillColorRow.innerHTML = '<div class="prop-label">进度颜色</div>';
          const vfsFillColorInput = document.createElement('input');
          vfsFillColorInput.id = 'propVFloatSliderFillColor';
          vfsFillColorInput.type = 'color';
          vfsFillColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vfsFillColorInput, props.fillColor || '#2b6cb0', '#2b6cb0');
          vfsFillColorRow.appendChild(vfsFillColorInput);
          styleRow.appendChild(vfsFillColorRow);
          const vfsValColorRow = document.createElement('div');
          vfsValColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vfsValColorRow.innerHTML = '<div class="prop-label">数值颜色</div>';
          const vfsValColorInput = document.createElement('input');
          vfsValColorInput.id = 'propVFloatSliderValueColor';
          vfsValColorInput.type = 'color';
          vfsValColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vfsValColorInput, props.valueColor || '#111827', '#111827');
          vfsValColorRow.appendChild(vfsValColorInput);
          styleRow.appendChild(vfsValColorRow);
          const vfsBorderColorRow = document.createElement('div');
          vfsBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          vfsBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const vfsBorderColorInput = document.createElement('input');
          vfsBorderColorInput.id = 'propVFloatSliderBorderColor';
          vfsBorderColorInput.type = 'color';
          vfsBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(vfsBorderColorInput, props.borderColor || '#e5e7eb', '#e5e7eb');
          vfsBorderColorRow.appendChild(vfsBorderColorInput);
          styleRow.appendChild(vfsBorderColorRow);
          const vfsBorderStyleRow = document.createElement('div');
          vfsBorderStyleRow.className = 'mb-2';
          vfsBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const vfsBorderStyleSelect = document.createElement('select');
          vfsBorderStyleSelect.id = 'propVFloatSliderBorderStyle';
          vfsBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            vfsBorderStyleSelect.appendChild(opt);
          });
          vfsBorderStyleRow.appendChild(vfsBorderStyleSelect);
          styleRow.appendChild(vfsBorderStyleRow);
        } else if (type === '旋钮') {
          const minRow = document.createElement('div');
          minRow.className = 'mb-2';
          minRow.innerHTML = '<div class="prop-label">最小值</div>';
          const minInput = document.createElement('input');
          minInput.id = 'propKnobMin';
          minInput.type = 'number';
          minInput.className = 'form-control form-control-sm';
          minInput.value = props.min ?? 0;
          minRow.appendChild(minInput);
          form.appendChild(minRow);
          const maxRow = document.createElement('div');
          maxRow.className = 'mb-2';
          maxRow.innerHTML = '<div class="prop-label">最大值</div>';
          const maxInput = document.createElement('input');
          maxInput.id = 'propKnobMax';
          maxInput.type = 'number';
          maxInput.className = 'form-control form-control-sm';
          maxInput.value = props.max ?? 100;
          maxRow.appendChild(maxInput);
          form.appendChild(maxRow);
          const valRow = document.createElement('div');
          valRow.className = 'mb-2';
          valRow.innerHTML = '<div class="prop-label">当前值</div>';
          const valInput = document.createElement('input');
          valInput.id = 'propKnobValue';
          valInput.type = 'number';
          valInput.className = 'form-control form-control-sm';
          valInput.value = props.value ?? 0;
          valRow.appendChild(valInput);
          form.appendChild(valRow);
          
          // 新增：颜色和步长设置
          const stepRow = document.createElement('div');
          stepRow.className = 'mb-2';
          stepRow.innerHTML = '<div class="prop-label">步长</div>';
          const stepInput = document.createElement('input');
          stepInput.id = 'propKnobStep';
          stepInput.type = 'number';
          stepInput.className = 'form-control form-control-sm';
          stepInput.value = props.step ?? 1;
          stepRow.appendChild(stepInput);
          form.appendChild(stepRow);
          
          const colorRow = document.createElement('div');
          colorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          colorRow.innerHTML = '<div class="prop-label">值颜色</div>';
          const colorInput = document.createElement('input');
          colorInput.id = 'propKnobValueColor';
          colorInput.type = 'color';
          colorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(colorInput, props.valueColor || '#3B82F6', '#3B82F6');
          colorRow.appendChild(colorInput);
          form.appendChild(colorRow);
          
          const kTextColorRow = document.createElement('div');
          kTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          kTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const kTextColorInput = document.createElement('input');
          kTextColorInput.id = 'propKnobTextColor';
          kTextColorInput.type = 'color';
          kTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(kTextColorInput, props.textColor || '#495057', '#495057');
          // 函数级注释：实时应用旋钮文字颜色
          kTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          kTextColorRow.appendChild(kTextColorInput);
          styleRow.appendChild(kTextColorRow);

          const kBorderColorRow = document.createElement('div');
          kBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          kBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const kBorderColorInput = document.createElement('input');
          kBorderColorInput.id = 'propKnobBorderColor';
          kBorderColorInput.type = 'color';
          kBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(kBorderColorInput, props.borderColor || '#dfe7ef', '#dfe7ef');
          // 函数级注释：实时应用旋钮边框颜色
          kBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          kBorderColorRow.appendChild(kBorderColorInput);
          styleRow.appendChild(kBorderColorRow);

          const kBorderStyleRow = document.createElement('div');
          kBorderStyleRow.className = 'mb-2';
          kBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const kBorderStyleSelect = document.createElement('select');
          kBorderStyleSelect.id = 'propKnobBorderStyle';
          kBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            kBorderStyleSelect.appendChild(opt);
          });
          // 函数级注释：实时应用旋钮边框样式
          kBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          kBorderStyleRow.appendChild(kBorderStyleSelect);
          styleRow.appendChild(kBorderStyleRow);

          const kRangeColorRow = document.createElement('div');
          kRangeColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          kRangeColorRow.innerHTML = '<div class="prop-label">范围颜色</div>';
          const kRangeColorInput = document.createElement('input');
          kRangeColorInput.id = 'propKnobRangeColor';
          kRangeColorInput.type = 'color';
          kRangeColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(kRangeColorInput, props.rangeColor || '#dfe7ef', '#dfe7ef');
          // 函数级注释：实时应用旋钮范围颜色
          kRangeColorInput.onchange = (e) => EPWidgets.setProps(node, { rangeColor: e.target.value });
          kRangeColorRow.appendChild(kRangeColorInput);
          styleRow.appendChild(kRangeColorRow);

          const kStrokeWidthRow = document.createElement('div');
          kStrokeWidthRow.className = 'mb-2';
          kStrokeWidthRow.innerHTML = '<div class="prop-label">描边宽度</div>';
          const kStrokeWidthInput = document.createElement('input');
          kStrokeWidthInput.id = 'propKnobStrokeWidth';
          kStrokeWidthInput.type = 'number';
          kStrokeWidthInput.className = 'form-control form-control-sm';
          kStrokeWidthInput.value = props.strokeWidth || 14;
          // 函数级注释：实时应用旋钮描边宽度
          kStrokeWidthInput.onchange = (e) => EPWidgets.setProps(node, { strokeWidth: Number(e.target.value) });
          kStrokeWidthRow.appendChild(kStrokeWidthInput);
          styleRow.appendChild(kStrokeWidthRow);

          const kShowValueRow = document.createElement('div');
          kShowValueRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          kShowValueRow.innerHTML = '<div class="prop-label">显示数值</div>';
          const kShowValueInput = document.createElement('input');
          kShowValueInput.id = 'propKnobShowValue';
          kShowValueInput.type = 'checkbox';
          kShowValueInput.className = 'form-check-input';
          kShowValueInput.checked = !!props.showValue;
          // 函数级注释：实时切换旋钮显示数值
          kShowValueInput.onchange = (e) => EPWidgets.setProps(node, { showValue: !!e.target.checked });
          kShowValueRow.appendChild(kShowValueInput);
          styleRow.appendChild(kShowValueRow);

        } else if (type === '切换按钮') {
          const onRow = document.createElement('div');
          onRow.className = 'mb-2';
          onRow.innerHTML = '<div class="prop-label">开启文本</div>';
          const onInput = document.createElement('input');
          onInput.id = 'propToggleLabelOn';
          onInput.className = 'form-control form-control-sm';
          onInput.value = props.labelOn || '开启';
          onRow.appendChild(onInput);
          form.appendChild(onRow);
          const offRow = document.createElement('div');
          offRow.className = 'mb-2';
          offRow.innerHTML = '<div class="prop-label">关闭文本</div>';
          const offInput = document.createElement('input');
          offInput.id = 'propToggleLabelOff';
          offInput.className = 'form-control form-control-sm';
          offInput.value = props.labelOff || '关闭';
          offRow.appendChild(offInput);
          form.appendChild(offRow);
          const activeRow = document.createElement('div');
          activeRow.className = 'mb-2';
          activeRow.innerHTML = '<div class="prop-label">激活</div>';
          const activeInput = document.createElement('input');
          activeInput.id = 'propToggleActive';
          activeInput.type = 'checkbox';
          activeInput.className = 'form-check-input';
          activeInput.checked = !!props.active;
          activeRow.appendChild(activeInput);
          form.appendChild(activeRow);
          
          // 样式设置：按钮颜色（未激活）
          const tBtnColorRow = document.createElement('div');
          tBtnColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tBtnColorRow.innerHTML = '<div class="prop-label">按钮颜色</div>';
          const tBtnColorInput = document.createElement('input');
          tBtnColorInput.id = 'propToggleBtnColor';
          tBtnColorInput.type = 'color';
          tBtnColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tBtnColorInput, props.buttonColor || '#409EFF', '#409EFF');
          tBtnColorInput.onchange = (e) => EPWidgets.setProps(node, { buttonColor: e.target.value });
          tBtnColorRow.appendChild(tBtnColorInput);
          styleRow.appendChild(tBtnColorRow);

          // 样式设置：按下颜色
          const tPressColorRow = document.createElement('div');
          tPressColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tPressColorRow.innerHTML = '<div class="prop-label">按下颜色</div>';
          const tPressColorInput = document.createElement('input');
          tPressColorInput.id = 'propTogglePressColor';
          tPressColorInput.type = 'color';
          tPressColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tPressColorInput, props.pressColor || '#2a7bd8', '#2a7bd8');
          tPressColorInput.onchange = (e) => EPWidgets.setProps(node, { pressColor: e.target.value });
          tPressColorRow.appendChild(tPressColorInput);
          styleRow.appendChild(tPressColorRow);

          // 样式设置：激活颜色（切换后）
          const tActiveColorRow = document.createElement('div');
          tActiveColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tActiveColorRow.innerHTML = '<div class="prop-label">激活颜色</div>';
          const tActiveColorInput = document.createElement('input');
          tActiveColorInput.id = 'propToggleActiveColor';
          tActiveColorInput.type = 'color';
          tActiveColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tActiveColorInput, props.activeColor || '#3a8ee6', '#3a8ee6');
          tActiveColorInput.onchange = (e) => EPWidgets.setProps(node, { activeColor: e.target.value });
          tActiveColorRow.appendChild(tActiveColorInput);
          styleRow.appendChild(tActiveColorRow);

          // 样式设置：文字颜色
          const tTextColorRow = document.createElement('div');
          tTextColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tTextColorRow.innerHTML = '<div class="prop-label">文字颜色</div>';
          const tTextColorInput = document.createElement('input');
          tTextColorInput.id = 'propToggleTextColor';
          tTextColorInput.type = 'color';
          tTextColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tTextColorInput, props.textColor || '#ffffff', '#ffffff');
          tTextColorInput.onchange = (e) => EPWidgets.setProps(node, { textColor: e.target.value });
          tTextColorRow.appendChild(tTextColorInput);
          styleRow.appendChild(tTextColorRow);

          // 样式设置：边框颜色
          const tBorderColorRow = document.createElement('div');
          tBorderColorRow.className = 'mb-2 d-flex align-items-center justify-content-between';
          tBorderColorRow.innerHTML = '<div class="prop-label">边框颜色</div>';
          const tBorderColorInput = document.createElement('input');
          tBorderColorInput.id = 'propToggleBorderColor';
          tBorderColorInput.type = 'color';
          tBorderColorInput.className = 'form-control form-control-color form-control-sm';
          __initColorInput(tBorderColorInput, props.borderColor || '#409EFF', '#409EFF');
          tBorderColorInput.onchange = (e) => EPWidgets.setProps(node, { borderColor: e.target.value });
          tBorderColorRow.appendChild(tBorderColorInput);
          styleRow.appendChild(tBorderColorRow);

          // 样式设置：边框样式
          const tBorderStyleRow = document.createElement('div');
          tBorderStyleRow.className = 'mb-2';
          tBorderStyleRow.innerHTML = '<div class="prop-label">边框样式</div>';
          const tBorderStyleSelect = document.createElement('select');
          tBorderStyleSelect.id = 'propToggleBorderStyle';
          tBorderStyleSelect.className = 'form-select form-select-sm';
          ['solid', 'dashed', 'dotted', 'double', 'none'].forEach(s => {
            const opt = document.createElement('option');
            opt.value = s;
            opt.textContent = s;
            if (s === (props.borderStyle || 'none')) opt.selected = true;
            tBorderStyleSelect.appendChild(opt);
          });
          tBorderStyleSelect.onchange = (e) => EPWidgets.setProps(node, { borderStyle: e.target.value });
          tBorderStyleRow.appendChild(tBorderStyleSelect);
          styleRow.appendChild(tBorderStyleRow);
        }
        panel.appendChild(form);
        try { __upgradeColorInputs(panel); } catch {}

        let __autoApplyTimer = null;
        const __autoApply = () => {
          if (__autoApplyTimer) clearTimeout(__autoApplyTimer);
          __autoApplyTimer = setTimeout(() => {
            try { applyProperties({ noRefresh: true }); } catch {}
          }, 150);
        };
        try { form.addEventListener('change', __autoApply, true); } catch {}
        try { form.addEventListener('input', __autoApply, true); } catch {}
      }

      // 函数级注释：应用属性修改到选中控件
      function applyProperties(opts) {
        if (!ctx.state.currentSelected) return;
        const type = ctx.state.currentSelected.dataset.type;
        if (window.NSPropSchema && NSPropSchema.supportsType(type)) return;
        const props = {};
        const titleEl = document.getElementById('propTitle');
        if (titleEl) props.title = titleEl.value.trim();
        if (type === '按钮') {
          props.label = document.getElementById('propBtnLabel').value.trim();
          props.buttonColor = document.getElementById('propBtnColor').value;
          props.activeColor = document.getElementById('propBtnActiveColor').value;
          props.textColor = document.getElementById('propBtnTextColor').value;
          props.borderColor = document.getElementById('propBtnBorderColor').value;
          props.borderStyle = document.getElementById('propBtnBorderStyle').value;
        } else if (type === '超链接') {
          const lblEl = document.getElementById('propLinkLabel');
          const hrefEl = document.getElementById('propLinkHref');
          const tgtEl = document.getElementById('propLinkTargetBlank');
          if (lblEl) props.label = String(lblEl.value || '').trim();
          if (hrefEl) props.href = String(hrefEl.value || '').trim();
          if (tgtEl) props.targetBlank = !!tgtEl.checked;

          const btnColorEl = document.getElementById('propLinkBtnColor');
          const activeColorEl = document.getElementById('propLinkActiveColor');
          const textColorEl = document.getElementById('propLinkTextColor');
          const borderColorEl = document.getElementById('propLinkBorderColor');
          const borderStyleEl = document.getElementById('propLinkBorderStyle');
          if (btnColorEl) props.buttonColor = btnColorEl.value;
          if (activeColorEl) props.activeColor = activeColorEl.value;
          if (textColorEl) props.textColor = textColorEl.value;
          if (borderColorEl) props.borderColor = borderColorEl.value;
          if (borderStyleEl) props.borderStyle = borderStyleEl.value;
        } else if (type === '滑块') {
          props.min = Number(document.getElementById('propSliderMin').value);
          props.max = Number(document.getElementById('propSliderMax').value);
          props.value = Number(document.getElementById('propSliderValue').value);
          const sbEl = document.getElementById('propSliderBarColor');
          const sfEl = document.getElementById('propSliderFillColor');
          const svEl = document.getElementById('propSliderValueColor');
          const sbcEl = document.getElementById('propSliderBorderColor');
          const sbsEl = document.getElementById('propSliderBorderStyle');
          if (sbEl) props.barColor = sbEl.value;
          if (sfEl) props.fillColor = sfEl.value;
          if (svEl) props.valueColor = svEl.value;
          if (sbcEl) props.borderColor = sbcEl.value;
          if (sbsEl) props.borderStyle = sbsEl.value;
        } else if (type === '浮点滑块') {
          props.min = Number(document.getElementById('propFloatSliderMin').value);
          props.max = Number(document.getElementById('propFloatSliderMax').value);
          props.step = Number(document.getElementById('propFloatSliderStep').value);
          props.value = Number(document.getElementById('propFloatSliderValue').value);
          const fsbEl = document.getElementById('propFloatSliderBarColor');
          const fsfEl = document.getElementById('propFloatSliderFillColor');
          const fsvEl = document.getElementById('propFloatSliderValueColor');
          const fsbcEl = document.getElementById('propFloatSliderBorderColor');
          const fsbsEl = document.getElementById('propFloatSliderBorderStyle');
          if (fsbEl) props.barColor = fsbEl.value;
          if (fsfEl) props.fillColor = fsfEl.value;
          if (fsvEl) props.valueColor = fsvEl.value;
          if (fsbcEl) props.borderColor = fsbcEl.value;
          if (fsbsEl) props.borderStyle = fsbsEl.value;
        } else if (type === '勾选') {
          props.label = document.getElementById('propCheckLabel').value.trim();
          props.checked = document.getElementById('propCheckChecked').checked;
          const caEl = document.getElementById('propCheckActiveColor');
          const ctEl = document.getElementById('propCheckTextColor');
          const cbEl = document.getElementById('propCheckBorderColor');
          const cbsEl = document.getElementById('propCheckBorderStyle');
          if (caEl) props.activeColor = caEl.value;
          if (ctEl) props.textColor = ctEl.value;
          if (cbEl) props.borderColor = cbEl.value;
          if (cbsEl) props.borderStyle = cbsEl.value;
        } else if (type === '开关') {
          props.checked = document.getElementById('propSwitchChecked').checked;
          const soEl = document.getElementById('propSwitchOnColor');
          const sfEl = document.getElementById('propSwitchOffColor');
          const sbEl = document.getElementById('propSwitchBorderColor');
          const sbsEl = document.getElementById('propSwitchBorderStyle');
          if (soEl) props.onColor = soEl.value;
          if (sfEl) props.offColor = sfEl.value;
          if (sbEl) props.borderColor = sbEl.value;
          if (sbsEl) props.borderStyle = sbsEl.value;
        } else if (type === '输入框') {
          props.placeholder = document.getElementById('propInputPlaceholder').value.trim();
          props.value = document.getElementById('propInputValue').value;
          const itEl = document.getElementById('propInputTextColor');
          const ibEl = document.getElementById('propInputBorderColor');
          const ibsEl = document.getElementById('propInputBorderStyle');
          if (itEl) props.textColor = itEl.value;
          if (ibEl) props.borderColor = ibEl.value;
          if (ibsEl) props.borderStyle = ibsEl.value;
        } else if (type === '竖向分割线') {
          const vlcEl = document.getElementById('propVDividerLineColor');
          const vlwEl = document.getElementById('propVDividerLineWidth');
          const vbsEl = document.getElementById('propVDividerBorderStyle');
          if (vlcEl) props.lineColor = vlcEl.value;
          if (vlwEl) props.lineWidth = Number(vlwEl.value);
          if (vbsEl) props.borderStyle = vbsEl.value;
        } else if (type === '分割线') {
          props.text = document.getElementById('propDividerText').value.trim();
          const dtEl = document.getElementById('propDividerTextColor');
          const dlcEl = document.getElementById('propDividerLineColor');
          const dlwEl = document.getElementById('propDividerLineWidth');
          const dbsEl = document.getElementById('propDividerBorderStyle');
          if (dtEl) props.textColor = dtEl.value;
          if (dlcEl) props.lineColor = dlcEl.value;
          if (dlwEl) props.lineWidth = Number(dlwEl.value);
          if (dbsEl) props.borderStyle = dbsEl.value;
        } else if (type === '标签') {
          props.text = document.getElementById('propLabelText').value.trim();
          const tColorEl = document.getElementById('propLabelTextColor');
          const bColorEl = document.getElementById('propLabelBorderColor');
          const bStyleEl = document.getElementById('propLabelBorderStyle');
          if (tColorEl) props.textColor = tColorEl.value;
          if (bColorEl) props.borderColor = bColorEl.value;
          if (bStyleEl) props.borderStyle = bStyleEl.value;
        } else if (type === '竖向滑动条') {
          props.min = Number(document.getElementById('propVSliderMin').value);
          props.max = Number(document.getElementById('propVSliderMax').value);
          props.value = Number(document.getElementById('propVSliderValue').value);
          props.height = document.getElementById('propVSliderHeight').value.trim();
          const vbEl = document.getElementById('propVSliderBarColor');
          const vfEl = document.getElementById('propVSliderFillColor');
          const vvEl = document.getElementById('propVSliderValueColor');
          const vbcEl = document.getElementById('propVSliderBorderColor');
          const vbsEl = document.getElementById('propVSliderBorderStyle');
          if (vbEl) props.barColor = vbEl.value;
          if (vfEl) props.fillColor = vfEl.value;
          if (vvEl) props.valueColor = vvEl.value;
          if (vbcEl) props.borderColor = vbcEl.value;
          if (vbsEl) props.borderStyle = vbsEl.value;
        } else if (type === '竖向浮点滑块') {
          props.min = Number(document.getElementById('propVFloatSliderMin').value);
          props.max = Number(document.getElementById('propVFloatSliderMax').value);
          props.step = Number(document.getElementById('propVFloatSliderStep').value);
          props.value = Number(document.getElementById('propVFloatSliderValue').value);
          props.height = document.getElementById('propVFloatSliderHeight').value.trim();
          const vfbEl = document.getElementById('propVFloatSliderBarColor');
          const vffEl = document.getElementById('propVFloatSliderFillColor');
          const vfvEl = document.getElementById('propVFloatSliderValueColor');
          const vfbcEl = document.getElementById('propVFloatSliderBorderColor');
          const vfbsEl = document.getElementById('propVFloatSliderBorderStyle');
          if (vfbEl) props.barColor = vfbEl.value;
          if (vffEl) props.fillColor = vffEl.value;
          if (vfvEl) props.valueColor = vfvEl.value;
          if (vfbcEl) props.borderColor = vfbcEl.value;
          if (vfbsEl) props.borderStyle = vfbsEl.value;
        } else if (type === '旋钮') {
          props.min = Number(document.getElementById('propKnobMin').value);
          props.max = Number(document.getElementById('propKnobMax').value);
          props.value = Number(document.getElementById('propKnobValue').value);
          props.step = Number(document.getElementById('propKnobStep').value);
          props.valueColor = document.getElementById('propKnobValueColor').value;
          const ktEl = document.getElementById('propKnobTextColor');
          const kbEl = document.getElementById('propKnobBorderColor');
          const kbsEl = document.getElementById('propKnobBorderStyle');
          const krEl = document.getElementById('propKnobRangeColor');
          const kswEl = document.getElementById('propKnobStrokeWidth');
          const ksvEl = document.getElementById('propKnobShowValue');
          if (ktEl) props.textColor = ktEl.value;
          if (kbEl) props.borderColor = kbEl.value;
          if (kbsEl) props.borderStyle = kbsEl.value;
          if (krEl) props.rangeColor = krEl.value;
          if (kswEl) props.strokeWidth = Number(kswEl.value);
          if (ksvEl) props.showValue = ksvEl.checked;
        } else if (type === '切换按钮') {
          props.labelOn = document.getElementById('propToggleLabelOn').value.trim();
          props.labelOff = document.getElementById('propToggleLabelOff').value.trim();
          props.active = document.getElementById('propToggleActive').checked;
          // 样式属性
          const btnColorEl = document.getElementById('propToggleBtnColor');
          const pressColorEl = document.getElementById('propTogglePressColor');
          const activeColorEl = document.getElementById('propToggleActiveColor');
          const textColorEl = document.getElementById('propToggleTextColor');
          const borderColorEl = document.getElementById('propToggleBorderColor');
          const borderStyleEl = document.getElementById('propToggleBorderStyle');
          if (btnColorEl) props.buttonColor = btnColorEl.value;
          if (pressColorEl) props.pressColor = pressColorEl.value;
          if (activeColorEl) props.activeColor = activeColorEl.value;
          if (textColorEl) props.textColor = textColorEl.value;
          if (borderColorEl) props.borderColor = borderColorEl.value;
          if (borderStyleEl) props.borderStyle = borderStyleEl.value;
        }
        EPWidgets.setProps(ctx.state.currentSelected, props);
        try {
          if (ctx.services.NS.activeTabId) {
            const info = ctx.services.NS.grids.get(ctx.services.NS.activeTabId);
            if (info && info.grid) ctx.layout.saveLayoutLocal(ctx.services.NS.activeTabId, info.grid);
          }
        } catch {}
        if (!(opts && opts.noRefresh)) updatePropPanel(ctx.state.currentSelected);
      }

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
