// 函数级注释：获取全局 Vue 引用（兼容不同打包）
function getVue() {
  if (typeof window !== 'undefined' && window.Vue) return window.Vue;
  if (typeof Vue !== 'undefined') return Vue;
  return undefined;
}

// 函数级注释：注册控件到映射，保存属性访问器，便于属性面板读写
const widgetRegistry = new WeakMap();
function registerNode(node, type, api) {
  node.dataset.type = type;
  const props = api.getProps ? api.getProps() : {};
  node.dataset.title = props.title || type;
  widgetRegistry.set(node, api);
}

// 函数级注释：解析像素布局参数（仅支持 opts.rect；放弃旧 grid 的 x/y/w/h 兼容）
function resolvePixelRect(hostEl, w, h, opts) {
  const rect = (opts && opts.rect && typeof opts.rect === 'object') ? opts.rect : null;
  if (rect && rect.x !== undefined && rect.y !== undefined && rect.w !== undefined && rect.h !== undefined) {
    return {
      x: Math.max(0, Math.round(Number(rect.x) || 0)),
      y: Math.max(0, Math.round(Number(rect.y) || 0)),
      w: Math.max(10, Math.round(Number(rect.w) || 10)),
      h: Math.max(10, Math.round(Number(rect.h) || 10)),
    };
  }

  const unitW = 16;
  const unitH = 20;
  const ww0 = Number(w);
  const hh0 = Number(h);
  const ww = Number.isFinite(ww0) ? ww0 : 10;
  const hh = Number.isFinite(hh0) ? hh0 : 6;

  const pxW = Math.max(60, Math.round(ww * unitW));
  const pxH = Math.max(40, Math.round(hh * unitH));

  return { x: 0, y: 0, w: pxW, h: pxH };
}

// 函数级注释：创建一个无标题的容器并挂载到像素画布（支持初始位置）
function createContainer(grid, title, w=4, h=2, opts) {
  const content = document.createElement('div');
  content.style.height = '100%';
  content.style.display = 'flex';
  content.style.flexDirection = 'column';
  content.className = 'widget-content-wrapper';

  const mountNode = document.createElement('div');
  mountNode.className = 'widget-mount';
  mountNode.style.flex = '1';
  mountNode.style.display = 'flex';
  mountNode.style.alignItems = 'stretch';
  mountNode.style.justifyContent = 'stretch';
  mountNode.style.overflow = 'hidden';
  mountNode.style.width = '100%';
  mountNode.style.height = '100%';

  const handle = document.createElement('div');
  handle.className = 'widget-drag-handle';
  handle.style.position = 'absolute';
  handle.style.top = '0';
  handle.style.left = '0';
  handle.style.width = '100%';
  handle.style.height = '100%';
  handle.style.zIndex = '1';

  mountNode.style.zIndex = '2';
  mountNode.style.position = 'relative';

  const overlay = document.createElement('div');
  overlay.className = 'widget-overlay-mask';
  overlay.style.position = 'absolute';
  overlay.style.top = '0';
  overlay.style.left = '0';
  overlay.style.width = '100%';
  overlay.style.height = '100%';
  overlay.style.zIndex = '10';

  content.appendChild(handle);
  content.appendChild(mountNode);
  content.appendChild(overlay);

  const host = (grid && grid.el) ? grid.el : grid;
  const r = resolvePixelRect(host, w, h, opts);
  const node = window.Widgets.addWidgetDom(host, content, r.w, r.h, r.x, r.y);
  try {
    node.dataset.pxX = String(r.x);
    node.dataset.pxY = String(r.y);
    node.dataset.pxW = String(r.w);
    node.dataset.pxH = String(r.h);
  } catch {}
  return { node, mountNode, header: null };
}

// 函数级注释：读取选中控件的属性
function getProps(node) {
  const api = widgetRegistry.get(node);
  return api ? api.getProps() : {};
}

// 函数级注释：设置选中控件的属性
function setProps(node, props) {
  const api = widgetRegistry.get(node);
  if (api && api.setProps) api.setProps(props);
}

// 函数级注释：通用模板加载器
function loadTemplate(path) {
  return fetch(path).then(res => res.text());
}

// 函数级注释：将任意输入转换为布尔（兼容 true/"true"/1 等）
function toBool(v) {
  if (v === true) return true;
  if (v === false) return false;
  const s = String(v ?? '').trim().toLowerCase();
  if (s === 'true' || s === '1' || s === 'on' || s === 'yes') return true;
  if (s === 'false' || s === '0' || s === 'off' || s === 'no' || s === '') return false;
  return !!v;
}

