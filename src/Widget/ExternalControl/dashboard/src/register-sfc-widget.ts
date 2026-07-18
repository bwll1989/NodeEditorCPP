import type { Component } from 'vue';
import type { WidgetMeta } from './widget-types';

export function registerSfcWidget(component: Component, meta: WidgetMeta) {
  const ep = window.EPWidgets || (window.EPWidgets = {});
  const createVueSfcWidget = ep.createVueSfcWidget as
    | ((grid: unknown, cfg: Record<string, unknown>) => HTMLElement)
    | undefined;

  if (typeof createVueSfcWidget !== 'function') {
    throw new Error('EPWidgets.createVueSfcWidget 未就绪，请先加载 ep-widgets.js');
  }

  const factoryName = meta.factory;
  if (!factoryName) {
    throw new Error(`控件 ${meta.type} 缺少 factory 名称`);
  }

  ep[factoryName] = function createWidget(
    grid: unknown,
    initialProps: Record<string, unknown> = {},
    opts: Record<string, unknown> = {},
  ) {
    return createVueSfcWidget(grid, {
      type: meta.type,
      component,
      initialProps,
      opts,
      defaultW: meta.defaultW ?? 4,
      defaultH: meta.defaultH ?? 2,
      defaults: { ...meta.defaults },
      propKeys: meta.propKeys ?? Object.keys(meta.defaults),
      valueMapper: meta.valueMapper,
      coercers: meta.coercers,
      onNodeCreated: meta.onNodeCreated,
    });
  };

  if (factoryName === 'createEPLinkWidget') {
    ep.createEPHyperlinkWidget = ep[factoryName];
  }
}
