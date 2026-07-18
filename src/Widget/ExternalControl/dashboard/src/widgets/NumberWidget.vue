<script lang="ts">
import type { WidgetMeta } from '../widget-types';
import { toBool } from '../useWidgetState';

export const widgetMeta: WidgetMeta = {
  type: '数值',
  factory: 'createEPNumberWidget',
  defaultW: 10,
  defaultH: 2,
  defaults: {
    commandId: '/cmd/number',
    bgColor: 'transparent',
    fontSize: '14',
    value: 0,
    min: 0,
    max: 100,
    step: 1,
    precision: 0,
    readOnly: false,
    textColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
  },
  valueMapper(value) {
    let num = Number(value);
    if (Number.isNaN(num)) num = 0;
    return { value: num };
  },
  coercers: {
    value(v) {
      const n = Number(v);
      return Number.isNaN(n) ? 0 : n;
    },
    min(v) {
      const n = Number(v);
      return Number.isNaN(n) ? 0 : n;
    },
    max(v) {
      const n = Number(v);
      return Number.isNaN(n) ? 100 : n;
    },
    step(v) {
      const n = Number(v);
      return Number.isNaN(n) ? 1 : n;
    },
    precision(v) {
      const n = Number(v);
      return Number.isNaN(n) ? 0 : Math.max(0, Math.floor(n));
    },
    readOnly(v) {
      return toBool(v);
    },
  },
};
</script>

<script setup lang="ts">
import { computed, watch } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  value: number;
  min: number;
  max: number;
  step: number;
  precision: number;
  readOnly: boolean;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  bgColor: string;
}>();

const inputStyle = computed(() => ({
  width: '100%',
  height: '100%',
  boxSizing: 'border-box',
  color: s.textColor,
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  backgroundColor: s.bgColor,
  '--el-input-border-color': s.borderColor,
  '--el-input-hover-border-color': s.borderColor,
  '--el-input-focus-border-color': s.borderColor,
  '--el-fill-color-blank': s.bgColor,
  '--el-input-bg-color': s.bgColor,
  '--el-color-white': s.bgColor,
}));

watch(
  () => s.value,
  (nv) => {
    if (s.readOnly) return;
    const addr = s.commandId || '/cmd/number';
    let num = Number(nv);
    if (Number.isNaN(num)) num = 0;
    if (s.precision > 0) num = Number(num.toFixed(s.precision));
    else num = Math.round(num);
    if (s.min !== undefined) num = Math.max(s.min, num);
    if (s.max !== undefined) num = Math.min(s.max, num);
    sendCommand(addr, String(num));
  },
);

watch(
  () => s.precision,
  (nv) => {
    const p = Math.max(0, Math.floor(Number(nv) || 0));
    let num = Number(s.value);
    if (Number.isNaN(num)) num = 0;
    s.value = p > 0 ? Number(num.toFixed(p)) : Math.round(num);
  },
);

watch(
  () => s.min,
  (nv) => {
    const m = Number(nv);
    if (!Number.isNaN(m)) s.value = Math.max(m, Number(s.value) || 0);
  },
);

watch(
  () => s.max,
  (nv) => {
    const m = Number(nv);
    if (!Number.isNaN(m)) s.value = Math.min(m, Number(s.value) || 0);
  },
);
</script>

<template>
  <el-input-number
    v-model="s.value"
    :min="s.min"
    :max="s.max"
    :step="s.step"
    :precision="s.precision"
    :disabled="s.readOnly"
    size="small"
    :style="inputStyle"
  />
</template>
