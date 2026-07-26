<script lang="ts">
import type { WidgetMeta } from '../widget-types';

function asStringList(v: unknown): string[] {
  if (Array.isArray(v)) return v.map((x) => String(x ?? ''));
  if (typeof v === 'string') {
    const t = v.trim();
    if (!t) return [];
    try {
      const parsed = JSON.parse(t);
      if (Array.isArray(parsed)) return parsed.map((x) => String(x ?? ''));
    } catch {
      /* newline separated */
    }
    return t.split(/\r?\n/).map((x) => x.trim());
  }
  return [];
}

function clampCount(n: unknown): number {
  const v = Math.floor(Number(n));
  if (!Number.isFinite(v)) return 3;
  return Math.max(2, Math.min(16, v));
}

function padLabels(list: string[], count: number): string[] {
  const out = list.slice(0, count).map((x) => String(x ?? ''));
  while (out.length < count) out.push(`选项 ${out.length + 1}`);
  return out;
}

function clampSelected(value: unknown, count: number): number {
  const n = Math.floor(Number(value));
  if (!Number.isFinite(n)) return 0;
  return Math.max(0, Math.min(count - 1, n));
}

function normalizeDirection(v: unknown): 'horizontal' | 'vertical' {
  const s = String(v ?? '').trim().toLowerCase();
  if (s === 'horizontal' || s === 'h' || s === '横向' || s === 'row') return 'horizontal';
  return 'vertical';
}

export const widgetMeta: WidgetMeta = {
  type: '单选框',
  factory: 'createEPRadioWidget',
  defaultW: 10,
  defaultH: 6,
  defaults: {
    commandId: '/cmd/radio',
    bgColor: 'transparent',
    fontSize: '14',
    count: 3,
    labels: ['选项 1', '选项 2', '选项 3'],
    selected: 0,
    activeColor: '#409EFF',
    textColor: '#111827',
    borderColor: '#e5e7eb',
    borderStyle: 'none',
    direction: 'vertical',
  },
  valueMapper(value, ctx) {
    const props = (ctx && ctx.props) || {};
    const count = clampCount(props.count);
    return { selected: clampSelected(value, count) };
  },
  coercers: {
    count(v) {
      return clampCount(v);
    },
    selected(v) {
      return clampSelected(v, 16);
    },
    labels(v) {
      return asStringList(v);
    },
    direction(v) {
      return normalizeDirection(v);
    },
  },
};
</script>

<script setup lang="ts">
import { computed, toRef } from 'vue';
import { useWidgetState, sendCommand } from '../useWidgetState';

const s = useWidgetState<{
  commandId: string;
  count: number;
  labels: string[];
  selected: number;
  activeColor: string;
  textColor: string;
  borderColor: string;
  borderStyle: string;
  fontSize: string;
  direction: string;
}>();

const directionRef = toRef(s, 'direction');
const count = computed(() => clampCount(s.count));
const labels = computed(() => padLabels(asStringList(s.labels), count.value));
const selected = computed(() => clampSelected(s.selected, count.value));
const isHorizontal = computed(() => normalizeDirection(directionRef.value) === 'horizontal');

const containerStyle = computed(() => ({
  borderColor: s.borderColor,
  borderStyle: s.borderStyle,
  color: s.textColor,
  fontSize: (s.fontSize || '14') + 'px',
}));

function select(index: number) {
  const i = clampSelected(index, count.value);
  s.selected = i;
  sendCommand(s.commandId || '/cmd/radio', i);
}
</script>

<template>
  <div
    class="ns-radio-widget"
    :class="isHorizontal ? 'is-horizontal' : 'is-vertical'"
    :style="containerStyle"
    role="radiogroup"
  >
    <button
      v-for="(label, index) in labels"
      :key="index"
      type="button"
      class="ns-radio-item"
      role="radio"
      :aria-checked="selected === index"
      :class="{ active: selected === index }"
      :style="{
        '--ns-radio-active': s.activeColor,
        color: s.textColor,
      }"
      @click="select(index)"
    >
      <span class="ns-radio-dot" aria-hidden="true" />
      <span class="ns-radio-label">{{ label }}</span>
    </button>
  </div>
</template>

<style scoped>
.ns-radio-widget {
  width: 100%;
  height: 100%;
  display: flex;
  gap: 8px;
  padding: 6px;
  box-sizing: border-box;
  overflow: auto;
}
.ns-radio-widget.is-vertical {
  flex-direction: column;
  flex-wrap: nowrap;
  align-items: stretch;
  justify-content: center;
}
.ns-radio-widget.is-horizontal {
  flex-direction: row;
  flex-wrap: nowrap;
  align-items: center;
  justify-content: flex-start;
}
.ns-radio-widget.is-vertical .ns-radio-item {
  width: 100%;
}
.ns-radio-widget.is-horizontal .ns-radio-item {
  width: auto;
  flex: 0 0 auto;
  white-space: nowrap;
}
.ns-radio-item {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  margin: 0;
  padding: 4px 6px;
  border: 0;
  background: transparent;
  cursor: pointer;
  text-align: left;
  font: inherit;
  color: inherit;
  border-radius: 6px;
}
.ns-radio-item:hover {
  background: rgba(15, 23, 42, 0.04);
}
.ns-radio-dot {
  width: 1em;
  height: 1em;
  min-width: 1em;
  border-radius: 50%;
  box-sizing: border-box;
  border: 2px solid #94a3b8;
  background: transparent;
  position: relative;
  flex-shrink: 0;
}
.ns-radio-item.active .ns-radio-dot {
  border-color: var(--ns-radio-active, #409eff);
}
.ns-radio-item.active .ns-radio-dot::after {
  content: '';
  position: absolute;
  inset: 20%;
  border-radius: 50%;
  background: var(--ns-radio-active, #409eff);
}
.ns-radio-label {
  line-height: 1.3;
  user-select: none;
}
</style>
