// NodeStudio 全局命名空间 —— 所有模块共享的状态
(function() {
  'use strict';
  window.NS = window.NS || {};

  // 所有分页数据 Map<tid, {name, tabBtn, pageWrap, viewportEl, grid, canvasEl, view, design, loaded, renderSeq, rendering, ...}>
  NS.grids = new Map();

  // 当前激活页 ID
  NS.activeTabId = null;

  // 分页列表 [{id, name}, ...]
  NS.tabs = [];

  // WebSocket 实例
  NS.ws = null;

  // WebSocket 重连定时器
  NS.wsReconnectTimer = null;

  // WebSocket 心跳定时器
  NS.wsHeartbeatTimer = null;

  // 心跳间隔
  NS.WS_HEARTBEAT_INTERVAL_MS = 5000;

  // queryAllStatuses 防抖定时器
  NS.queryStatusTimer = null;

  // 分组指示器 RAF
  NS.groupIndicatorRaf = 0;

  // 控件脚本加载缓存
  NS.widgetScriptPromises = {};
})();
