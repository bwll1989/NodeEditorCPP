// widget-manifest.js —— 控件注册表（SFC bundle 注册，供加载器、SW、控件库共用）
(function() {
  'use strict';

  const ENTRIES = [
    { type: 'Trigger 按钮', libId: 'addButtonWidget', thumbLabel: 'Trigger 按钮', factory: 'createEPButtonWidget' },
    { type: '滑块', libId: 'addSliderWidget', thumbLabel: '滑块', factory: 'createEPSliderWidget' },
    { type: '勾选', libId: 'addCheckboxWidget', thumbLabel: '勾选', factory: 'createEPCheckboxWidget' },
    { type: '开关', libId: 'addSwitchWidget', thumbLabel: '开关', factory: 'createEPSwitchWidget' },
    { type: 'LED', libId: 'addLedWidget', thumbLabel: 'LED', factory: 'createEPLedWidget' },
    { type: '输入框', libId: 'addInputWidget', thumbLabel: '输入框', factory: 'createEPInputWidget' },
    { type: 'Toggle 按钮', libId: 'addToggleButtonWidget', thumbLabel: 'Toggle 按钮', factory: 'createEPToggleButtonWidget' },
    { type: '分割线', libId: 'addDividerWidget', thumbLabel: '分割线', factory: 'createEPDividerWidget' },
    { type: '标签', libId: 'addLabelWidget', thumbLabel: '标签', factory: 'createEPLabelWidget' },
    { type: 'Text', libId: 'addTextWidget', thumbLabel: 'Text', factory: 'createEPTextWidget' },
    { type: '旋钮', libId: 'addKnobWidget', thumbLabel: '旋钮', factory: 'createEPKnobWidget' },
    { type: '时间码', libId: 'addTimecodeWidget', thumbLabel: 'Timecode', factory: 'createEPTimecodeWidget' },
    { type: '数值', libId: 'addNumberWidget', thumbLabel: '数值', factory: 'createEPNumberWidget' },
    { type: '步进器', libId: 'addStepperWidget', thumbLabel: '步进器', factory: 'createEPStepperWidget' },
    { type: '3D散点', libId: 'addScatter3DWidget', thumbLabel: '3D散点', factory: 'createEPScatter3DWidget' },
    { type: '3D折线', libId: 'addLine3DWidget', thumbLabel: '3D折线', factory: 'createEPLine3DWidget' },
    { type: 'Frame', libId: 'addFrameWidget', thumbLabel: 'Frame', factory: 'createEPFrameWidget' },
    { type: '超链接', libId: 'addLinkWidget', thumbLabel: '超链接', factory: 'createEPLinkWidget' },
    { type: '单选框', libId: 'addRadioWidget', thumbLabel: '单选框', factory: 'createEPRadioWidget' },
    { type: 'Web', libId: 'addWebWidget', thumbLabel: 'Web', factory: 'createEPWebWidget' }
  ];

  const SFC_BUNDLE = 'assets/widgets-sfc.js';

  const typeToEntry = {};
  const libIdToType = {};
  // 旧布局类型名 → 现行 type
  const TYPE_ALIASES = {
    '按钮': 'Trigger 按钮',
    '切换按钮': 'Toggle 按钮',
    '竖向分割线': '分割线',
    '浮点滑块': '滑块',
    '竖向滑动条': '滑块',
    '竖向浮点滑块': '滑块'
  };
  ENTRIES.forEach(entry => {
    typeToEntry[entry.type] = entry;
    if (entry.libId) libIdToType[entry.libId] = entry.type;
  });

  function resolveType(type) {
    const t = String(type || '').trim();
    return TYPE_ALIASES[t] || t;
  }

  window.NSWidgetManifest = {
    entries: ENTRIES,
    sfcBundle: SFC_BUNDLE,
    libraryEntries() {
      return ENTRIES.filter(e => e.libId);
    },
    getScriptByType(type) {
      const entry = typeToEntry[resolveType(type)];
      if (!entry) return '';
      const factory = entry.factory;
      if (factory && window.EPWidgets && typeof window.EPWidgets[factory] === 'function') {
        return '';
      }
      return SFC_BUNDLE;
    },
    getEntryByType(type) {
      return typeToEntry[resolveType(type)] || null;
    },
    getTypeByLibId(libId) {
      return libIdToType[String(libId || '').trim()] || '';
    },
    resolveType,
    getCacheUrls() {
      return [SFC_BUNDLE, 'assets/style.css'];
    }
  };
})();
