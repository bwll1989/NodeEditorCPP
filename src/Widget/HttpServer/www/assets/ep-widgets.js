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

// 函数级注释：创建一个无标题的容器并挂载到 Gridstack（支持初始位置）
function createContainer(grid, title, w=4, h=2, opts) {
  const content = document.createElement('div');
  content.style.height = '100%';
  content.style.display = 'flex';
  content.style.flexDirection = 'column';
  content.className = 'widget-content-wrapper'; // 添加类名以便CSS控制

  // 不再创建 header，直接创建挂载点
  const mountNode = document.createElement('div');
  mountNode.style.flex = '1';
  mountNode.style.display = 'flex';
  mountNode.style.alignItems = 'center';
  mountNode.style.justifyContent = 'center';
  mountNode.style.overflow = 'hidden';
  mountNode.style.width = '100%';
  
  // 拖拽手柄区域，覆盖整个控件，但pointer-events需精细控制
  const handle = document.createElement('div');
  handle.className = 'widget-drag-handle';
  handle.style.position = 'absolute';
  handle.style.top = '0';
  handle.style.left = '0';
  handle.style.width = '100%';
  handle.style.height = '100%';
  handle.style.zIndex = '1'; // 位于底层
  
  // 挂载点位于上层，以便交互
  mountNode.style.zIndex = '2';
  mountNode.style.position = 'relative';

  // 编辑模式下的遮罩层，用于拖拽和选中
  const overlay = document.createElement('div');
  overlay.className = 'widget-overlay-mask';
  overlay.style.position = 'absolute';
  overlay.style.top = '0';
  overlay.style.left = '0';
  overlay.style.width = '100%';
  overlay.style.height = '100%';
  overlay.style.zIndex = '10'; // 最上层
  // 默认隐藏，由CSS控制显示
  
  content.appendChild(handle);
  content.appendChild(mountNode);
  content.appendChild(overlay);
  
  const node = window.Widgets.addWidgetDom(
    grid,
    content,
    (opts && typeof opts.w === 'number') ? opts.w : w,
    (opts && typeof opts.h === 'number') ? opts.h : h,
    (opts && typeof opts.x === 'number') ? opts.x : undefined,
    (opts && typeof opts.y === 'number') ? opts.y : undefined
  );
  return { node, mountNode, header: null }; // header 为 null
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

// 函数级注释：导出核心工具库
window.EPWidgets = {
  getVue,
  registerNode,
  createContainer,
  getProps,
  setProps,
  loadTemplate,
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
        // 容错：后端若返回非严格JSON，也不影响调用方
        try { return JSON.parse(t); } catch { return { ok: true, raw: t }; }
      }).catch(() => ({ ok: false }));
    } catch {
      return Promise.resolve({ ok: false });
    }
  }
};
