<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

function toNum(v: unknown, fallback: number): number {
  const n = Number(v);
  return Number.isFinite(n) ? n : fallback;
}

function clampValue(value: number, min: number, max: number, precision: number): number {
  let n = value;
  if (!Number.isFinite(n)) n = 0;
  n = Math.max(min, Math.min(max, n));
  const p = Math.max(0, Math.floor(precision));
  if (p > 0) return Number(n.toFixed(p));
  return Math.round(n);
}

export const widgetMeta: WidgetMeta = {
  type: '步进器',
  factory: 'createEPStepperWidget',
  defaultW: 10,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/stepper',
    bgColor: 'transparent',
    fontSize: '14',
    value: 0,
    min: 0,
    max: 100,
    step: 1,
    precision: 0,
    readOnly: false,
    buttonColor: '#409EFF',
    textColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'solid',
  },
  valueMapper(value) {
    return { value: toNum(value, 0) };
  },
  coercers: {
    value(v) {
      return toNum(v, 0);
    },
    min(v) {
      return toNum(v, 0);
    },
    max(v) {
      return toNum(v, 100);
    },
    step(v) {
      const n = toNum(v, 1);
      return n === 0 ? 1 : Math.abs(n);
    },
    precision(v) {
      return Math.max(0, Math.floor(toNum(v, 0)));
    },
    readOnly(v) {
      return toBool(v);
    },
  },
};
</script>

<script setup lang="ts">
import { computed } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  value: number;
  min: number;
  max: number;
  step: number;
  precision: number;
  readOnly: boolean;
  buttonColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
  bgColor: string;
}>();

const min = computed(() => toNum(s.min, 0));
const max = computed(() => toNum(s.max, 100));
const step = computed(() => {
  const n = Math.abs(toNum(s.step, 1));
  return n === 0 ? 1 : n;
});
const precision = computed(() => Math.max(0, Math.floor(toNum(s.precision, 0))));
const value = computed(() => clampValue(toNum(s.value, 0), min.value, max.value, precision.value));
const display = computed(() => {
  const p = precision.value;
  return p > 0 ? value.value.toFixed(p) : String(Math.round(value.value));
});

const canDec = computed(() => !s.readOnly && value.value > min.value);
const canInc = computed(() => !s.readOnly && value.value < max.value);

const rootStyle = computed(() => ({
  width: '100%',
  height: '100%',
  display: 'flex',
  alignItems: 'stretch',
  gap: '6px',
  boxSizing: 'border-box' as const,
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  background: s.bgColor || 'transparent',
  fontSize: (s.fontSize || '14') + 'px',
  color: s.textColor,
  padding: '2px',
}));

const btnStyle = computed(() => ({
  flex: '0 0 auto',
  width: '2.2em',
  minWidth: '28px',
  border: `1px solid ${s.borderColor || '#e5e7eb'}`,
  borderRadius: '6px',
  background: s.buttonColor || '#409EFF',
  color: '#fff',
  cursor: s.readOnly ? 'not-allowed' : 'pointer',
  font: 'inherit',
  fontWeight: 600,
  lineHeight: 1,
  padding: 0,
  opacity: s.readOnly ? 0.5 : 1,
}));

const valueStyle = computed(() => ({
  flex: '1 1 auto',
  minWidth: 0,
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  border: `1px solid ${s.borderColor || '#e5e7eb'}`,
  borderRadius: '6px',
  background: '#fff',
  color: s.textColor,
  fontVariantNumeric: 'tabular-nums' as const,
  userSelect: 'none' as const,
}));

function commit(next: number) {
  const n = clampValue(next, min.value, max.value, precision.value);
  s.value = n;
  if (!s.readOnly) {
    sendCommand(s.commandId || '/cmd/stepper', n);
  }
}

function dec() {
  if (!canDec.value) return;
  commit(value.value - step.value);
}

function inc() {
  if (!canInc.value) return;
  commit(value.value + step.value);
}
</script>

<template>
  <div class="ns-stepper" :style="rootStyle">
    <button
      type="button"
      class="ns-stepper-btn"
      :style="btnStyle"
      :disabled="!canDec"
      aria-label="减少"
      @click="dec"
    >
      −
    </button>
    <div class="ns-stepper-value" :style="valueStyle" :title="display">{{ display }}</div>
    <button
      type="button"
      class="ns-stepper-btn"
      :style="btnStyle"
      :disabled="!canInc"
      aria-label="增加"
      @click="inc"
    >
      +
    </button>
  </div>
</template>

<style scoped>
.ns-stepper-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}
.ns-stepper-btn:not(:disabled):hover {
  filter: brightness(0.95);
}
.ns-stepper-btn:not(:disabled):active {
  filter: brightness(0.9);
}
</style>