// 函数级注释：应用通用容器样式（背景色与字号），用于所有控件一致的外观
function applyCommonStyle(node, props) {
  try {
    const inner = node && node.querySelector ? node.querySelector('.grid-stack-item-content') : null;
    if (inner && props && props.bgColor) inner.style.backgroundColor = props.bgColor;
    if (node && props && props.fontSize !== undefined && props.fontSize !== null) {
      const fs = Number(props.fontSize);
      if (!Number.isNaN(fs)) node.style.fontSize = fs + 'px';
    }
  } catch {}
}

// 函数级注释：基于 defaults/initialProps/vm 自动生成 getProps/setProps，减少每个控件重复代码
function createPropsApi(params) {
  const { node, initialProps, vm, defaults, propKeys, valueMapper, coercers } = params || {};
  const keys = Array.isArray(propKeys) ? propKeys : Object.keys(defaults || {});

  if (defaults && initialProps) {
    keys.forEach(k => {
      if (initialProps[k] === undefined) initialProps[k] = defaults[k];
    });
  }

  return {
    getProps() {
      const out = {};
      keys.forEach(k => {
        if (vm && vm[k] !== undefined) out[k] = vm[k];
        else if (initialProps && initialProps[k] !== undefined) out[k] = initialProps[k];
        else if (defaults && defaults[k] !== undefined) out[k] = defaults[k];
      });
      return out;
    },
    setProps(p) {
      if (!p) return;
      let patch = p;
      if (p.value !== undefined && typeof valueMapper === 'function') {
        try {
          const mapped = valueMapper(p.value);
          if (mapped && typeof mapped === 'object') patch = Object.assign({}, p, mapped);
        } catch {}
      }

      keys.forEach(k => {
        if (patch[k] === undefined) return;
        let val = patch[k];
        if (coercers && coercers[k]) {
          try { val = coercers[k](val); } catch {}
        }
        if (initialProps) initialProps[k] = val;
        if (vm) {
          try { vm[k] = val; } catch {}
        }
      });

      if (patch.bgColor !== undefined || patch.fontSize !== undefined) {
        applyCommonStyle(node, initialProps || patch);
      }
    }
  };
}

// 函数级注释：创建并挂载一个基于模板的 Vue/ElementPlus 控件（自动注册属性API与应用通用样式）
function createVueWidget(grid, cfg) {
  const { type, templatePath, initialProps, opts, defaultW, defaultH, defaults, propKeys, valueMapper, coercers, appFactory } = cfg || {};
  const { getVue, registerNode, createContainer } = window.EPWidgets || {};

  const vue = getVue && getVue();
  const ready = !!vue && !!window.ElementPlus;
  const title = (initialProps && initialProps.title) || type || '控件';
  const { node, mountNode } = createContainer(grid, title, defaultW || 4, defaultH || 2, opts || {});

  registerNode(node, type || title, createPropsApi({ node, initialProps, vm: null, defaults: defaults || {}, propKeys, valueMapper, coercers }));

  if (!ready) {
    mountNode.innerHTML = '<div style="color:#b91c1c;font-size:12px;">Vue/ElementPlus 未加载或路径错误</div>';
    return node;
  }

  loadTemplate(templatePath).then(template => {
    const appOptions = (typeof appFactory === 'function') ? appFactory(template) : { template };
    const app = vue.createApp(appOptions);
    app.use(window.ElementPlus);
    const vm = app.mount(mountNode);
    node.vm = vm;

    applyCommonStyle(node, initialProps || {});
    registerNode(node, type || title, createPropsApi({ node, initialProps, vm, defaults: defaults || {}, propKeys, valueMapper, coercers }));
  }).catch(err => {
    mountNode.innerHTML = `<div style="color:red;font-size:12px;">加载模板失败: ${err}</div>`;
  });

  return node;
}

// 函数级注释：导出核心工具库
window.EPWidgets = {
  getVue,
  registerNode,
  createContainer,
  getProps,
  setProps,
  loadTemplate,
  toBool,
  applyCommonStyle,
  createPropsApi,
  createVueWidget,
  // 函数级注释：向后端发送指令JSON（addr/value）
  sendCommand(addr, value) {
    if (this.isRemoteUpdating) return Promise.resolve({ ok: true });
    try {
      const payload = { addr: String(addr || ''), value: String(value ?? '') };
      return fetch('/api/command', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      }).then(r => r.text()).then(t => {
        try { return JSON.parse(t); } catch { return { ok: true, raw: t }; }
      }).catch(() => ({ ok: false }));
    } catch {
      return Promise.resolve({ ok: false });
    }
  }
};
