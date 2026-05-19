// ns-ws-sync.js —— WebSocket 连接/心跳/重连/queryAllStatuses/全屏控制/设置页跳转
(function() {
  'use strict';
  const NS = window.NS;
  const NSCanvas = window.NSCanvas;

  const wsProtocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const wsUrl = wsProtocol + '//' + location.host + '/ws';

  // 函数级注释：更新顶部菜单中“主机”状态显示文本（基于 WebSocket 连接状态）
  function updateWebSocketStatusLabel(stateText) {
    try {
      const el = document.getElementById('wsStatusItem');
      if (!el) return;
      const txt = String(stateText || '').trim();
      el.textContent = '主机：' + (txt || '未知');
      if (txt === '已连接') {
        el.classList.remove('text-danger', 'text-warning');
        el.classList.add('text-success');
      } else if (txt === '连接中…') {
        el.classList.remove('text-success', 'text-danger');
        el.classList.add('text-warning');
      } else {
        el.classList.remove('text-success', 'text-warning');
        el.classList.add('text-danger');
      }
    } catch {}
  }

  // 函数级注释：查询所有已加载控件的状态（发送 query 指令），增加防抖处理避免短时间内多次查询
  function queryAllStatuses() {
    if (NS.queryStatusTimer) clearTimeout(NS.queryStatusTimer);
    NS.queryStatusTimer = setTimeout(() => {
      if (!NS.ws || NS.ws.readyState !== WebSocket.OPEN) return;
      // 直接从索引表读取所有 commandId，无需遍历 DOM
      const ids = new Set(EPWidgets.getCommandIndex().keys());
      const arr = Array.from(ids);
      if (arr.length > 0) {
        const CHUNK = 128;
        for (let i = 0; i < arr.length; i += CHUNK) {
          const sub = arr.slice(i, i + CHUNK);
          try { NS.ws.send(JSON.stringify({ query: sub })); } catch {}
        }
      }
    }, 300); // 300ms 防抖
  }

  // 函数级注释：跳转到设置页面（setting.html）
  function openSettingsPage() {
    window.location.href = 'setting.html';
  }

  // 函数级注释：向服务器提交设置页/编辑模式密码进行校验（POST /api/auth/setting）
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

  // 函数级注释：进入编辑模式前进行密码校验（若服务器未配置密码，则自动通过）
  async function requireSettingAuth() {
    const okEmpty = await verifySettingPassword('');
    if (okEmpty) return true;
    while (true) {
      const pw = window.prompt('请输入编辑模式密码');
      if (pw === null) return false;
      const ok = await verifySettingPassword(pw);
      if (ok) return true;
      window.alert('密码错误');
    }
  }

  // 函数级注释：初始化全屏控制（按钮绑定与状态同步）
  function setupFullscreenControls() {
    const btn = document.getElementById('toggleFullscreen');
    if (!btn) return;
    updateFullscreenButton();
    btn.addEventListener('click', toggleFullscreen);
    document.addEventListener('fullscreenchange', updateFullscreenButton);
    document.addEventListener('webkitfullscreenchange', updateFullscreenButton);
    document.addEventListener('msfullscreenchange', updateFullscreenButton);
  }

  // 函数级注释：切换全屏（进入/退出）
  function toggleFullscreen() {
    const doc = document;
    const isFs = doc.fullscreenElement || doc.webkitFullscreenElement || doc.msFullscreenElement;
    const root = document.documentElement;
    if (!isFs) {
      const req = root.requestFullscreen || root.webkitRequestFullscreen || root.msRequestFullscreen;
      if (req) req.call(root);
    } else {
      const exit = doc.exitFullscreen || doc.webkitExitFullscreen || doc.msExitFullscreen;
      if (exit) exit.call(doc);
    }
  }

  // 函数级注释：根据当前全屏状态更新按钮文本
  function updateFullscreenButton() {
    const btn = document.getElementById('toggleFullscreen');
    if (!btn) return;
    const doc = document;
    const isFs = doc.fullscreenElement || doc.webkitFullscreenElement || doc.msFullscreenElement;
    btn.textContent = isFs ? '退出全屏' : '全屏';
  }

  // 函数级注释：启动 WebSocket 心跳定时器（每隔固定时间发送一次无副作用 JSON，维持连接活跃）
  function startWebSocketHeartbeat() {
    stopWebSocketHeartbeat();
    NS.wsHeartbeatTimer = setInterval(() => {
      if (!NS.ws || NS.ws.readyState !== WebSocket.OPEN) return;
      try {
        NS.ws.send(JSON.stringify({ heartbeat: true, t: Date.now() }));
      } catch (e) {
      }
    }, NS.WS_HEARTBEAT_INTERVAL_MS);
  }

  // 函数级注释：停止 WebSocket 心跳定时器
  function stopWebSocketHeartbeat() {
    if (NS.wsHeartbeatTimer) {
      clearInterval(NS.wsHeartbeatTimer);
      NS.wsHeartbeatTimer = null;
    }
  }

  function connectWebSocket() {
    if (NS.ws) return;
    updateWebSocketStatusLabel('连接中…');
    NS.ws = new WebSocket(wsUrl);
    NS.ws.onopen = function() {
      console.log('WebSocket connected');
      if (NS.wsReconnectTimer) {
        clearTimeout(NS.wsReconnectTimer);
        NS.wsReconnectTimer = null;
      }
      startWebSocketHeartbeat();
      updateWebSocketStatusLabel('已连接');
      // 连接建立后尝试查询一次（处理重连场景）
      queryAllStatuses();
    };
    NS.ws.onmessage = function(event) {
      try {
        const msg = JSON.parse(event.data);
        if (msg.address) {
          try {
            window.dispatchEvent(new CustomEvent('ws-message', {
              detail: { commandId: String(msg.address), value: msg.value, text: msg.text, raw: msg }
            }));
          } catch (e) {
          }

          // 标记为远程更新，阻止 sendCommand
          EPWidgets.isRemoteUpdating = true;
          
          // 使用 commandId 索引表 O(1) 查找目标控件
          const targets = EPWidgets.getCommandIndex().get(msg.address);
          if (targets && targets.size > 0) {
            targets.forEach(item => {
              // 确认节点仍在 DOM 中（防止已删除但未清理的残留引用）
              if (item.isConnected) {
                EPWidgets.setProps(item, { value: msg.value });
              }
            });
          }
          
          // 恢复标志（等待 Vue watcher 周期结束）
          const vue = EPWidgets.getVue();
          if (vue && vue.nextTick) {
            vue.nextTick(() => { EPWidgets.isRemoteUpdating = false; });
          } else {
            setTimeout(() => { EPWidgets.isRemoteUpdating = false; }, 0);
          }
        }
      } catch (e) {
        console.error('WebSocket message error:', e);
        EPWidgets.isRemoteUpdating = false;
      }
    };
    NS.ws.onclose = function() {
      console.log('WebSocket closed');
      stopWebSocketHeartbeat();
      NS.ws = null;
      updateWebSocketStatusLabel('已断开，重连中…');
      if (!NS.wsReconnectTimer) {
        NS.wsReconnectTimer = setTimeout(connectWebSocket, 3000);
      }
    };
    NS.ws.onerror = function(err) {
      console.error('WebSocket error:', err);
      updateWebSocketStatusLabel('错误，重连中…');
    };
  }

  // 函数级注释：周期性根据 NS.ws.readyState 校正主机状态标签（防止偶发事件丢失或多 WebView 差异）
  function startWebSocketStatusWatcher() {
    if (NS.wsStatusTimer) clearInterval(NS.wsStatusTimer);
    NS.wsStatusTimer = setInterval(() => {
      try {
        const ws = NS.ws;
        if (!ws) {
          updateWebSocketStatusLabel('未连接');
          return;
        }
        switch (ws.readyState) {
          case WebSocket.CONNECTING:
            updateWebSocketStatusLabel('连接中…');
            break;
          case WebSocket.OPEN:
            updateWebSocketStatusLabel('已连接');
            break;
          case WebSocket.CLOSING:
            updateWebSocketStatusLabel('关闭中…');
            break;
          case WebSocket.CLOSED:
          default:
            updateWebSocketStatusLabel('未连接');
            break;
        }
      } catch {}
    }, 2000);
  }

  // 函数级注释：使用 Split.js 初始化分栏（画布 + 右侧边栏），并持久化右侧宽度
  function setupSplitPanels() {
    try { NSUtils.mergeLeftSidebarIntoRight(); } catch {}
    const shell = document.querySelector('.app-shell');
    const main = document.querySelector('.main-area');
    const right = document.querySelector('.sidebar-right');
    if (!shell || !main || !right || typeof window.Split === 'undefined') return;
    const MIN_RIGHT = 0, GUTTER = 6;

    let sizesPct = null;
    try {
      const rw = Number(localStorage.getItem('ns_sidebar_right_w') || 0);
      const total = shell.clientWidth || 1;
      if (rw > 0) {
        const rp = Math.max(MIN_RIGHT, rw) / total * 100;
        const mp = Math.max(0, 100 - rp);
        sizesPct = [mp, rp];
        right.classList.remove('collapsed');
      } else {
        right.classList.add('collapsed');
      }
    } catch {}

    const split = Split(['.main-area', '.sidebar-right'], {
      direction: 'horizontal',
      gutterSize: GUTTER,
      cursor: 'col-resize',
      minSize: [240, MIN_RIGHT],
      sizes: sizesPct || [75, 25],
      onDragEnd: (sizes) => {
        try {
          const total = shell.clientWidth || 1;
          const rp = sizes[1] / 100 * total;
          localStorage.setItem('ns_sidebar_right_w', String(Math.floor(rp)));
          localStorage.setItem('ns_split_sizes', JSON.stringify(sizes));
          if (sizes[1] > 0.5) localStorage.setItem('ns_right_last_pct', String(sizes[1]));
        } catch {}
        try { updateGutterVisibility(sizes); } catch {}
      }
    });
    window.__splitInstance = split;
    function updateGutterVisibility(sizes){
      const rp = sizes[1] || 0;
      try { right.classList.toggle('collapsed', rp <= 0.01); } catch {}
    }
    try {
      const guts = Array.from(document.querySelectorAll('.gutter.gutter-horizontal'));
      guts.forEach(g => {
        const existing = Array.from(g.querySelectorAll('.gutter-handle'));
        existing.slice(1).forEach(el => el.remove());
        if (existing.length === 0) {
          const h = document.createElement('div');
          h.className = 'gutter-handle';
          g.appendChild(h);
        }
      });
      const EPS = 0.01;
      const toggleRight = () => {
        const cur = split.getSizes();
        if ((cur[1] || 0) <= EPS) {
          const restore = Number(localStorage.getItem('ns_right_last_pct') || '25');
          split.setSizes([Math.max(0, 100 - restore), restore]);
        } else {
          localStorage.setItem('ns_right_last_pct', String(cur[1]));
          split.setSizes([100, 0]);
        }
        updateGutterVisibility(split.getSizes());
      };
      if (guts[0]) guts[0].addEventListener('dblclick', toggleRight);
      updateGutterVisibility(split.getSizes());
    } catch {}
  }

  // 自动连接
  if (document.readyState === 'complete') {
    connectWebSocket();
    startWebSocketStatusWatcher();
    setupFullscreenControls();
    setupSplitPanels();
  } else {
    window.addEventListener('load', () => {
      connectWebSocket();
      startWebSocketStatusWatcher();
      setupFullscreenControls();
      setupSplitPanels();
    });
  }

  // 导出
  window.NSWsSync = {
    queryAllStatuses,
    openSettingsPage,
    verifySettingPassword,
    requireSettingAuth,
    setupFullscreenControls,
    toggleFullscreen,
    updateFullscreenButton,
    startWebSocketHeartbeat,
    stopWebSocketHeartbeat,
    connectWebSocket,
    setupSplitPanels,
    updateWebSocketStatusLabel,
    startWebSocketStatusWatcher
  };
})();
