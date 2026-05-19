// ns-utils.js —— 分组链操作、控件工厂/按需加载、布局项收集、分组包围盒/位移/缩放
(function() {
  'use strict';
  const NS = window.NS;

  // ===== 分组链操作 =====

  // 函数级注释：读取节点的分组链（外->内）；语义：groupId=当前层(最内层)，groupChain=父链(不包含自身)
  function __getNodeGroupChain(node) {
    try {
      const parentsRaw = String((node && node.dataset ? (node.dataset.groupChain || '') : '') || '').trim();
      const gid = String((node && node.dataset ? (node.dataset.groupId || '') : '') || '').trim();

      const parents = parentsRaw
        ? parentsRaw.split(/[\s]*[;,|]+[\s]*/).map(s => String(s || '').trim()).filter(Boolean)
        : [];

      if (gid) return [...parents, gid];
      return parents;
    } catch { return []; }
  }

  // 函数级注释：写回节点的分组链（外->内）；语义：groupId=当前层(最内层)，groupChain=父链(不包含自身)
  function __setNodeGroupChain(node, chain) {
    if (!node || !node.dataset) return;
    const arr = Array.isArray(chain) ? chain.map(s => String(s || '').trim()).filter(Boolean) : [];
    const leaf = arr.length ? String(arr[arr.length - 1] || '').trim() : '';
    const parents = arr.length > 1 ? arr.slice(0, -1) : [];

    try {
      if (parents.length) node.dataset.groupChain = parents.join(',');
      else node.removeAttribute('data-group-chain');
    } catch {}
    try {
      if (leaf) node.dataset.groupId = leaf;
      else node.removeAttribute('data-group-id');
    } catch {}
  }

  // 函数级注释：从布局 spec 归一化分组链
  function __normalizeGroupChainFromSpec(groupChain, groupId) {
    const gid = String(groupId || '').trim();

    let arr = [];
    if (Array.isArray(groupChain)) {
      arr = groupChain.map(s => String(s || '').trim()).filter(Boolean);
    } else {
      const raw = String(groupChain || '').trim();
      if (raw) arr = raw.split(/[\s]*[;,|]+[\s]*/).map(s => String(s || '').trim()).filter(Boolean);
    }

    if (!gid) return arr;
    if (!arr.length) return [gid];

    const first = arr[0] || '';
    const last = arr[arr.length - 1] || '';

    if (last === gid) return arr;
    if (first === gid) return arr.slice().reverse();

    const idx = arr.indexOf(gid);
    if (idx >= 0) return arr.slice(0, idx + 1);

    return [...arr, gid];
  }

  // 函数级注释：判断节点是否属于某个分组
  function __nodeInGroup(node, gid) {
    const g = String(gid || '').trim();
    if (!g) return false;
    const chain = __getNodeGroupChain(node);
    return !!(chain && chain.length && chain.indexOf(g) >= 0);
  }

  // 函数级注释：构建当前页分组的父子关系与深度
  function __buildGroupMeta(canvas) {
    const parentOf = new Map();
    const depthOf = new Map();
    if (!canvas) return { parentOf, depthOf };
    const nodes = Array.from(canvas.querySelectorAll('.grid-stack-item[data-group-id], .grid-stack-item[data-group-chain]'));
    nodes.forEach(n => {
      const chain = __getNodeGroupChain(n);
      chain.forEach((gid, idx) => {
        const g = String(gid || '').trim();
        if (!g) return;
        const p = idx > 0 ? String(chain[idx - 1] || '').trim() : '';
        if (p && !parentOf.has(g)) parentOf.set(g, p);
        const cur = depthOf.has(g) ? Number(depthOf.get(g)) : -1;
        if (idx > cur) depthOf.set(g, idx);
      });
    });
    return { parentOf, depthOf };
  }

  // 函数级注释：将选中的分组列表归一化
  function __normalizeSelectedGroupIds(canvas, gids) {
    const arr = (Array.isArray(gids) ? gids : []).map(g => String(g || '').trim()).filter(Boolean);
    if (!arr.length) return [];
    const meta = __buildGroupMeta(canvas);
    const sel = new Set(arr);
    return arr.filter(gid => {
      let p = meta.parentOf.get(gid) || '';
      while (p) {
        if (sel.has(p)) return false;
        p = meta.parentOf.get(p) || '';
      }
      return true;
    });
  }

  // 函数级注释：获取某个分组的成员节点列表
  function __getGroupMembers(canvas, gid) {
    if (!canvas) return [];
    const g = String(gid || '').trim();
    if (!g) return [];
    try {
      const nodes = Array.from(canvas.querySelectorAll('.grid-stack-item[data-group-id], .grid-stack-item[data-group-chain]'));
      return nodes.filter(n => __nodeInGroup(n, g));
    } catch { return []; }
  }

  // ===== 像素坐标读写（纯工具函数，供多模块复用） =====

  function __readRectPx(node) {
    const x = Number(node && node.dataset ? node.dataset.pxX : NaN);
    const y = Number(node && node.dataset ? node.dataset.pxY : NaN);
    const w = Number(node && node.dataset ? node.dataset.pxW : NaN);
    const h = Number(node && node.dataset ? node.dataset.pxH : NaN);
    if (Number.isFinite(x) && Number.isFinite(y) && Number.isFinite(w) && Number.isFinite(h)) return { x, y, w, h };
    const sx = parseFloat((node && node.style ? node.style.left : '0') || '0') || 0;
    const sy = parseFloat((node && node.style ? node.style.top : '0') || '0') || 0;
    const sw = parseFloat((node && node.style ? node.style.width : '0') || '0') || 0;
    const sh = parseFloat((node && node.style ? node.style.height : '0') || '0') || 0;
    return { x: sx, y: sy, w: sw, h: sh };
  }

  function __writeRectPx(node, r) {
    const x = Math.max(0, Math.round(Number(r && r.x) || 0));
    const y = Math.max(0, Math.round(Number(r && r.y) || 0));
    const w = Math.max(40, Math.round(Number(r && r.w) || 40));
    const h = Math.max(30, Math.round(Number(r && r.h) || 30));
    try {
      node.style.left = x + 'px';
      node.style.top = y + 'px';
      node.style.width = w + 'px';
      node.style.height = h + 'px';
    } catch {}
    try {
      node.dataset.pxX = String(x);
      node.dataset.pxY = String(y);
      node.dataset.pxW = String(w);
      node.dataset.pxH = String(h);
    } catch {}
  }

  // ===== 布局项收集 =====

  // 函数级注释：收集指定页面的控件项（全局作用域，供多分页保存使用；仅像素 rect）
  function collectGridItems(grid) {
    const items = [];
    const nodes = (grid && grid.el) ? grid.el.querySelectorAll('.grid-stack-item') : [];
    nodes.forEach(item => {
      const type = item.dataset.type || '';
      const props = EPWidgets.getProps(item);
      const groupId = String(item.dataset.groupId || '').trim();
      const groupChain = String(item.dataset.groupChain || '').trim();

      const pxX = Number(item.dataset.pxX);
      const pxY = Number(item.dataset.pxY);
      const pxW = Number(item.dataset.pxW);
      const pxH = Number(item.dataset.pxH);
      const hasPx = Number.isFinite(pxX) && Number.isFinite(pxY) && Number.isFinite(pxW) && Number.isFinite(pxH);

      if (!type) return;
      if (hasPx) {
        items.push({ type, props, groupId, groupChain, rect: { x: pxX, y: pxY, w: pxW, h: pxH } });
        return;
      }

      const x = Math.max(0, Math.round(Number(item.offsetLeft) || 0));
      const y = Math.max(0, Math.round(Number(item.offsetTop) || 0));
      const w = Math.max(10, Math.round(Number(item.offsetWidth) || 10));
      const h = Math.max(10, Math.round(Number(item.offsetHeight) || 10));
      items.push({ type, props, groupId, groupChain, rect: { x, y, w, h } });
    });
    return items;
  }

  // ===== 控件工厂 =====

  // 函数级注释：按控件类型创建控件（像素画布体系）
  function __createWidgetByType(grid, type, props, opts) {
    const t = String(type || '').trim();
    const p = props && typeof props === 'object' ? props : {};
    const o = opts && typeof opts === 'object' ? opts : {};
    if (t === '按钮' && typeof EPWidgets.createEPButtonWidget === 'function') return EPWidgets.createEPButtonWidget(grid, p, o);
    if (t === '滑块' && typeof EPWidgets.createEPSliderWidget === 'function') return EPWidgets.createEPSliderWidget(grid, p, o);
    if (t === '浮点滑块' && typeof EPWidgets.createEPFloatSliderWidget === 'function') return EPWidgets.createEPFloatSliderWidget(grid, p, o);
    if (t === '勾选' && typeof EPWidgets.createEPCheckboxWidget === 'function') return EPWidgets.createEPCheckboxWidget(grid, p, o);
    if (t === '开关' && typeof EPWidgets.createEPSwitchWidget === 'function') return EPWidgets.createEPSwitchWidget(grid, p, o);
    if (t === '输入框' && typeof EPWidgets.createEPInputWidget === 'function') return EPWidgets.createEPInputWidget(grid, p, o);
    if (t === '切换按钮' && typeof EPWidgets.createEPToggleButtonWidget === 'function') return EPWidgets.createEPToggleButtonWidget(grid, p, o);
    if (t === '分割线' && typeof EPWidgets.createEPDividerWidget === 'function') return EPWidgets.createEPDividerWidget(grid, p, o);
    if (t === '竖向分割线' && typeof EPWidgets.createEPVDividerWidget === 'function') return EPWidgets.createEPVDividerWidget(grid, p, o);
    if (t === '标签' && typeof EPWidgets.createEPLabelWidget === 'function') return EPWidgets.createEPLabelWidget(grid, p, o);
    if (t === '竖向滑动条' && typeof EPWidgets.createEPVSliderWidget === 'function') return EPWidgets.createEPVSliderWidget(grid, p, o);
    if (t === '竖向浮点滑块' && typeof EPWidgets.createEPVFloatSliderWidget === 'function') return EPWidgets.createEPVFloatSliderWidget(grid, p, o);
    if (t === '旋钮' && typeof EPWidgets.createEPKnobWidget === 'function') return EPWidgets.createEPKnobWidget(grid, p, o);
    if (t === '时间码' && typeof EPWidgets.createEPTimecodeWidget === 'function') return EPWidgets.createEPTimecodeWidget(grid, p, o);
    if (t === '时间线' && typeof EPWidgets.createEPTimelineWidget === 'function') return EPWidgets.createEPTimelineWidget(grid, p, o);
    if (t === '数值' && typeof EPWidgets.createEPNumberWidget === 'function') return EPWidgets.createEPNumberWidget(grid, p, o);
    if ((t === 'Frame' || t === '卡片') && typeof EPWidgets.createEPFrameWidget === 'function') return EPWidgets.createEPFrameWidget(grid, p, o);
    if (t === '超链接' && typeof EPWidgets.createEPLinkWidget === 'function') return EPWidgets.createEPLinkWidget(grid, p, o);
    return null;
  }

  // 函数级注释：根据规范在指定 Grid 上创建控件并绑定遮罩
  function createWidgetFromSpec(grid, spec) {
    if (!spec || !grid) return null;
    const rect = (spec.rect && typeof spec.rect === 'object') ? spec.rect : null;
    if (!rect || rect.x === undefined || rect.y === undefined || rect.w === undefined || rect.h === undefined) return null;

    const opts = { rect: rect };
    const p = spec.props || {};
    const groupId = String(spec.groupId || '').trim();
    const groupChain = String(spec.groupChain || '').trim();

    const node = __createWidgetByType(grid, spec.type, p, opts);
    if (node) {
      try { node.dataset.type = spec.type; } catch {}
      const chain = __normalizeGroupChainFromSpec(groupChain, groupId);
      __setNodeGroupChain(node, chain);
      NSInteract.attachOverlay(node);
    }
    return node;
  }

  // ===== 控件按需加载 =====

  const __nsWidgetTypeToScript = {
    '按钮': 'widgets/Button/widget.js',
    '滑块': 'widgets/Slider/widget.js',
    '浮点滑块': 'widgets/FloatSlider/widget.js',
    '竖向滑动条': 'widgets/VSlider/widget.js',
    '竖向浮点滑块': 'widgets/VFloatSlider/widget.js',
    '勾选': 'widgets/Checkbox/widget.js',
    '开关': 'widgets/Switch/widget.js',
    '输入框': 'widgets/Input/widget.js',
    '切换按钮': 'widgets/ToggleButton/widget.js',
    '分割线': 'widgets/Divider/widget.js',
    '竖向分割线': 'widgets/VDivider/widget.js',
    '标签': 'widgets/Label/widget.js',
    '旋钮': 'widgets/Knob/widget.js',
    '时间码': 'widgets/Timecode/widget.js',
    '时间线': 'widgets/Timeline/widget.js',
    '数值': 'widgets/Number/widget.js',
    'Frame': 'widgets/Frame/widget.js',
    '卡片': 'widgets/Frame/widget.js',
    '超链接': 'widgets/Hyperlink/widget.js'
  };

  function __getWidgetScriptByType(type) {
    const t = String(type || '').trim();
    return __nsWidgetTypeToScript[t] || '';
  }

  // 函数级注释：限制同时加载的脚本数量，避免移动端/弱网环境下出现大量请求排队或资源不足
  function __ensureScriptLoadLimiter() {
    if (NS.__scriptLoadLimiter) return NS.__scriptLoadLimiter;
    NS.__scriptLoadLimiter = { active: 0, queue: [], max: 3 };
    return NS.__scriptLoadLimiter;
  }

  // 函数级注释：将脚本加载任务加入队列，并在有空闲并发槽时执行
  function __enqueueScriptLoad(task) {
    const lim = __ensureScriptLoadLimiter();
    lim.queue.push(task);
    __drainScriptLoadQueue();
  }

  // 函数级注释：执行队列中的脚本加载任务（遵循 max 并发限制）
  function __drainScriptLoadQueue() {
    const lim = __ensureScriptLoadLimiter();
    while (lim.active < lim.max && lim.queue.length > 0) {
      const task = lim.queue.shift();
      if (!task) continue;
      lim.active++;
      try {
        task(() => {
          try { lim.active = Math.max(0, lim.active - 1); } catch {}
          __drainScriptLoadQueue();
        });
      } catch {
        try { lim.active = Math.max(0, lim.active - 1); } catch {}
      }
    }
  }

  function __loadScriptOnce(src) {
    const s = String(src || '').trim();
    if (!s) return Promise.resolve();
    if (NS.widgetScriptPromises[s]) return NS.widgetScriptPromises[s];
    NS.widgetScriptPromises[s] = new Promise((resolve) => {
      __enqueueScriptLoad((done) => {
        try {
          const el = document.createElement('script');
          el.src = s;
          el.async = true;
          el.onload = () => { try { done(); } catch {} resolve(); };
          el.onerror = () => { try { done(); } catch {} resolve(); };
          document.head.appendChild(el);
        } catch {
          try { done(); } catch {}
          resolve();
        }
      });
    });
    return NS.widgetScriptPromises[s];
  }

  function ensureEPReady(callback) {
    function loaded() {
      return (typeof window.Vue !== 'undefined' || typeof Vue !== 'undefined') && typeof window.ElementPlus !== 'undefined';
    }
    if (loaded()) { callback(); return; }
    if (typeof window.Vue === 'undefined' && typeof Vue === 'undefined') {
      var s = document.createElement('script');
      s.src = 'assets/vendor/vue.global.js';
      s.onload = function(){ callback(); };
      s.onerror = function(){ callback(); };
      document.head.appendChild(s);
    } else {
      var tries = 0;
      var timer = setInterval(function(){
        if (loaded() || (++tries > 40)) { clearInterval(timer); callback(); }
      }, 50);
    }
  }

  function ensureWidgetReady(type, callback) {
    ensureWidgetTypesReady([type], callback);
  }

  function ensureWidgetTypesReady(types, callback) {
    const list = Array.isArray(types) ? types : [];
    const uniq = Array.from(new Set(list.map(t => String(t || '').trim()).filter(Boolean)));
    const scripts = uniq.map(__getWidgetScriptByType).filter(Boolean);
    ensureEPReady(() => {
      Promise.all(scripts.map(__loadScriptOnce)).then(() => { callback(); }).catch(() => { callback(); });
    });
  }

  // ===== 分组包围盒/位移/缩放 =====

  function __getGroupRect(canvas, gid) {
    const members = __getGroupMembers(canvas, gid);
    if (!members || members.length === 0) return null;
    let minX = Infinity, minY = Infinity, maxR = -Infinity, maxB = -Infinity;
    members.forEach(n => {
      const r = __readRectPx(n);
      minX = Math.min(minX, r.x);
      minY = Math.min(minY, r.y);
      maxR = Math.max(maxR, r.x + r.w);
      maxB = Math.max(maxB, r.y + r.h);
    });
    return { x: minX, y: minY, w: Math.max(0, maxR - minX), h: Math.max(0, maxB - minY) };
  }

  function __shiftGroup(canvas, gid, dx, dy, boundW, boundH) {
    const members = __getGroupMembers(canvas, gid);
    if (!members || members.length === 0) return;
    members.forEach(n => {
      const r = __readRectPx(n);
      let x = r.x + (Number(dx) || 0);
      let y = r.y + (Number(dy) || 0);
      if (Number.isFinite(boundW)) x = Math.max(0, Math.min(boundW - r.w, x));
      if (Number.isFinite(boundH)) y = Math.max(0, Math.min(boundH - r.h, y));
      __writeRectPx(n, { x, y, w: r.w, h: r.h });
    });
  }

  function __resizeGroupTo(canvas, gid, targetW, targetH, boundW, boundH) {
    const members = __getGroupMembers(canvas, gid);
    if (!members || members.length === 0) return;
    const base = __getGroupRect(canvas, gid);
    if (!base || base.w <= 0 || base.h <= 0) return;
    const sx = Math.max(0.05, Number(targetW) / base.w);
    const sy = Math.max(0.05, Number(targetH) / base.h);
    members.forEach(n => {
      const r = __readRectPx(n);
      let x = base.x + (r.x - base.x) * sx;
      let y = base.y + (r.y - base.y) * sy;
      let w = r.w * sx;
      let h = r.h * sy;
      if (Number.isFinite(boundW)) x = Math.max(0, Math.min(boundW - w, x));
      if (Number.isFinite(boundH)) y = Math.max(0, Math.min(boundH - h, y));
      __writeRectPx(n, { x, y, w, h });
    });
  }

  // ===== 侧边栏合并 =====

  function mergeLeftSidebarIntoRight() {
    try {
      if (window.__ns_left_sidebar_merged) return;
      window.__ns_left_sidebar_merged = true;
      const left = document.querySelector('.sidebar-left');
      const right = document.querySelector('.sidebar-right');
      if (!left || !right) return;
      const sections = Array.from(left.children || []).filter(el => el && el.classList && el.classList.contains('sidebar-section'));
      const anchor = right.firstElementChild;
      sections.forEach(sec => {
        try { right.insertBefore(sec, anchor); } catch { try { right.appendChild(sec); } catch {} }
      });
      try { if (left.parentElement) left.parentElement.removeChild(left); } catch {}
    } catch {}
  }

  // 导出
  window.NSUtils = {
    __getNodeGroupChain,
    __setNodeGroupChain,
    __normalizeGroupChainFromSpec,
    __nodeInGroup,
    __buildGroupMeta,
    __normalizeSelectedGroupIds,
    __getGroupMembers,
    collectGridItems,
    __createWidgetByType,
    createWidgetFromSpec,
    __getWidgetScriptByType,
    __loadScriptOnce,
    ensureEPReady,
    ensureWidgetReady,
    ensureWidgetTypesReady,
    __readRectPx,
    __writeRectPx,
    __getGroupRect,
    __shiftGroup,
    __resizeGroupTo,
    mergeLeftSidebarIntoRight
  };
})();
