// 函数级注释：向像素画布容器添加一个标准控件容器（支持初始化位置/尺寸）
function addWidgetDom(grid, contentEl, w = 4, h = 2, x = undefined, y = undefined) {
  const wrapper = document.createElement('div');
  wrapper.className = 'grid-stack-item';
  const inner = document.createElement('div');
  inner.className = 'grid-stack-item-content';
  inner.style.position = 'absolute';
  inner.style.inset = '0';
  inner.style.width = '100%';
  inner.style.height = '100%';
  contentEl.style.height = '100%';
  inner.appendChild(contentEl);
  wrapper.appendChild(inner);

  const host = (grid && grid.el) ? grid.el : grid;
  if (!host || host.nodeType !== 1) return wrapper;

  wrapper.style.position = 'absolute';
  wrapper.style.left = (typeof x === 'number' ? x : 0) + 'px';
  wrapper.style.top = (typeof y === 'number' ? y : 0) + 'px';
  wrapper.style.width = (typeof w === 'number' ? w : 200) + 'px';
  wrapper.style.height = (typeof h === 'number' ? h : 120) + 'px';
  host.appendChild(wrapper);
  return wrapper;
}

// 函数级注释：导出必要API到全局，供控件容器创建/定位使用
window.Widgets = {
  addWidgetDom,
};
