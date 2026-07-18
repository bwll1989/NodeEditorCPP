<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '时间码',
  factory: 'createEPTimecodeWidget',
  defaultW: 28,
  defaultH: 10,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '30',
    textColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
    fps: 25,
    value: 0,
    readOnly: false,
    btnBgColor: '#f8fafc',
    btnBorderColor: '#cbd5e1',
    btnTextColor: '#111827',
  },
  coercers: {
    fontSize: (v) => String(v ?? '30'),
    fps: (v) => Number(v),
    value: (v) => Number(v),
    readOnly: (v) => toBool(v),
  },
  valueMapper(value) {
    return { value: Number(value) };
  },
};
</script>

<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, reactive, ref, watch } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  value: number;
  fps: number;
  fontSize: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  readOnly: boolean;
  btnBgColor: string;
  btnBorderColor: string;
  btnTextColor: string;
}>();

const rootEl = ref<HTMLElement | null>(null);
let ro: ResizeObserver | null = null;
let onWinResize: (() => void) | null = null;

const ui = reactive({
  pad: 10,
  gap: 10,
  btnH: 46,
  btnFont: 22,
  btnRadius: 6,
  displayMargin: 6,
});

const normFps = computed(() => {
  const f = Number(s.fps);
  if (!Number.isFinite(f)) return 25;
  return Math.max(1, Math.floor(f));
});

const parts = computed(() => {
  const fps = normFps.value;
  const total = Math.max(0, Math.floor(Number(s.value) || 0));
  const perHour = fps * 3600;
  const perMin = fps * 60;
  const h = Math.floor(total / perHour);
  const remH = total % perHour;
  const m = Math.floor(remH / perMin);
  const remM = remH % perMin;
  const sec = Math.floor(remM / fps);
  const f = remM % fps;
  return { h, m, s: sec, f };
});

const timecodeText = computed(() => {
  const pad2 = (n: number) => String(Math.max(0, Math.floor(n))).padStart(2, '0');
  const { h, m, s: sec, f } = parts.value;
  const hh = String(Math.max(0, Math.floor(h))).padStart(2, '0');
  return `${hh}:${pad2(m)}:${pad2(sec)}:${pad2(f)}`;
});

const containerStyle = computed(() => {
  const pad = ui.pad;
  const radius = Math.max(6, Math.min(10, pad));
  return {
    width: '100%',
    height: '100%',
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'stretch',
    justifyContent: 'space-between',
    padding: pad + 'px',
    borderRadius: radius + 'px',
    borderColor: s.borderColor,
    borderStyle: s.borderStyle,
    borderWidth: '1px',
    boxSizing: 'border-box',
    background: 'transparent',
  };
});

const rowStyle = computed(() => ({
  display: 'grid',
  gridTemplateColumns: 'repeat(4, 1fr)',
  gap: ui.gap + 'px',
}));

const buttonStyle = computed(() => ({
  height: ui.btnH + 'px',
  borderRadius: ui.btnRadius + 'px',
  border: `1px solid ${s.btnBorderColor}`,
  background: s.btnBgColor,
  color: s.btnTextColor,
  fontSize: ui.btnFont + 'px',
  fontWeight: 700,
  lineHeight: '1',
  cursor: 'pointer',
  userSelect: 'none',
  padding: '0',
  touchAction: 'manipulation',
}));

const displayStyle = computed(() => {
  const fs = Number(s.fontSize);
  const fontPx = Number.isFinite(fs) ? fs : 64;
  return {
    flex: '1',
    display: 'flex',
    alignItems: 'center',
    justifyContent: 'center',
    color: s.textColor,
    fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Consolas, "Liberation Mono", monospace',
    fontSize: `${fontPx}px`,
    fontWeight: 700,
    letterSpacing: '1px',
    lineHeight: '1.05',
    margin: ui.displayMargin + 'px 0',
  };
});

function recalcUi() {
  const el = rootEl.value;
  if (!el) return;
  const h = Math.max(0, Number(el.clientHeight) || 0);
  const clamp = (min: number, v: number, max: number) => Math.max(min, Math.min(max, v));
  ui.pad = Math.round(clamp(6, h * 0.06, 10));
  ui.btnH = Math.round(clamp(20, h * 0.16, 36));
  ui.gap = Math.round(clamp(4, h * 0.045, 10));
  ui.btnFont = Math.round(clamp(12, ui.btnH * 0.55, 20));
  ui.btnRadius = Math.round(clamp(4, ui.btnH * 0.16, 8));
  ui.displayMargin = Math.round(clamp(4, h * 0.03, 8));
}

function setFrames(v: number) {
  s.value = Math.max(0, Math.floor(Number(v) || 0));
}

function stepFrames(unit: 'h' | 'm' | 's' | 'f') {
  const fps = normFps.value;
  if (unit === 'h') return fps * 3600;
  if (unit === 'm') return fps * 60;
  if (unit === 's') return fps;
  return 1;
}

function inc(unit: 'h' | 'm' | 's' | 'f') {
  if (s.readOnly) return;
  setFrames((Number(s.value) || 0) + stepFrames(unit));
}

function dec(unit: 'h' | 'm' | 's' | 'f') {
  if (s.readOnly) return;
  setFrames((Number(s.value) || 0) - stepFrames(unit));
}

watch(
  () => s.value,
  (nv) => {
    const addr = s.commandId || '/cmd/demo';
    const frames = Math.max(0, Math.floor(Number(nv) || 0));
    sendCommand(addr, String(frames));
  },
);

watch(
  () => s.fps,
  () => {
    setFrames(s.value);
  },
);

onMounted(() => {
  recalcUi();
  onWinResize = () => {
    try {
      recalcUi();
    } catch {}
  };
  try {
    window.addEventListener('resize', onWinResize);
  } catch {}
  try {
    if (typeof ResizeObserver !== 'undefined' && rootEl.value) {
      ro = new ResizeObserver(() => {
        try {
          recalcUi();
        } catch {}
      });
      ro.observe(rootEl.value);
    }
  } catch {}
});

onBeforeUnmount(() => {
  try {
    if (ro) {
      ro.disconnect();
      ro = null;
    }
  } catch {}
  try {
    if (onWinResize) {
      window.removeEventListener('resize', onWinResize);
      onWinResize = null;
    }
  } catch {}
});
</script>

<template>
  <div ref="rootEl" :style="containerStyle">
    <div v-if="!s.readOnly" :style="rowStyle">
      <button type="button" :style="buttonStyle" @click="inc('h')">+</button>
      <button type="button" :style="buttonStyle" @click="inc('m')">+</button>
      <button type="button" :style="buttonStyle" @click="inc('s')">+</button>
      <button type="button" :style="buttonStyle" @click="inc('f')">+</button>
    </div>

    <div :style="displayStyle">{{ timecodeText }}</div>

    <div v-if="!s.readOnly" :style="rowStyle">
      <button type="button" :style="buttonStyle" @click="dec('h')">-</button>
      <button type="button" :style="buttonStyle" @click="dec('m')">-</button>
      <button type="button" :style="buttonStyle" @click="dec('s')">-</button>
      <button type="button" :style="buttonStyle" @click="dec('f')">-</button>
    </div>
  </div>
</template>
