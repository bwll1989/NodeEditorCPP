import { inject } from 'vue';

export function useWidgetState<T extends Record<string, unknown> = Record<string, unknown>>() {
  const state = inject<T>('epWidgetState');
  if (!state) {
    throw new Error('epWidgetState not provided');
  }
  return state;
}

export function sendCommand(addr: string, value: string | number) {
  const ep = window.EPWidgets;
  if (ep && typeof ep.sendCommand === 'function') {
    return ep.sendCommand(addr, value);
  }
  return Promise.resolve({ ok: false });
}

export function toBool(v: unknown): boolean {
  if (window.EPWidgets && typeof window.EPWidgets.toBool === 'function') {
    return window.EPWidgets.toBool(v);
  }
  if (v === true) return true;
  if (v === false) return false;
  const s = String(v ?? '').trim().toLowerCase();
  if (s === 'true' || s === '1' || s === 'on' || s === 'yes') return true;
  if (s === 'false' || s === '0' || s === 'off' || s === 'no' || s === '') return false;
  return !!v;
}
