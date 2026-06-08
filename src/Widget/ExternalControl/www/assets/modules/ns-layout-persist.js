// ns-layout-persist.js —— 布局持久化：localStorage 防抖、历史栈提交、服务端静默同步
(function() {
  'use strict';

  const LOCAL_DEBOUNCE_MS = 400;
  const SERVER_DEBOUNCE_MS = 2000;

  const localTimers = new Map();
  let serverTimer = null;
  let lastServerHash = '';

  let collectPage = null;
  let pushHistory = null;
  let saveAllLayoutsSilent = null;

  function writeLocalStorage(tid, payload) {
    try {
      localStorage.setItem('ns_layout_' + tid, JSON.stringify(payload));
    } catch (err) {
      console.error('NSLayoutPersist: localStorage write failed', err);
    }
  }

  function flushLocalSave(tid, grid, withHistory) {
    if (!tid || !grid || typeof collectPage !== 'function') return;
    try {
      const payload = collectPage(tid, grid);
      writeLocalStorage(tid, payload);
      if (withHistory && typeof pushHistory === 'function') {
        pushHistory(tid, grid);
      }
    } catch (err) {
      console.error('NSLayoutPersist: flush failed', err);
    }
  }

  function scheduleServerSave() {
    if (typeof saveAllLayoutsSilent !== 'function') return;
    if (serverTimer) clearTimeout(serverTimer);
    serverTimer = setTimeout(() => {
      serverTimer = null;
      try {
        const NS = window.NS;
        if (!NS || !NS.grids || NS.grids.size === 0) return;
        const pages = {};
        NS.grids.forEach((info, pageId) => {
          if (!info || !info.grid || typeof collectPage !== 'function') return;
          pages[pageId] = collectPage(pageId, info.grid);
        });
        const payload = { tabs: NS.tabs || [], activeTabId: NS.activeTabId, pages };
        const hash = JSON.stringify(payload);
        if (hash === lastServerHash) return;
        lastServerHash = hash;
        saveAllLayoutsSilent(payload);
      } catch (err) {
        console.error('NSLayoutPersist: server schedule failed', err);
      }
    }, SERVER_DEBOUNCE_MS);
  }

  function scheduleSave(tid, grid) {
    if (!tid || !grid) return;
    if (localTimers.has(tid)) clearTimeout(localTimers.get(tid));
    localTimers.set(tid, setTimeout(() => {
      localTimers.delete(tid);
      flushLocalSave(tid, grid, false);
      scheduleServerSave();
    }, LOCAL_DEBOUNCE_MS));
  }

  function commitSave(tid, grid) {
    if (!tid || !grid) return;
    if (localTimers.has(tid)) {
      clearTimeout(localTimers.get(tid));
      localTimers.delete(tid);
    }
    flushLocalSave(tid, grid, true);
    scheduleServerSave();
  }

  function flushAllPending() {
    localTimers.forEach((timer, tid) => {
      clearTimeout(timer);
      const info = window.NS && window.NS.grids ? window.NS.grids.get(tid) : null;
      if (info && info.grid) flushLocalSave(tid, info.grid, false);
    });
    localTimers.clear();
    if (serverTimer) {
      clearTimeout(serverTimer);
      serverTimer = null;
      scheduleServerSave();
    }
  }

  window.NSLayoutPersist = {
    init(hooks) {
      collectPage = hooks && hooks.collectPage;
      pushHistory = hooks && hooks.pushHistory;
      saveAllLayoutsSilent = hooks && hooks.saveAllLayoutsSilent;
    },
    scheduleSave,
    commitSave,
    flushAllPending,
    invalidateServerHash() { lastServerHash = ''; }
  };

  window.addEventListener('beforeunload', () => {
    try { flushAllPending(); } catch {}
  });
})();
