// ns-dashboard-ctx.js —— 仪表盘统一上下文（state + 分模块 API + 对外 api）
(function() {
  'use strict';

  function createNSDashboardCtx() {
    return {
      services: {
        NS: window.NS,
        NSCanvas: window.NSCanvas,
        NSWsSync: window.NSWsSync,
        NSUtils: window.NSUtils,
        get NSInteract() { return window.NSInteract; }
      },
      state: {
        currentSelected: null,
        anchorSelected: null,
        selectedNodes: new Set(),
        selectedGroupIds: new Set(),
        selectedGroupAnchor: '',
        groupDrillId: '',
        lastMouseCanvasPos: null,
        clipboard: null,
        pasteLock: false
      },
      history: {},
      layout: {},
      props: {},
      selection: {},
      edit: {},
      tabs: {},
      api: {}
    };
  }

  function buildPublicApi(ctx, extras) {
    const ex = extras || {};
    const st = ctx.state;
    ctx.api = {
      getActiveGrid: () => ctx.layout.getActiveGrid(),
      saveLayoutLocal: (...args) => ctx.layout.saveLayoutLocal(...args),
      commitLayoutLocal: (...args) => ctx.layout.commitLayoutLocal(...args),
      saveLayout: (...args) => ctx.layout.saveLayout(...args),
      saveAllLayouts: (...args) => ctx.layout.saveAllLayouts(...args),
      attachAndSelect: ex.attachAndSelect,
      getGlobalEditMode: () => ctx.edit.getGlobalEditMode(),
      applyEditModeAll: (...args) => ctx.edit.applyEditModeAll(...args),
      setGlobalEditMode: (...args) => ctx.edit.setGlobalEditMode(...args),
      updatePropPanel: (...args) => ctx.props.updatePropPanel(...args),
      applyProperties: (...args) => ctx.props.applyProperties(...args),
      undoLayout: () => ctx.history.undoLayout(),
      redoLayout: () => ctx.history.redoLayout(),
      copySelection: () => ctx.history.copySelection(),
      pasteSelection: () => ctx.history.pasteSelection(),
      selectNode: (...args) => ctx.selection.selectNode(...args),
      clearSelection: () => ctx.selection.clearSelection(),
      selectGroup: (...args) => ctx.selection.selectGroup(...args),
      createTab: (...args) => ctx.tabs.createTab(...args),
      switchTab: (...args) => ctx.tabs.switchTab(...args),
      beginTabRender: (...args) => ctx.history.beginTabRender(...args),
      isTabRenderCurrent: (...args) => ctx.history.isTabRenderCurrent(...args),
      _getSelectedNodes: () => st.selectedNodes,
      _getCurrentSelected: () => st.currentSelected,
      _getAnchorSelected: () => st.anchorSelected,
      _setCurrentSelected: (v) => { st.currentSelected = v; },
      _setAnchorSelected: (v) => { st.anchorSelected = v; },
      __initColorInput: (...args) => ctx.props.__initColorInput(...args),
      __upgradeColorInputs: (...args) => ctx.props.__upgradeColorInputs(...args),
      __closeEpColorPickers: (...args) => ctx.props.__closeEpColorPickers(...args)
    };
    return ctx.api;
  }

  window.NSDashboardCtxFactory = {
    create: createNSDashboardCtx,
    buildPublicApi
  };
})();
