<script lang="ts">
import type { WidgetMeta } from '../widget-types';

function normalizeDirection(v: unknown): 'horizontal' | 'vertical' {
  const s = String(v ?? '').trim().toLowerCase();
  if (s === 'vertical' || s === 'v' || s === '纵向' || s === '竖向' || s === 'column') return 'vertical';
  return 'horizontal';
}

function toNum(v: unknown, fallback: number): number {
  const n = Number(v);
  return Number.isFinite(n) ? n : fallback;
}

export const widgetMeta: WidgetMeta = {
  type: '滑块',
  factory: 'createEPSliderWidget',
  defaultW: 12,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/demo',
    bgColor: 'transparent',
    fontSize: '14',
    direction: 'horizontal',
    min: 0,
    max: 100,
    step: 1,
    value: 0,
    barColor: '#e5e7eb',
    fillColor: '#2b6cb0',
    valueColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
  },
  coercers: {
    fontSize: (v) => String(v ?? '14'),
    direction: (v) => normalizeDirection(v),
    min: (v) => toNum(v, 0),
    max: (v) => toNum(v, 100),
    step: (v) => {
      const n = Math.abs(toNum(v, 1));
      return n === 0 ? 1 : n;
    },
    value: (v) => toNum(v, 0),
  },
  valueMapper(value) {
    return { value: toNum(value, 0) };
  },
};
</script>

<script setup lang="ts">
import { computed, onMounted, ref, toRef, watch } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  direction: string;
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

const directionRef = toRef(s, 'direction');
const isVertical = computed(() => normalizeDirection(directionRef.value) === 'vertical');

function quantize(v: number) {
  const min = toNum(s.min, 0);
  const max = toNum(s.max, 100);
  const step = Math.max(0.000001, Math.abs(toNum(s.step, 1)) || 1);
  const q = Math.round((v - min) / step) * step + min;
  const clamped = Math.max(min, Math.min(max, q));
  // 避免浮点误差；整数步进时取整
  if (step >= 1 && Number.isInteger(step)) return Math.round(clamped);
  const decimals = Math.min(6, Math.max(0, String(step).split('.')[1]?.length || 2));
  return Number(clamped.toFixed(decimals));
}

function formatValue(v: number) {
  const step = Math.max(0.000001, Math.abs(toNum(s.step, 1)) || 1);
  if (step >= 1 && Number.isInteger(step)) return String(Math.round(v));
  const decimals = Math.min(6, Math.max(0, String(step).split('.')[1]?.length || 2));
  return Number(v).toFixed(decimals);
}

const barStyle = computed(() => ({
  position: 'relative' as const,
  width: '100%',
  height: '100%',
  backgroundColor: s.barColor,
  borderRadius: '4px',
  cursor: 'pointer',
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  touchAction: 'none' as const,
  boxSizing: 'border-box' as const,
}));

const fillStyle = computed(() => {
  if (isVertical.value) {
    return {
      position: 'absolute' as const,
      left: '0',
      bottom: '0',
      width: '100%',
      height: '0%',
      backgroundColor: s.fillColor,
      borderRadius: '4px',
    };
  }
  return {
    position: 'absolute' as const,
    left: '0',
    top: '0',
    bottom: '0',
    width: '0%',
    backgroundColor: s.fillColor,
    borderRadius: '4px',
  };
});

const valueStyle = computed(() => ({
  position: 'absolute' as const,
  left: '50%',
  top: '50%',
  transform: 'translate(-50%,-50%)',
  pointerEvents: 'none' as const,
  color: s.valueColor,
  fontWeight: 600,
  fontSize: (s.fontSize || '14') + 'px',
}));

function draw() {
  const fill = fillRef.value;
  const valEl = valRef.value;
  const min = toNum(s.min, 0);
  const max = toNum(s.max, 100);
  const range = Math.max(0.0001, max - min);
  const r = Math.max(0, Math.min(1, (toNum(s.value, 0) - min) / range));
  if (fill) {
    if (isVertical.value) {
      fill.style.width = '100%';
      fill.style.height = (r * 100).toFixed(2) + '%';
      fill.style.top = 'auto';
      fill.style.bottom = '0';
      fill.style.left = '0';
    } else {
      fill.style.height = '100%';
      fill.style.width = (r * 100).toFixed(2) + '%';
      fill.style.top = '0';
      fill.style.bottom = '0';
      fill.style.left = '0';
    }
  }
  if (valEl) valEl.textContent = formatValue(toNum(s.value, 0));
}

function toVal(evt: PointerEvent) {
  const bar = barRef.value;
  if (!bar) return toNum(s.value, 0);
  const rect = bar.getBoundingClientRect();
  const min = toNum(s.min, 0);
  const max = toNum(s.max, 100);
  let r = 0;
  if (isVertical.value) {
    const y = evt.clientY - rect.top;
    r = Math.max(0, Math.min(1, 1 - y / Math.max(1, rect.height)));
  } else {
    const x = evt.clientX - rect.left;
    r = Math.max(0, Math.min(1, x / Math.max(1, rect.width)));
  }
  return quantize(min + r * (max - min));
}

function onPointerDown(e: PointerEvent) {
  dragging = true;
  try {
    barRef.value?.setPointerCapture(e.pointerId);
  } catch {}
  s.value = toVal(e);
  draw();
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

watch(
  () => s.value,
  (nv) => {
    sendCommand(s.commandId || '/cmd/demo', String(nv));
  },
);

watch([() => s.value, () => s.min, () => s.max, () => s.step, isVertical], draw);

onMounted(() => {
  draw();
});
</script>

<template>
  <div class="ns-slider" :class="isVertical ? 'is-vertical' : 'is-horizontal'">
    <div
      ref="barRef"
      class="ns-slider-bar"
      :style="barStyle"
      @pointerdown="onPointerDown"
      @pointerup="onPointerUp"
      @pointercancel="onPointerUp"
      @pointermove="onPointerMove"
    >
      <div ref="fillRef" class="ns-slider-fill" :style="fillStyle" />
      <div ref="valRef" class="ns-slider-value" :style="valueStyle" />
    </div>
  </div>
</template>

<style scoped>
.ns-slider {
  width: 100%;
  height: 100%;
  display: flex;
}
.ns-slider-bar {
  flex: 1 1 auto;
}
</style>
