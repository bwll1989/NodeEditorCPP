// widget-manifest.js —— 控件注册表（SFC bundle 注册，供加载器、SW、控件库共用）
(function() {
  'use strict';

  const ENTRIES = [
    { type: '按钮', libId: 'addButtonWidget', thumbLabel: '按钮', factory: 'createEPButtonWidget' },
    { type: '滑块', libId: 'addSliderWidget', thumbLabel: '滑块', factory: 'createEPSliderWidget' },
    { type: '浮点滑块', libId: 'addFloatSliderWidget', thumbLabel: '浮点滑块', factory: 'createEPFloatSliderWidget' },
    { type: '竖向滑动条', libId: 'addVSliderWidget', thumbLabel: '竖向滑动条', factory: 'createEPVSliderWidget' },
    { type: '竖向浮点滑块', libId: 'addVFloatSliderWidget', thumbLabel: '竖向浮点滑块', factory: 'createEPVFloatSliderWidget' },
    { type: '勾选', libId: 'addCheckboxWidget', thumbLabel: '勾选', factory: 'createEPCheckboxWidget' },
    { type: '开关', libId: 'addSwitchWidget', thumbLabel: '开关', factory: 'createEPSwitchWidget' },
    { type: '输入框', libId: 'addInputWidget', thumbLabel: '输入框', factory: 'createEPInputWidget' },
    { type: '切换按钮', libId: 'addToggleButtonWidget', thumbLabel: '切换按钮', factory: 'createEPToggleButtonWidget' },
    { type: '分割线', libId: 'addDividerWidget', thumbLabel: '分割线', factory: 'createEPDividerWidget' },
    { type: '竖向分割线', libId: 'addVDividerWidget', thumbLabel: '竖向分割线', factory: 'createEPVDividerWidget' },
    { type: '标签', libId: 'addLabelWidget', thumbLabel: '标签', factory: 'createEPLabelWidget' },
    { type: '旋钮', libId: 'addKnobWidget', thumbLabel: '旋钮', factory: 'createEPKnobWidget' },
    { type: '时间码', libId: 'addTimecodeWidget', thumbLabel: 'Timecode', factory: 'createEPTimecodeWidget' },
    { type: '时间线', libId: 'addTimelineWidget', thumbLabel: '时间线', factory: 'createEPTimelineWidget' },
    { type: '数值', libId: 'addNumberWidget', thumbLabel: '数值', factory: 'createEPNumberWidget' },
    { type: '3D散点', libId: 'addScatter3DWidget', thumbLabel: '3D散点', factory: 'createEPScatter3DWidget' },
    { type: '3D折线', libId: 'addLine3DWidget', thumbLabel: '3D折线', factory: 'createEPLine3DWidget' },
    { type: 'Frame', libId: 'addFrameWidget', thumbLabel: 'Frame', factory: 'createEPFrameWidget' },
    { type: '超链接', libId: 'addLinkWidget', thumbLabel: '超链接', factory: 'createEPLinkWidget' },
    { type: '卡片', factory: 'createEPCardWidget' }
  ];

  const SFC_BUNDLE = 'assets/widgets-sfc.js';

  const typeToEntry = {};
  const libIdToType = {};
  ENTRIES.forEach(entry => {
    typeToEntry[entry.type] = entry;
    if (entry.libId) libIdToType[entry.libId] = entry.type;
  });

  window.NSWidgetManifest = {
    entries: ENTRIES,
    sfcBundle: SFC_BUNDLE,
    libraryEntries() {
      return ENTRIES.filter(e => e.libId);
    },
    getScriptByType(type) {
      const entry = typeToEntry[String(type || '').trim()];
      if (!entry) return '';
      const factory = entry.factory;
      if (factory && window.EPWidgets && typeof window.EPWidgets[factory] === 'function') {
        return '';
      }
      return SFC_BUNDLE;
    },
    getEntryByType(type) {
      return typeToEntry[String(type || '').trim()] || null;
    },
    getTypeByLibId(libId) {
      return libIdToType[String(libId || '').trim()] || '';
    },
    getCacheUrls() {
      return [SFC_BUNDLE, 'assets/style.css'];
    }
  };
})();
