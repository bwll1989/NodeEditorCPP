// 函数级注释：向 Gridstack 网格添加一个标准控件容器（支持初始化位置/尺寸）
function addWidgetDom(grid, contentEl, w = 4, h = 2, x = undefined, y = undefined) {
  const wrapper = document.createElement('div');
  wrapper.className = 'grid-stack-item';
  const inner = document.createElement('div');
  inner.className = 'grid-stack-item-content';
  // 确保内容元素填充整个容器
  contentEl.style.height = '100%';
  inner.appendChild(contentEl);
  wrapper.appendChild(inner);
  const opts = { w, h };
  if (typeof x === 'number') opts.x = x;
  if (typeof y === 'number') opts.y = y;
  grid.addWidget(wrapper, opts);
  return wrapper;
}

// 函数级注释：创建“按钮”控件，点击执行后端API并显示状态
function createButtonWidget(grid) {
  const content = document.createElement('div');
  content.innerHTML = `
    <div class="widget-title">按钮</div>
    <div class="d-flex gap-2">
      <input class="form-control form-control-sm" id="cmdInput" placeholder="输入命令，例如 start_camera"/>
      <button class="btn btn-sm btn-primary" id="execBtn">执行</button>
    </div>
    <div class="small text-muted mt-2" id="statusText">状态: 就绪</div>
  `;
  const node = addWidgetDom(grid, content, 4, 2);
  const execBtn = content.querySelector('#execBtn');
  const cmdInput = content.querySelector('#cmdInput');
  const statusText = content.querySelector('#statusText');
  execBtn.onclick = () => {
    const cmd = encodeURIComponent(cmdInput.value || 'demo');
    statusText.textContent = '状态: 执行中...';
    fetch(`/api/exec?cmd=${cmd}`)
      .then(r => r.json())
      .then(j => {
        statusText.textContent = '状态: 已执行 -> ' + (j.query || '');
      })
      .catch(e => {
        statusText.textContent = '状态: 失败 -> ' + e.message;
      });
  };
  return node;
}

// 函数级注释：创建“进度条”控件，支持模拟更新与重置
function createProgressWidget(grid) {
  const content = document.createElement('div');
  content.innerHTML = `
    <div class="widget-title">进度条</div>
    <div class="progress">
      <div class="progress-bar" role="progressbar" style="width: 0%" aria-valuemin="0" aria-valuemax="100">0%</div>
    </div>
    <div class="mt-2 d-flex gap-2">
      <button class="btn btn-sm btn-secondary" id="simulate">模拟进度</button>
      <button class="btn btn-sm btn-outline-danger" id="reset">重置</button>
    </div>
  `;
  const node = addWidgetDom(grid, content, 4, 2);
  const bar = content.querySelector('.progress-bar');
  content.querySelector('#simulate').onclick = () => {
    let v = parseInt(bar.textContent) || 0;
    v = Math.min(100, v + 10);
    bar.style.width = v + '%';
    bar.textContent = v + '%';
  };
  content.querySelector('#reset').onclick = () => {
    bar.style.width = '0%';
    bar.textContent = '0%';
  };
  return node;
}

// 函数级注释：导出控件工厂到全局，便于页面调用与后续扩展
window.Widgets = {
  addWidgetDom,
  createButtonWidget,
  createProgressWidget,
};
