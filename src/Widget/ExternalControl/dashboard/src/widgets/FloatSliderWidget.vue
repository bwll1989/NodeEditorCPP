<script lang="ts">
import type { WidgetMeta } from '../widget-types';

export const widgetMeta: WidgetMeta = {
  type: '浮点滑块',
  factory: 'createEPFloatSliderWidget',
  defaultW: 12,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    min: 0,
    max: 1,
    step: 0.01,
    value: 0,
    barColor: '#e5e7eb',
    fillColor: '#2b6cb0',
    valueColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
  },
  coercers: {
    fontSize: (v) => String(v ?? '14'),
    min: (v) => Number(v),
    max: (v) => Number(v),
    step: (v) => Number(v),
    value: (v) => Number(v),
  },
  valueMapper(value) {
    return { value: Number(value) };
  },
};
</script>

<script setup lang="ts">
import { computed, onMounted, ref, watch } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  min: number;
  max: number;
  step: number;
  value: number;
  barColor: string;
  fillColor: string;
  valueColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
}>();

const barRef = ref<HTMLElement | null>(null);
const fillRef = ref<HTMLElement | null>(null);
const valRef = ref<HTMLElement | null>(null);
let dragging = false;

function quantize(v: number) {
  const step = Math.max(0.000001, s.step || 0.01);
  const q = Math.round((v - s.min) / step) * step + s.min;
  return Number(Math.max(s.min, Math.min(s.max, q)).toFixed(6));
}

const barStyle = computed(() => ({
  position: 'relative',
  width: '100%',
  height: '100%',
  backgroundColor: s.barColor,
  borderRadius: '4px',
  cursor: 'pointer',
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  touchAction: 'none',
}));

const fillStyle = computed(() => ({
  position: 'absolute',
  left: '0',
  top: '0',
  bottom: '0',
  width: '0',
  backgroundColor: s.fillColor,
  borderRadius: '4px',
}));

const valueStyle = computed(() => ({
  position: 'absolute',
  left: '50%',
  top: '50%',
  transform: 'translate(-50%,-50%)',
  pointerEvents: 'none',
  color: s.valueColor,
  fontWeight: 600,
  fontSize: s.fontSize + 'px',
}));

function draw() {
  const fill = fillRef.value;
  const valEl = valRef.value;
  const range = Math.max(0.0001, s.max - s.min);
  const r = Math.max(0, Math.min(1, (s.value - s.min) / range));
  if (fill) fill.style.width = (r * 100).toFixed(2) + '%';
  if (valEl) {
    const step = Math.max(0.000001, s.step || 0.01);
    const decimals = step < 1 ? 2 : 0;
    valEl.textContent = decimals ? Number(s.value).toFixed(decimals) : String(Math.round(s.value));
  }
}

function toVal(evt: PointerEvent) {
  const bar = barRef.value;
  if (!bar) return s.value;
  const rect = bar.getBoundingClientRect();
  const x = evt.clientX - rect.left;
  const w = rect.width;
  const r = Math.max(0, Math.min(1, x / Math.max(1, w)));
  return quantize(s.min + r * (s.max - s.min));
}

function onPointerDown(e: PointerEvent) {
  dragging = true;
  try {
    barRef.value?.setPointerCapture(e.pointerId);
  } catch {}
}

function onPointerUp(e: PointerEvent) {
  dragging = false;
  try {
    barRef.value?.releasePointerCapture(e.pointerId);
  } catch {}
}

function onPointerMove(e: PointerEvent) {
  if (!dragging) return;
  e.preventDefault();
  s.value = toVal(e);
  draw();
}

watch(() => s.value, (nv) => {
  sendCommand(s.commandId || '/cmd/demo', String(nv));
});

watch([() => s.value, () => s.min, () => s.max, () => s.step], draw);

onMounted(() => {
  draw();
});
</script>

<template>
  <div style="width:100%;height:100%;display:flex;">
    <div
      ref="barRef"
      :style="barStyle"
      @pointerdown="onPointerDown"
      @pointerup="onPointerUp"
      @pointercancel="onPointerUp"
      @pointermove="onPointerMove"
    >
      <div ref="fillRef" :style="fillStyle" />
      <div ref="valRef" :style="valueStyle" />
    </div>
  </div>
</template>
