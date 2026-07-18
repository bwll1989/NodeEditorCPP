import type { Component } from 'vue';

export interface WidgetMeta {
  type: string;
  factory: string;
  defaultW?: number;
  defaultH?: number;
  defaults: Record<string, unknown>;
  propKeys?: string[];
  valueMapper?: (value: unknown) => Record<string, unknown>;
  coercers?: Record<string, (v: unknown) => unknown>;
  onNodeCreated?: (node: HTMLElement) => void;
}

export interface WidgetModule {
  default: Component;
  widgetMeta: WidgetMeta;
}

declare global {
  interface Window {
    EPWidgets: Record<string, unknown> & {
      toBool?: (v: unknown) => boolean;
      sendCommand?: (addr: string, value: string | number) => Promise<{ ok: boolean }>;
    };
    Vue: typeof import('vue');
    ElementPlus: unknown;
  }
}
