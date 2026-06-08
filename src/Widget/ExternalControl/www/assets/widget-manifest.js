// widget-manifest.js —— 控件注册表（脚本/模板/控件库元数据，供加载器、SW、控件库共用）
(function() {
  'use strict';

  const ENTRIES = [
    { type: '按钮', libId: 'addButtonWidget', thumbLabel: '按钮', script: 'widgets/Button/widget.js', template: 'widgets/Button/widget.html', factory: 'createEPButtonWidget' },
    { type: '滑块', libId: 'addSliderWidget', thumbLabel: '滑块', script: 'widgets/Slider/widget.js', template: 'widgets/Slider/widget.html', factory: 'createEPSliderWidget' },
    { type: '浮点滑块', libId: 'addFloatSliderWidget', thumbLabel: '浮点滑块', script: 'widgets/FloatSlider/widget.js', template: 'widgets/FloatSlider/widget.html', factory: 'createEPFloatSliderWidget' },
    { type: '竖向滑动条', libId: 'addVSliderWidget', thumbLabel: '竖向滑动条', script: 'widgets/VSlider/widget.js', template: 'widgets/VSlider/widget.html', factory: 'createEPVSliderWidget' },
    { type: '竖向浮点滑块', libId: 'addVFloatSliderWidget', thumbLabel: '竖向浮点滑块', script: 'widgets/VFloatSlider/widget.js', template: 'widgets/VFloatSlider/widget.html', factory: 'createEPVFloatSliderWidget' },
    { type: '勾选', libId: 'addCheckboxWidget', thumbLabel: '勾选', script: 'widgets/Checkbox/widget.js', template: 'widgets/Checkbox/widget.html', factory: 'createEPCheckboxWidget' },
    { type: '开关', libId: 'addSwitchWidget', thumbLabel: '开关', script: 'widgets/Switch/widget.js', template: 'widgets/Switch/widget.html', factory: 'createEPSwitchWidget' },
    { type: '输入框', libId: 'addInputWidget', thumbLabel: '输入框', script: 'widgets/Input/widget.js', template: 'widgets/Input/widget.html', factory: 'createEPInputWidget' },
    { type: '切换按钮', libId: 'addToggleButtonWidget', thumbLabel: '切换按钮', script: 'widgets/ToggleButton/widget.js', template: 'widgets/ToggleButton/widget.html', factory: 'createEPToggleButtonWidget' },
    { type: '分割线', libId: 'addDividerWidget', thumbLabel: '分割线', script: 'widgets/Divider/widget.js', template: 'widgets/Divider/widget.html', factory: 'createEPDividerWidget' },
    { type: '竖向分割线', libId: 'addVDividerWidget', thumbLabel: '竖向分割线', script: 'widgets/VDivider/widget.js', template: 'widgets/VDivider/widget.html', factory: 'createEPVDividerWidget' },
    { type: '标签', libId: 'addLabelWidget', thumbLabel: '标签', script: 'widgets/Label/widget.js', template: 'widgets/Label/widget.html', factory: 'createEPLabelWidget' },
    { type: '旋钮', libId: 'addKnobWidget', thumbLabel: '旋钮', script: 'widgets/Knob/widget.js', template: 'widgets/Knob/widget.html', factory: 'createEPKnobWidget' },
    { type: '时间码', libId: 'addTimecodeWidget', thumbLabel: 'Timecode', script: 'widgets/TimeCode/widget.js', template: 'widgets/TimeCode/widget.html', factory: 'createEPTimecodeWidget' },
    { type: '时间线', libId: 'addTimelineWidget', thumbLabel: '时间线', script: 'widgets/Timeline/widget.js', template: 'widgets/Timeline/widget.html', factory: 'createEPTimelineWidget' },
    { type: '数值', libId: 'addNumberWidget', thumbLabel: '数值', script: 'widgets/Number/widget.js', template: 'widgets/Number/widget.html', factory: 'createEPNumberWidget' },
    { type: 'Frame', libId: 'addFrameWidget', thumbLabel: 'Frame', script: 'widgets/Frame/widget.js', template: 'widgets/Frame/widget.html', factory: 'createEPFrameWidget' },
    { type: '超链接', libId: 'addLinkWidget', thumbLabel: '超链接', script: 'widgets/Hyperlink/widget.js', template: 'widgets/Hyperlink/widget.html', factory: 'createEPHyperlinkWidget' },
    { type: '卡片', script: 'widgets/Frame/widget.js', template: 'widgets/Frame/widget.html', factory: 'createEPFrameWidget' }
  ];

  const typeToScript = {};
  const typeToEntry = {};
  const libIdToType = {};
  ENTRIES.forEach(entry => {
    typeToScript[entry.type] = entry.script;
    typeToEntry[entry.type] = entry;
    if (entry.libId) libIdToType[entry.libId] = entry.type;
  });

  window.NSWidgetManifest = {
    entries: ENTRIES,
    libraryEntries() {
      return ENTRIES.filter(e => e.libId);
    },
    getScriptByType(type) {
      return typeToScript[String(type || '').trim()] || '';
    },
    getEntryByType(type) {
      return typeToEntry[String(type || '').trim()] || null;
    },
    getTypeByLibId(libId) {
      return libIdToType[String(libId || '').trim()] || '';
    },
    getCacheUrls() {
      const urls = new Set();
      ENTRIES.forEach(entry => {
        if (entry.script) urls.add(entry.script);
        if (entry.template) urls.add(entry.template);
      });
      return Array.from(urls);
    }
  };
})();
